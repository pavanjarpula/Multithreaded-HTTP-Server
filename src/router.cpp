#include "server/router.hpp"
#include "server/metrics.hpp"
#include "server/logger.hpp"

#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>

namespace server {

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    routes_[path][method] = std::move(handler);
}

HttpResponse Router::handle(const HttpRequest& req) {
    auto route_it = routes_.find(req.path);
    if (route_it == routes_.end()) {
        if (req.method == HttpMethod::GET || req.method == HttpMethod::HEAD) {
            auto file_resp = static_handler_.serve(req.path);
            if (file_resp.has_value()) {
                if (req.method == HttpMethod::HEAD) {
                    HttpResponse head_resp = file_resp.value();
                    head_resp.set_body("");
                    return head_resp;
                }
                return file_resp.value();
            }
        }
        return HttpResponse::not_found();
    }

    auto method_it = route_it->second.find(req.method_string);
    if (method_it == route_it->second.end()) {
        std::string allowed;
        for (auto& pair : route_it->second) {
            if (!allowed.empty()) allowed += ", ";
            allowed += pair.first;
        }
        return HttpResponse::method_not_allowed(allowed);
    }

    return method_it->second(req);
}

void Router::setup_default_routes() {
    add_route("GET", "/dashboard", [this](const HttpRequest&) {
        std::ifstream file(document_root_ + "/dashboard.html");
        if (!file.is_open()) {
            return HttpResponse::not_found("Dashboard not found");
        }
        std::string html((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        return HttpResponse::ok(html, "text/html");
    });

    add_route("GET", "/playground", [this](const HttpRequest&) {
        std::ifstream file(document_root_ + "/playground.html");
        if (!file.is_open()) {
            return HttpResponse::not_found("Playground not found");
        }
        std::string html((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        return HttpResponse::ok(html, "text/html");
    });

    add_route("GET", "/lab", [this](const HttpRequest&) {
        std::ifstream file(document_root_ + "/lab.html");
        if (!file.is_open()) {
            return HttpResponse::not_found("Lab not found");
        }
        std::string html((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        return HttpResponse::ok(html, "text/html");
    });

    add_route("GET", "/work", [](const HttpRequest& req) {
        std::string delay_str = "100";
        std::string body_str = "0";
        std::size_t qs = req.path.find('?');
        if (qs != std::string::npos) {
            std::string params = req.path.substr(qs + 1);
            std::size_t dpos = params.find("delay=");
            if (dpos != std::string::npos) {
                std::size_t end = params.find('&', dpos);
                delay_str = params.substr(dpos + 6, end == std::string::npos ? std::string::npos : end - dpos - 6);
            }
            std::size_t bpos = params.find("body=");
            if (bpos != std::string::npos) {
                std::size_t end = params.find('&', bpos);
                body_str = params.substr(bpos + 6, end == std::string::npos ? std::string::npos : end - bpos - 6);
            }
        }

        int delay_ms = 100;
        int body_kb = 0;
        try { delay_ms = std::stoi(delay_str); } catch (...) {}
        try { body_kb = std::stoi(body_str); } catch (...) {}
        if (delay_ms < 0) delay_ms = 0;
        if (delay_ms > 30000) delay_ms = 30000;
        if (body_kb < 0) body_kb = 0;
        if (body_kb > 10240) body_kb = 10240;

        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

        std::string body;
        if (body_kb > 0) {
            body.resize(static_cast<std::size_t>(body_kb) * 1024, 'X');
            std::mt19937 rng(std::random_device{}());
            std::uniform_int_distribution<int> dist(65, 90);
            for (auto& c : body) c = static_cast<char>(dist(rng));
        }

        auto end = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();

        std::ostringstream oss;
        oss << "{\n"
            << "  \"status\": \"work_done\",\n"
            << "  \"delay_ms\": " << delay_ms << ",\n"
            << "  \"actual_ms\": " << std::fixed << std::setprecision(1) << elapsed << ",\n"
            << "  \"body_kb\": " << body_kb << "\n"
            << "}";

        HttpResponse resp = HttpResponse::ok(oss.str(), "application/json");
        resp.set_header("X-Work-Delay", std::to_string(delay_ms));
        resp.set_header("X-Work-Actual", std::to_string(static_cast<int>(elapsed)));
        return resp;
    });

    add_route("GET", "/health", [](const HttpRequest&) {
        return HttpResponse::ok(R"({"status":"ok"})", "application/json");
    });

    add_route("GET", "/hello", [](const HttpRequest&) {
        return HttpResponse::ok("Hello, World!\n", "text/plain");
    });

    add_route("GET", "/metrics", [](const HttpRequest&) {
        return HttpResponse::ok(
            Metrics::instance().to_json(),
            "application/json"
        );
    });

    add_route("POST", "/echo", [](const HttpRequest& req) {
        return HttpResponse::ok(req.body, "text/plain");
    });

    add_route("HEAD", "/hello", [](const HttpRequest&) {
        return HttpResponse::ok("", "text/plain");
    });

    add_route("HEAD", "/health", [](const HttpRequest&) {
        return HttpResponse::ok("", "application/json");
    });

    LOG_INFO("Default routes registered");
}

} // namespace server

#include "server/router.hpp"
#include "server/metrics.hpp"
#include "server/logger.hpp"

#include <sstream>

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
    add_route("GET", "/", [](const HttpRequest&) {
        std::string html = R"(<!DOCTYPE html>
<html>
<head><title>C++ HTTP Server</title></head>
<body>
<h1>Welcome to the C++ HTTP Server</h1>
<p>A multithreaded HTTP/1.1 server built from scratch.</p>
<h2>Available Endpoints</h2>
<ul>
<li><a href="/health">/health</a> - Health check</li>
<li><a href="/hello">/hello</a> - Hello world</li>
<li><a href="/metrics">/metrics</a> - Server metrics</li>
<li><a href="/style.css">/style.css</a> - Static file example</li>
</ul>
<p>POST /echo - Echo request body</p>
</body>
</html>)";
        return HttpResponse::ok(html, "text/html");
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

    add_route("HEAD", "/", [](const HttpRequest&) {
        return HttpResponse::ok("", "text/html");
    });

    LOG_INFO("Default routes registered");
}

} // namespace server

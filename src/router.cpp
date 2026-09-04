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
    add_route("GET", "/dashboard", [](const HttpRequest&) {
        std::string html = R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Dashboard - C++ HTTP Server</title>
    <link rel="stylesheet" href="/dashboard.css">
</head>
<body>
    <div class="dashboard-container">
        <header>
            <h1>Live Monitoring Dashboard</h1>
            <div id="status-badge" class="badge offline">OFFLINE</div>
        </header>
        <div class="metrics-grid">
            <div class="metric-card">
                <div class="metric-label">Total Requests</div>
                <div class="metric-value" id="total-requests">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Successful</div>
                <div class="metric-value success" id="successful-requests">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Client Errors</div>
                <div class="metric-value warning" id="client-errors">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Server Errors</div>
                <div class="metric-value danger" id="server-errors">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Active Connections</div>
                <div class="metric-value" id="active-connections">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Peak Connections</div>
                <div class="metric-value" id="peak-connections">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Avg Latency</div>
                <div class="metric-value" id="avg-latency">0 ms</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Requests/sec</div>
                <div class="metric-value" id="rps">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Bytes Sent</div>
                <div class="metric-value" id="bytes-sent">0 B</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Uptime</div>
                <div class="metric-value" id="uptime">0s</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Worker Threads</div>
                <div class="metric-value" id="thread-count">0</div>
            </div>
            <div class="metric-card">
                <div class="metric-label">Queue Size</div>
                <div class="metric-value" id="queue-size">0</div>
            </div>
        </div>
        <div class="charts-section">
            <div class="chart-container">
                <h3>Requests Per Second</h3>
                <canvas id="rps-chart" width="600" height="200"></canvas>
            </div>
            <div class="chart-container">
                <h3>Average Latency (ms)</h3>
                <canvas id="latency-chart" width="600" height="200"></canvas>
            </div>
        </div>
    </div>
    <script src="/dashboard.js"></script>
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

    LOG_INFO("Default routes registered");
}

} // namespace server

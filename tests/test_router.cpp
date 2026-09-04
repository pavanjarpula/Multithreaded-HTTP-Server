#include "test_framework.hpp"
#include "server/router.hpp"
#include "server/http_parser.hpp"

using namespace server;

TEST(router_get_root) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
}

TEST(router_get_health) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/health";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "application/json");
    ASSERT_CONTAINS(serialized, "{\"status\":\"ok\"}");
}

TEST(router_get_hello) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/hello";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "Hello, World!");
}

TEST(router_post_echo) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::POST;
    req.method_string = "POST";
    req.path = "/echo";
    req.body = "test body content";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "test body content");
}

TEST(router_404_not_found) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/nonexistent";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 404);
}

TEST(router_405_wrong_method) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::UNKNOWN;
    req.method_string = "DELETE";
    req.path = "/hello";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 405);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "Allow");
}

TEST(router_head_hello) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::HEAD;
    req.method_string = "HEAD";
    req.path = "/hello";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
}

TEST(router_get_metrics) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/metrics";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "total_requests");
    ASSERT_CONTAINS(serialized, "worker_threads");
    ASSERT_CONTAINS(serialized, "queue_size");
    ASSERT_CONTAINS(serialized, "queue_capacity");
    ASSERT_CONTAINS(serialized, "active_workers");
    ASSERT_CONTAINS(serialized, "requests_per_second");
}

TEST(router_get_dashboard) {
    Router router("public");
    router.setup_default_routes();

    HttpRequest req;
    req.method = HttpMethod::GET;
    req.method_string = "GET";
    req.path = "/dashboard";

    HttpResponse resp = router.handle(req);
    ASSERT_EQ(static_cast<int>(resp.status_code()), 200);
    std::string serialized = resp.serialize();
    ASSERT_CONTAINS(serialized, "Live Monitoring Dashboard");
    ASSERT_CONTAINS(serialized, "dashboard.js");
}

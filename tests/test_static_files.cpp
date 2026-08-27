#include "test_framework.hpp"
#include "server/static_file_handler.hpp"

using namespace server;

TEST(static_serve_existing_file) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/hello.txt");
    ASSERT_TRUE(resp.has_value());
    std::string serialized = resp.value().serialize();
    ASSERT_CONTAINS(serialized, "Hello from the C++ HTTP Server");
}

TEST(static_serve_missing_file) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/nonexistent.txt");
    ASSERT_FALSE(resp.has_value());
}

TEST(static_directory_traversal_blocked) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/../../etc/passwd");
    // Should either be 403 or not found - never serve the file
    if (resp.has_value()) {
        ASSERT_EQ(static_cast<int>(resp.value().status_code()), 403);
    }
}

TEST(static_dotdot_traversal_blocked) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/../src/main.cpp");
    if (resp.has_value()) {
        ASSERT_EQ(static_cast<int>(resp.value().status_code()), 403);
    }
}

TEST(static_mime_type_html) {
    ASSERT_EQ(StaticFileHandler::mime_type(".html"), "text/html");
    ASSERT_EQ(StaticFileHandler::mime_type(".htm"), "text/html");
}

TEST(static_mime_type_css) {
    ASSERT_EQ(StaticFileHandler::mime_type(".css"), "text/css");
}

TEST(static_mime_type_js) {
    ASSERT_EQ(StaticFileHandler::mime_type(".js"), "application/javascript");
}

TEST(static_mime_type_json) {
    ASSERT_EQ(StaticFileHandler::mime_type(".json"), "application/json");
}

TEST(static_mime_type_txt) {
    ASSERT_EQ(StaticFileHandler::mime_type(".txt"), "text/plain");
}

TEST(static_mime_type_png) {
    ASSERT_EQ(StaticFileHandler::mime_type(".png"), "image/png");
}

TEST(static_mime_type_unknown) {
    ASSERT_EQ(StaticFileHandler::mime_type(".xyz"), "application/octet-stream");
}

TEST(static_serve_css) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/style.css");
    ASSERT_TRUE(resp.has_value());
    std::string serialized = resp.value().serialize();
    ASSERT_CONTAINS(serialized, "text/css");
}

TEST(static_serve_root_defaults_to_index) {
    StaticFileHandler handler("public");
    auto resp = handler.serve("/");
    ASSERT_TRUE(resp.has_value());
    std::string serialized = resp.value().serialize();
    ASSERT_CONTAINS(serialized, "text/html");
}

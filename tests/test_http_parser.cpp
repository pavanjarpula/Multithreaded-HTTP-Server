#include "test_framework.hpp"
#include "server/http_parser.hpp"

using namespace server;

TEST(parse_simple_get) {
    std::string raw = "GET /hello HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_EQ(result.method, "GET");
    ASSERT_EQ(result.path, "/hello");
    ASSERT_EQ(result.version, "HTTP/1.1");
}

TEST(parse_post_with_body) {
    std::string raw = "POST /echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nhello";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_EQ(result.method, "POST");
    ASSERT_EQ(result.path, "/echo");
    ASSERT_EQ(result.body, "hello");
}

TEST(parse_headers) {
    std::string raw = "GET / HTTP/1.1\r\nHost: example.com\r\nUser-Agent: test\r\nAccept: */*\r\n\r\n";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_CONTAINS(result.headers_raw, "Host: example.com");
    ASSERT_CONTAINS(result.headers_raw, "User-Agent: test");
}

TEST(parse_malformed_no_method) {
    std::string raw = "NOTAVALIDREQUEST\r\n\r\n";
    auto result = HttpParser::parse(raw);
    // The parser tries to parse 3 tokens; if it can, it returns NONE
    // "NOTAVALIDREQUEST" has no spaces, so stream >> method >> path >> version fails
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::MALFORMED));
}

TEST(parse_incomplete) {
    std::string raw = "GET / HTTP/1.1\r\nHost: localhost";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::INCOMPLETE));
}

TEST(parse_head_method) {
    std::string raw = "HEAD /hello HTTP/1.1\r\nHost: localhost\r\n\r\n";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_EQ(result.method, "HEAD");
}

TEST(parse_unknown_method) {
    std::string raw = "FOOBAR /test HTTP/1.1\r\n\r\n";
    auto result = HttpParser::parse(raw);
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_EQ(result.method, "FOOBAR");
}

TEST(parse_invalid_http_version) {
    std::string raw = "GET / HTTP/2.0\r\n\r\n";
    auto result = HttpParser::parse(raw);
    // HTTP/2.0 still starts with "HTTP/" so it parses as a valid request line
    ASSERT_EQ(static_cast<int>(result.error), static_cast<int>(HttpParser::ParseResult::Error::NONE));
    ASSERT_EQ(result.version, "HTTP/2.0");
}

TEST(build_request_converts_method) {
    std::string raw = "GET /test HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc";
    auto result = HttpParser::parse(raw);
    HttpRequest req = HttpParser::build_request(result);
    ASSERT_EQ(static_cast<int>(req.method), static_cast<int>(HttpMethod::GET));
    ASSERT_EQ(req.path, "/test");
    ASSERT_EQ(req.body, "abc");
}

TEST(build_request_post) {
    std::string raw = "POST /data HTTP/1.1\r\nContent-Type: text/plain\r\nContent-Length: 11\r\n\r\nhello world";
    auto result = HttpParser::parse(raw);
    HttpRequest req = HttpParser::build_request(result);
    ASSERT_EQ(static_cast<int>(req.method), static_cast<int>(HttpMethod::POST));
    ASSERT_EQ(req.body, "hello world");
    ASSERT_EQ(req.get_header("Content-Type"), "text/plain");
}

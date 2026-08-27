#pragma once

#include "http_request.hpp"
#include <string>

namespace server {

class HttpParser {
public:
    struct ParseResult {
        enum class Error {
            NONE,
            INCOMPLETE,
            MALFORMED,
            HEADER_TOO_LARGE,
            BODY_TOO_LARGE
        };

        Error error = Error::NONE;
        // raw request data preserved for caller to build HttpRequest
        std::string method;
        std::string path;
        std::string version;
        std::string headers_raw;
        std::string body;
        bool has_headers = false;
    };

    static ParseResult parse(const std::string& raw,
                             std::size_t max_header_size = 64 * 1024,
                             std::size_t max_body_size = 1 * 1024 * 1024);

    static HttpRequest build_request(const ParseResult& result);

private:
    static bool parse_request_line(const std::string& line, HttpRequest& req);
    static bool parse_header_line(const std::string& line, HttpRequest& req);
    static std::string trim(const std::string& s);
};

} // namespace server

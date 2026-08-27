#include "server/http_parser.hpp"

#include <sstream>
#include <algorithm>
#include <cctype>

namespace server {

HttpParser::ParseResult HttpParser::parse(const std::string& raw,
                                          std::size_t max_header_size,
                                          std::size_t max_body_size) {
    ParseResult result;

    // Find header/body separator
    std::size_t header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        result.error = ParseResult::Error::INCOMPLETE;
        return result;
    }

    // Check header size limit
    if (header_end + 4 > max_header_size) {
        result.error = ParseResult::Error::HEADER_TOO_LARGE;
        return result;
    }

    std::string headers_section = raw.substr(0, header_end);
    std::string body_data = raw.substr(header_end + 4);

    std::istringstream stream(headers_section);
    std::string line;

    // Parse request line
    if (!std::getline(stream, line)) {
        result.error = ParseResult::Error::MALFORMED;
        return result;
    }

    // Remove trailing \r if present
    if (!line.empty() && line.back() == '\r') line.pop_back();

    // Parse: METHOD PATH HTTP/x.y
    std::istringstream rl(line);
    if (!(rl >> result.method >> result.path >> result.version)) {
        result.error = ParseResult::Error::MALFORMED;
        return result;
    }

    // Validate HTTP version prefix
    if (result.version.find("HTTP/") != 0) {
        result.error = ParseResult::Error::MALFORMED;
        return result;
    }

    result.has_headers = true;

    // Store remaining headers for building HttpRequest
    std::ostringstream hdrs;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;
        hdrs << line << "\n";
    }
    result.headers_raw = hdrs.str();

    // Handle body based on Content-Length
    // Search for content-length in the raw request (up to and including \r\n\r\n)
    std::string raw_up_to_headers = raw.substr(0, header_end + 4);
    std::string lower_raw = raw_up_to_headers;
    for (auto& c : lower_raw) c = static_cast<char>(std::tolower(c));

    std::size_t cl_pos = lower_raw.find("content-length:");
    if (cl_pos != std::string::npos) {
        std::size_t colon_pos = raw_up_to_headers.find(':', cl_pos);
        std::size_t eol_pos = raw_up_to_headers.find("\r\n", colon_pos + 1);
        if (colon_pos != std::string::npos && eol_pos != std::string::npos) {
            std::string cl_value = raw_up_to_headers.substr(colon_pos + 1, eol_pos - colon_pos - 1);
            std::size_t start = cl_value.find_first_not_of(" \t");
            if (start != std::string::npos) {
                cl_value = cl_value.substr(start);
            }

            std::size_t content_length = 0;
            try {
                content_length = static_cast<std::size_t>(std::stoull(cl_value));
            } catch (...) {
                result.error = ParseResult::Error::MALFORMED;
                return result;
            }

            if (content_length > max_body_size) {
                result.error = ParseResult::Error::BODY_TOO_LARGE;
                return result;
            }

            if (body_data.size() < content_length) {
                result.error = ParseResult::Error::INCOMPLETE;
                return result;
            }

            result.body = body_data.substr(0, content_length);
        }
    } else {
        result.body = body_data;
    }

    result.error = ParseResult::Error::NONE;
    return result;
}

HttpRequest HttpParser::build_request(const ParseResult& result) {
    HttpRequest req;
    req.method_string = result.method;
    req.path = result.path;
    req.version = result.version;
    req.body = result.body;

    if (result.method == "GET") req.method = HttpMethod::GET;
    else if (result.method == "POST") req.method = HttpMethod::POST;
    else if (result.method == "HEAD") req.method = HttpMethod::HEAD;
    else req.method = HttpMethod::UNKNOWN;

    // Parse headers from headers_raw
    std::istringstream stream(result.headers_raw);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\n') line.pop_back();
        if (line.empty()) continue;

        std::size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim whitespace from value
            std::size_t s = value.find_first_not_of(" \t");
            if (s != std::string::npos) value = value.substr(s);
            std::size_t e = value.find_last_not_of(" \t");
            if (e != std::string::npos) value = value.substr(0, e + 1);
            req.headers[key] = value;
        }
    }

    return req;
}

std::string HttpParser::trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

} // namespace server

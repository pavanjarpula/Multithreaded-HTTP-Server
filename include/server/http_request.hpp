#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <cstddef>

namespace server {

enum class HttpMethod {
    GET,
    POST,
    HEAD,
    UNKNOWN
};

inline const char* method_to_string(HttpMethod m) {
    switch (m) {
        case HttpMethod::GET:    return "GET";
        case HttpMethod::POST:   return "POST";
        case HttpMethod::HEAD:   return "HEAD";
        case HttpMethod::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

inline HttpMethod string_to_method(const std::string& s) {
    if (s == "GET")  return HttpMethod::GET;
    if (s == "POST") return HttpMethod::POST;
    if (s == "HEAD") return HttpMethod::HEAD;
    return HttpMethod::UNKNOWN;
}

struct HttpRequest {
    HttpMethod method = HttpMethod::UNKNOWN;
    std::string method_string;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string get_header(const std::string& name) const {
        // Case-insensitive lookup
        std::string lower_name = name;
        for (auto& c : lower_name) c = static_cast<char>(std::tolower(c));

        for (const auto& [key, value] : headers) {
            std::string lower_key = key;
            for (auto& c : lower_key) c = static_cast<char>(std::tolower(c));
            if (lower_key == lower_name) return value;
        }
        return "";
    }
};

} // namespace server

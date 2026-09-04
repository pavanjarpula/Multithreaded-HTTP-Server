#pragma once

#include <string>
#include <unordered_map>

namespace server {

enum class StatusCode : int {
    OK                     = 200,
    BAD_REQUEST            = 400,
    FORBIDDEN              = 403,
    NOT_FOUND              = 404,
    METHOD_NOT_ALLOWED     = 405,
    PAYLOAD_TOO_LARGE      = 413,
    INTERNAL_ERROR         = 500,
    NOT_IMPLEMENTED        = 501,
    SERVICE_UNAVAILABLE    = 503
};

inline const char* reason_phrase(StatusCode code) {
    switch (code) {
        case StatusCode::OK:                 return "OK";
        case StatusCode::BAD_REQUEST:        return "Bad Request";
        case StatusCode::FORBIDDEN:          return "Forbidden";
        case StatusCode::NOT_FOUND:          return "Not Found";
        case StatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case StatusCode::PAYLOAD_TOO_LARGE:  return "Payload Too Large";
        case StatusCode::INTERNAL_ERROR:     return "Internal Server Error";
        case StatusCode::NOT_IMPLEMENTED:    return "Not Implemented";
        case StatusCode::SERVICE_UNAVAILABLE: return "Service Unavailable";
    }
    return "Unknown";
}

class HttpResponse {
public:
    HttpResponse() : status_code_(StatusCode::OK) {}

    HttpResponse(StatusCode code, const std::string& body = "")
        : status_code_(code) {
        set_body(body);
    }

    void set_status(StatusCode code) { status_code_ = code; }

    void set_header(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

    void set_body(const std::string& body) {
        body_ = body;
        set_header("Content-Length", std::to_string(body.size()));
    }

    void set_content_type(const std::string& type) {
        set_header("Content-Type", type);
    }

    std::string serialize() const {
        std::string response;
        response.reserve(512 + body_.size());

        response += "HTTP/1.1 ";
        response += std::to_string(static_cast<int>(status_code_));
        response += " ";
        response += reason_phrase(status_code_);
        response += "\r\n";

        for (std::unordered_map<std::string, std::string>::const_iterator it = headers_.begin();
             it != headers_.end(); ++it) {
            response += it->first;
            response += ": ";
            response += it->second;
            response += "\r\n";
        }

        response += "\r\n";
        response += body_;
        return response;
    }

    StatusCode status_code() const { return status_code_; }

    // Get headers map (for HEAD response - return status/headers without body)
    std::string headers_map_get() const {
        std::string result;
        for (std::unordered_map<std::string, std::string>::const_iterator it = headers_.begin();
             it != headers_.end(); ++it) {
            result += it->first + ": " + it->second + "\n";
        }
        return result;
    }

    static HttpResponse ok(const std::string& body = "", const std::string& content_type = "text/html") {
        HttpResponse resp(StatusCode::OK, body);
        resp.set_content_type(content_type);
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse bad_request(const std::string& body = "Bad Request") {
        HttpResponse resp(StatusCode::BAD_REQUEST, body);
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse forbidden(const std::string& body = "Forbidden") {
        HttpResponse resp(StatusCode::FORBIDDEN, body);
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse not_found(const std::string& body = "Not Found") {
        HttpResponse resp(StatusCode::NOT_FOUND, body);
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse method_not_allowed(const std::string& allow = "") {
        HttpResponse resp(StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed");
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        if (!allow.empty()) {
            resp.set_header("Allow", allow);
        }
        return resp;
    }

    static HttpResponse payload_too_large() {
        HttpResponse resp(StatusCode::PAYLOAD_TOO_LARGE, "Payload Too Large");
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse internal_error(const std::string& body = "Internal Server Error") {
        HttpResponse resp(StatusCode::INTERNAL_ERROR, body);
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse not_implemented() {
        HttpResponse resp(StatusCode::NOT_IMPLEMENTED, "Not Implemented");
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        return resp;
    }

    static HttpResponse service_unavailable() {
        HttpResponse resp(StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable");
        resp.set_content_type("text/plain");
        resp.set_header("Connection", "close");
        resp.set_header("Retry-After", "5");
        return resp;
    }

private:
    StatusCode status_code_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

} // namespace server

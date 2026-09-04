#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "static_file_handler.hpp"

#include <functional>
#include <string>
#include <unordered_map>

namespace server {

class Router {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    explicit Router(std::string document_root = "public")
        : document_root_(std::move(document_root))
        , static_handler_(document_root_) {}

    void add_route(const std::string& method, const std::string& path, Handler handler);

    HttpResponse handle(const HttpRequest& req);

    void setup_default_routes();

private:
    std::string document_root_;
    std::unordered_map<std::string,
        std::unordered_map<std::string, Handler>> routes_;
    StaticFileHandler static_handler_;
};

} // namespace server

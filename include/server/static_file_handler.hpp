#pragma once

#include "http_response.hpp"

#include <string>
#include <optional>
#include <filesystem>

namespace server {

class StaticFileHandler {
public:
    explicit StaticFileHandler(std::string document_root);

    std::optional<HttpResponse> serve(const std::string& url_path);

    static std::string mime_type(const std::string& extension);

private:
    std::string document_root_;

    static std::string url_decode(const std::string& str);
    static std::string normalize_path(const std::string& path);
};

} // namespace server

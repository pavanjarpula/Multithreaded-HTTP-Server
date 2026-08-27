#include "server/static_file_handler.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

namespace server {

StaticFileHandler::StaticFileHandler(std::string document_root)
    : document_root_(std::move(document_root)) {
    std::error_code ec;
    auto canonical = fs::weakly_canonical(document_root_, ec);
    if (!ec) {
        document_root_ = canonical.string();
    }
}

std::optional<HttpResponse> StaticFileHandler::serve(const std::string& url_path) {
    std::string decoded = url_decode(url_path);
    std::string normalized = normalize_path(decoded);

    if (normalized.empty()) {
        normalized = "index.html";
    }

    fs::path file_path = fs::path(document_root_) / normalized;

    std::error_code ec;
    auto canonical = fs::canonical(file_path, ec);
    if (ec) {
        return std::nullopt;
    }

    auto root_canonical = fs::canonical(document_root_, ec);
    if (ec) {
        return std::nullopt;
    }

    std::string root_str = root_canonical.string();
    std::string file_str = canonical.string();

    if (!root_str.empty() && root_str.back() != fs::path::preferred_separator) {
        root_str += fs::path::preferred_separator;
    }

    if (file_str.find(root_str) != 0) {
        return HttpResponse::forbidden("403 Forbidden");
    }

    if (!fs::is_regular_file(canonical)) {
        return std::nullopt;
    }

    std::ifstream ifs(canonical, std::ios::binary);
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    std::string ext = canonical.extension().string();
    std::string content_type = mime_type(ext);

    HttpResponse resp = HttpResponse::ok(content, content_type);
    resp.set_header("Connection", "close");
    return resp;
}

std::string StaticFileHandler::mime_type(const std::string& extension) {
    std::string ext = extension;
    for (auto& c : ext) c = static_cast<char>(std::tolower(c));

    if (ext == ".html" || ext == ".htm")  return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf")  return "font/ttf";
    if (ext == ".pdf")  return "application/pdf";
    if (ext == ".xml")  return "application/xml";

    return "application/octet-stream";
}

std::string StaticFileHandler::url_decode(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (std::size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int value = 0;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> value) {
                result += static_cast<char>(value);
                i += 2;
            } else {
                result += str[i];
            }
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string StaticFileHandler::normalize_path(const std::string& path) {
    std::string safe = path;
    safe.erase(std::remove(safe.begin(), safe.end(), '\0'), safe.end());
    std::replace(safe.begin(), safe.end(), '\\', '/');

    while (!safe.empty() && safe.front() == '/') {
        safe.erase(safe.begin());
    }

    std::vector<std::string> parts;
    std::string current;
    for (std::size_t i = 0; i <= safe.size(); ++i) {
        if (i == safe.size() || safe[i] == '/') {
            if (!current.empty() && current != ".") {
                if (current == "..") {
                    if (!parts.empty()) parts.pop_back();
                } else {
                    parts.push_back(current);
                }
            }
            current.clear();
        } else {
            current += safe[i];
        }
    }

    std::string result;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) result += '/';
        result += parts[i];
    }

    return result;
}

} // namespace server

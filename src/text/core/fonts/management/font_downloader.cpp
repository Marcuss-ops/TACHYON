#include "tachyon/text/fonts/management/font_downloader.h"
#include "tachyon/text/fonts/management/font_manifest.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")
#endif

namespace tachyon::text {

namespace {

#ifdef _WIN32
// Convert UTF-8 string to wide string
std::wstring utf8_to_wide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring wide(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), wide.data(), size_needed);
    return wide;
}

// Convert wide string to UTF-8 string
std::string wide_to_utf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), utf8.data(), size_needed, NULL, NULL);
    return utf8;
}

std::optional<std::string> winhttp_get(const std::string& url) {
    std::wstring url_wide = utf8_to_wide(url);
    URL_COMPONENTSW components = {0};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = (DWORD)-1;
    components.dwHostNameLength = (DWORD)-1;
    components.dwUrlPathLength = (DWORD)-1;
    components.dwExtraInfoLength = (DWORD)-1;

    if (!InternetCrackUrlW(url_wide.c_str(), (DWORD)url_wide.length(), 0, &components)) {
        return std::nullopt;
    }

    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0) {
        path += std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    HINTERNET hSession = InternetOpenW(L"TachyonFontDownloader/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return std::nullopt;

    INTERNET_PORT port = components.nPort;
    HINTERNET hConnect = InternetConnectW(hSession, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    const wchar_t* accept_types[] = {L"text/css", L"application/font-ttf", L"application/octet-stream", nullptr};
    DWORD flags = (components.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) | INTERNET_FLAG_RELOAD;
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", path.c_str(), NULL, NULL, accept_types, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_size, NULL);
    if (status_code != 200) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    std::string response;
    char buffer[4096];
    DWORD bytes_read;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
        response.append(buffer, bytes_read);
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);

    return response;
}

std::optional<std::vector<std::byte>> winhttp_download_binary(const std::string& url) {
    std::wstring url_wide = utf8_to_wide(url);
    URL_COMPONENTSW components = {0};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = (DWORD)-1;
    components.dwHostNameLength = (DWORD)-1;
    components.dwUrlPathLength = (DWORD)-1;
    components.dwExtraInfoLength = (DWORD)-1;

    if (!InternetCrackUrlW(url_wide.c_str(), (DWORD)url_wide.length(), 0, &components)) {
        return std::nullopt;
    }

    std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0) {
        path += std::wstring(components.lpszExtraInfo, components.dwExtraInfoLength);
    }

    HINTERNET hSession = InternetOpenW(L"TachyonFontDownloader/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hSession) return std::nullopt;

    INTERNET_PORT port = components.nPort;
    HINTERNET hConnect = InternetConnectW(hSession, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    DWORD flags = (components.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) | INTERNET_FLAG_RELOAD;
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"GET", path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    DWORD status_code = 0;
    DWORD status_size = sizeof(status_code);
    HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &status_size, NULL);
    if (status_code != 200) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return std::nullopt;
    }

    std::vector<std::byte> response;
    char buffer[4096];
    DWORD bytes_read;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytes_read) && bytes_read > 0) {
        size_t start = response.size();
        response.resize(start + bytes_read);
        std::memcpy(response.data() + start, buffer, bytes_read);
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hSession);

    return response;
}
#endif

} // anonymous namespace

std::string FontDownloader::build_google_fonts_url(const std::string& family, const std::vector<std::uint32_t>& weights, const std::vector<std::string>& subsets) {
    std::string url = "https://fonts.googleapis.com/css2?family=";
    std::string family_encoded = url_encode(family);
    url += family_encoded;

    if (!weights.empty()) {
        url += ":wght@";
        for (size_t i = 0; i < weights.size(); ++i) {
            if (i > 0) url += ',';
            url += std::to_string(weights[i]);
        }
    }

    if (!subsets.empty() && subsets[0] != "latin") {
        url += "&subset=";
        for (size_t i = 0; i < subsets.size(); ++i) {
            if (i > 0) url += ',';
            url += subsets[i];
        }
    }

    return url;
}

std::string FontDownloader::url_encode(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            char hex[4];
            std::snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
            result += hex;
        }
    }
    return result;
}

std::optional<std::string> FontDownloader::fetch_css(const std::string& family, const std::vector<std::uint32_t>& weights, const std::vector<std::string>& subsets) {
    std::string url = build_google_fonts_url(family, weights, subsets);
#ifdef _WIN32
    return winhttp_get(url);
#else
    (void)url;
    return std::nullopt;
#endif
}

std::vector<std::string> FontDownloader::parse_font_urls(const std::string& css) {
    std::vector<std::string> urls;
    size_t pos = 0;

    while ((pos = css.find("url(", pos)) != std::string::npos) {
        size_t start = pos + 4;
        if (start >= css.length()) break;

        char quote = css[start];
        bool has_quote = (quote == '\'' || quote == '"');
        size_t end;

        if (has_quote) {
            end = css.find(quote, start + 1);
            start++;
        } else {
            end = css.find(')', start);
        }

        if (end == std::string::npos) break;

        std::string url = css.substr(start, end - start);
        if (!url.empty()) {
            urls.push_back(url);
        }

        pos = end + 1;
    }

    return urls;
}

std::optional<std::vector<std::byte>> FontDownloader::download_file(const std::string& url) {
#ifdef _WIN32
    return winhttp_download_binary(url);
#else
    (void)url;
    return std::nullopt;
#endif
}

std::string FontDownloader::generate_font_id(const std::string& family, std::uint32_t weight) {
    std::string id = sanitize_filename(family);
    id += "-" + std::to_string(weight);
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    return id;
}

std::string FontDownloader::sanitize_filename(const std::string& input) {
    std::string result;
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
            result += c;
        } else if (c == ' ') {
            result += '-';
        }
    }
    return result;
}

FontFetchResult FontDownloader::fetch(const FontFetchRequest& request, ProgressCallback callback) {
    FontFetchResult result;

    if (callback) callback(request.family, 0, "Fetching CSS from Google Fonts...");

    auto css_opt = fetch_css(request.family, request.weights, request.subsets);
    if (!css_opt.has_value()) {
        result.success = false;
        result.error_message = "Failed to fetch CSS from Google Fonts API";
        return result;
    }

    std::string css = *css_opt;
    auto font_urls = parse_font_urls(css);

    if (font_urls.empty()) {
        result.success = false;
        result.error_message = "No font URLs found in Google Fonts CSS response";
        return result;
    }

    std::filesystem::create_directories(request.dest);

    for (size_t i = 0; i < font_urls.size() && i < request.weights.size(); ++i) {
        std::uint32_t weight = request.weights[i % request.weights.size()];
        const std::string& url = font_urls[i];

        if (callback) callback(request.family, weight, "Downloading font...");

        auto font_data = download_file(url);
        if (!font_data.has_value()) {
            result.success = false;
            result.error_message = "Failed to download font file from: " + url;
            return result;
        }

        std::string font_id = generate_font_id(request.family, weight);
        std::filesystem::path font_path = request.dest / (font_id + ".ttf");

        if (std::filesystem::exists(font_path) && !request.overwrite) {
            if (callback) callback(request.family, weight, "Skipped (already exists)");
        } else {
            std::ofstream out(font_path, std::ios::binary);
            out.write(reinterpret_cast<const char*>(font_data->data()), font_data->size());
            out.close();

            if (callback) callback(request.family, weight, "Downloaded successfully");
        }

        FontManifestEntry entry;
        entry.id = font_id;
        entry.family = request.family;
        entry.src = std::filesystem::absolute(font_path).string();
        entry.weight = weight;
        entry.style = request.style;
        entry.subsets = request.subsets;

        result.downloaded_fonts.push_back(entry);
    }

    result.success = true;
    result.manifest_path = request.dest.parent_path() / "font_manifest.json";

    if (callback) callback(request.family, 0, "Done!");
    return result;
}

} // namespace tachyon::text

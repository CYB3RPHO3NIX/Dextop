#ifndef JSON_UTILS_HPP
#define JSON_UTILS_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <filesystem>

// Reads a JSON file into a nlohmann::json object. Returns true on success.
inline bool read_json_file(const std::string& path, nlohmann::json& j) {
    std::ifstream in(path);
    if (!in.is_open()) return false;
    try {
        in >> j;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

// Writes a nlohmann::json object to a file. Returns true on success.
inline bool write_json_file(const std::string& path, const nlohmann::json& j) {
    // Ensure parent directory exists.
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(p.parent_path(), ec);
            // If directory creation failed, proceed - ofstream will fail to open and function will return false.
        }
    } catch (const std::exception&) {
        // Ignore filesystem exceptions; will detect failure when opening the file.
    }

    std::ofstream out(path);
    if (!out.is_open()) return false;
    try {
        out << j.dump(4); // Pretty print with 4 spaces
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

#endif // JSON_UTILS_HPP

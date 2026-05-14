//
// Created by yves-dylane on 5/14/26.
//

#ifndef GAME1_FILEUTILS_H
#define GAME1_FILEUTILS_H

#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace FileUtils {

    // Get all PNG files in a folder (non-recursive)
    inline std::vector<std::string> getPNGsInFolder(const std::string& folderPath) {
        std::vector<std::string> result;
        try {
            for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
                if (!entry.is_regular_file()) continue;
                std::string ext = entry.path().extension().string();
                // case-insensitive check
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".png")
                    result.push_back(entry.path().string());
            }
            std::sort(result.begin(), result.end()); // alphabetical
        } catch (...) {}
        return result;
    }

    // Extract filename without extension
    inline std::string stemName(const std::string& path) {
        return std::filesystem::path(path).stem().string();
    }
}

#endif //GAME1_FILEUTILS_H
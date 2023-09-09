//
// Created by edvas on 9/9/23.
//

#ifndef WGA_GEOMETRY_HPP
#define WGA_GEOMETRY_HPP

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace wga::geometry {
    // https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/input-geometry/loading-from-file.html
    bool load(const std::filesystem::path &path,
              std::vector<float> &pointData, std::vector<uint32_t> &indexData) {
        std::ifstream file(path);
        if (!file.is_open()) {
            return false;
        }

        pointData.clear();
        indexData.clear();

        enum class Section {
            None,
            Points,
            Indices,
        };
        Section currentSection = Section::None;

        float value;
        uint16_t index;
        std::string line;
        while (!file.eof()) {
            getline(file, line);

            // overcome the `CRLF` problem
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line == "[points]") {
                currentSection = Section::Points;
            } else if (line == "[indices]") {
                currentSection = Section::Indices;
            } else if (line[0] == '#' || line.empty()) {
                // Do nothing, this is a comment
            } else if (currentSection == Section::Points) {
                std::istringstream iss(line);
                // Get x, y, r, g, b
                for (int i = 0; i < 5; ++i) {
                    iss >> value;
                    pointData.push_back(value);
                }
            } else if (currentSection == Section::Indices) {
                std::istringstream iss(line);
                // Get corners #0 #1 and #2
                for (int i = 0; i < 3; ++i) {
                    iss >> index;
                    indexData.push_back(index);
                }
            }
        }
        return true;
    }
}

#endif //WGA_GEOMETRY_HPP

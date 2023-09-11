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

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace wga::geometry {
    // https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/input-geometry/loading-from-file.html
    bool load(const std::filesystem::path &path, std::vector<float> &pointData,
              std::vector<uint32_t> &indexData, int dimensions) {
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
                // Get x, y, [z], r, g, b
                for (int i = 0; i < dimensions + 3; ++i) {
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

    // https://eliemichel.github.io/LearnWebGPU/basic-3d-rendering/3d-meshes/loading-from-file.html
    bool load_obj(const std::filesystem::path& path, std::vector<wga::shader_type::vertex_attributes>& vertex_data)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

        if(!warn.empty())
        {
            std::clog << warn << '\n';
        }

        if(!err.empty())
        {
            std::clog << err << '\n';
        }

        if(!ret)
        {
            return false;
        }

        if(shapes.empty())
        {
            std::clog << "No shapes in file!\n";
        }

        vertex_data.clear();
        for(const auto& shape : shapes) {
            const std::size_t offset = vertex_data.size();
            const std::size_t size = shape.mesh.indices.size();
            vertex_data.resize(offset + size);

            for (std::size_t i = 0; i < size; ++i) {
                const auto &idx = shape.mesh.indices[i];

                // +X+Y+Z => +X-Z+Y
                const auto vi = static_cast<std::size_t>(idx.vertex_index);
                vertex_data[offset + i].position = {
                        attrib.vertices[3 * vi + 0],
                        -attrib.vertices[3 * vi + 2],
                        attrib.vertices[3 * vi + 1]
                };

                const auto ni = static_cast<std::size_t>(idx.normal_index);
                vertex_data[offset + i].normal = {
                        attrib.normals[3 * ni + 0],
                        -attrib.normals[3 * ni + 2],
                        attrib.normals[3 * ni + 1]
                };

                const auto ci = vi;
                vertex_data[offset + i].color = {
                        attrib.colors[3 * ci + 0],
                        attrib.colors[3 * ci + 1],
                        attrib.colors[3 * ci + 2]
                };
            }
        }

        return true;
    }
}

#endif //WGA_GEOMETRY_HPP

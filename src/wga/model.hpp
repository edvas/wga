//
// Created by edvas on 9/10/23.
//

#ifndef WGA_MODEL_HPP
#define WGA_MODEL_HPP

#include <filesystem>
#include <vector>

#include <wga/setup.hpp>
#include <wga/geometry/geometry.hpp>

namespace wga {
    struct model {
        wga::object<wgpu::Buffer, true> vertex_buffer;
        wga::object<wgpu::Buffer, true> index_buffer;

        std::uint32_t index_count;
        std::size_t point_data_size;
        std::size_t index_data_size;
    };

    auto create_model(wga::context &context, const std::filesystem::path &path, int dimensions) -> model {
        std::vector<float> point_data;
        std::vector<std::uint32_t> index_data;

        if (!wga::geometry::load(path, point_data, index_data, dimensions)) {
            throw std::runtime_error("Could not load geometry from file!");
        }

        wga::model model{
                wga::create_buffer(context.device, wga::bytesize(point_data),
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex),
                wga::create_buffer(context.device, wga::bytesize(index_data),
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index),
                static_cast<std::uint32_t>(index_data.size()),
                wga::bytesize(point_data),
                wga::bytesize(index_data)
        };

        context.queue.get().writeBuffer(model.vertex_buffer.get(), 0,
                                        point_data.data(), wga::bytesize(point_data));

        context.queue.get().writeBuffer(model.index_buffer.get(), 0,
                                        index_data.data(), wga::bytesize(index_data));

        return model;
    }

    struct model_obj {
        wga::object<wgpu::Buffer, true> vertex_buffer;
        std::size_t vertex_data_size;
        std::uint32_t index_count;
    };

    auto create_model_obj(wga::context &context, const std::filesystem::path &path) {
        std::vector<wga::shader_type::vertex_attributes> vertex_data;
        if (!wga::geometry::load_obj(path, vertex_data)) {
            throw std::runtime_error("Could not load geometry!");
        }

        wga::model_obj model{
                wga::create_buffer(context.device, wga::bytesize(vertex_data),
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex),
                wga::bytesize(vertex_data),
                static_cast<std::uint32_t>(vertex_data.size())
        };

        context.queue.get().writeBuffer(model.vertex_buffer.get(), 0,
                                        vertex_data.data(), wga::bytesize(vertex_data));
        return model;
    }
}

#endif //WGA_MODEL_HPP

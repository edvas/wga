#include <iostream>
#include <optional>
#include <chrono>

#include <cstdlib>

#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#ifndef WEBGPU_CPP_IMPLEMENTATION
// Removes unsused macro warning for WEBGPU_CPP_IMPLEMENTATION
#endif

#include <webgpu/webgpu.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <wga/wga.hpp>
#include <wga/setup.hpp>
#include <wga/model.hpp>

int main() {
    std::cout << "Hello, World!" << std::endl;

    try {
        wga::glfw_init glfw_init;

        static constexpr std::uint32_t width{640};
        static constexpr std::uint32_t height{480};
        auto window = wga::create_window(width, height);

        auto context = wga::setup(window, width, height, 1);

        auto MM = [] {
            float angle = 0.0f;
            auto S = glm::scale(glm::mat4x4(1.0), glm::vec3(0.3f));
            auto T = glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, 0.0));
            auto R = glm::rotate(glm::mat4x4(1.0), angle, glm::vec3(0.0, 0.0, 1.0));
            return R * T * S;
        }();

        auto VM = [] {
            float angle = 3.0f * glm::pi<float>() / 4.0f;
            glm::vec3 focal_point = {0.0, 0.0, -1.0};
            auto R = glm::rotate(glm::mat4x4(1.0), -angle, glm::vec3(1.0, 0.0, 0.0));
            auto T = glm::translate(glm::mat4x4(1.0), -focal_point);
            return T * R;
        }();

        auto PM = [] {

            float near = 0.001f;
            float far = 100.0f;
            float ratio = 640.0f / 480.0f;
            float focal_length = 2.0f;
            float fov = 2.0f * static_cast<float>(glm::atan(1.0f / focal_length));
            return glm::perspective(fov, ratio, near, far);

            //return glm::mat4x4(1.0f);
        }();

        wga::shader_type::uniforms uniforms{
                PM,
                VM,
                MM,
                {0.0f, 1.0f, 0.4f, 1.0f}, 1.0f};
        static_assert(sizeof(uniforms) % 16 == 0);

        context.queue.get().writeBuffer(context.uniform_buffer.get(), 0, &uniforms, sizeof(wga::shader_type::uniforms));

        wgpu::SupportedLimits supported_limits;
        context.device.get().getLimits(&supported_limits);
        std::clog << "device.maxVertexAttributes: " << supported_limits.limits.maxVertexAttributes << '\n';

        //auto model = wga::create_model(context, "../data/models/webgpu.txt", 2);
        //auto model = wga::create_model(context, "../data/models/pyramid.txt", 6);
        auto model = wga::create_model_obj(context, "../data/models/cube.obj");

        auto uniform_stride = get_uniform_buffer_stride(context.device);


        wgpu::TextureDescriptor texture_desc;
        texture_desc.dimension = wgpu::TextureDimension::_2D;
        texture_desc.format = wgpu::TextureFormat::RGBA8Unorm;
        texture_desc.mipLevelCount = 1;
        texture_desc.sampleCount = 1;
        texture_desc.size = {256, 256, 1};
        texture_desc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
        texture_desc.viewFormatCount = 0;
        texture_desc.viewFormats = nullptr;
        auto texture = wga::object<wgpu::Texture, true>{
                context.device.get().createTexture(texture_desc)};

        wgpu::TextureViewDescriptor texture_view_desc;
        texture_view_desc.aspect = wgpu::TextureAspect::All;
        texture_view_desc.baseArrayLayer = 0;
        texture_view_desc.arrayLayerCount = 1;
        texture_view_desc.baseMipLevel = 0;
        texture_view_desc.mipLevelCount = 1;
        texture_view_desc.dimension = wgpu::TextureViewDimension::_2D;
        texture_view_desc.format = texture_desc.format;
        auto texture_view = wga::object{texture.get().createView(texture_view_desc)};

        auto bind_group = wga::create_bind_group(context.device, context.uniform_buffer, context.bind_group_layout,
                                                 texture_view);

        std::vector<uint32_t> pixels(4 * texture_desc.size.width * texture_desc.size.height);
        for (std::uint32_t i = 0; i < texture_desc.size.width; ++i) {
            for (std::uint32_t j = 0; j < texture_desc.size.height; ++j) {
                std::uint32_t *p = &pixels[j * texture_desc.size.width + i];
                if constexpr (false) {
                    auto p1 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(i) << 24);
                    auto p2 = static_cast<std::uint32_t>(static_cast<std::uint8_t>(j) << 16);
                    auto p3 = static_cast<std::uint32_t>(128u << 8);
                    auto p4 = static_cast<std::uint32_t>(255u << 0);
                    *p = p1 | p2 | p3 | p4;
                } else {
                    auto p1 = static_cast<std::uint32_t>(
                            static_cast<std::uint8_t>((i / 16) % 2 == (j / 16) % 2 ? 255u : 0) << 0);
                    auto p2 = static_cast<std::uint32_t>(
                            static_cast<std::uint8_t>(((i - j) / 16) % 2 == 0 ? 255u : 0) << 8);
                    auto p3 = static_cast<std::uint32_t>(
                            static_cast<std::uint8_t>(((i + j) / 16) % 2 == 0 ? 255u : 0) << 16);
                    auto p4 = static_cast<std::uint32_t>(255u << 24);
                    *p = p1 | p2 | p3 | p4;
                }
            }
        }

        wgpu::ImageCopyTexture destination;
        destination.texture = texture.get();
        destination.mipLevel = 0; // Write to mip level 0
        destination.origin = {0, 0, 0};
        destination.aspect = wgpu::TextureAspect::All;

        wgpu::TextureDataLayout source;
        source.offset = 0;
        source.bytesPerRow = 4 * texture_desc.size.width;
        source.rowsPerImage = texture_desc.size.height;

        context.queue.get().writeTexture(destination, pixels.data(), pixels.size(), source, texture_desc.size);

        auto start_time = std::chrono::steady_clock::now();
        while (!glfwWindowShouldClose(window.get()) && std::chrono::steady_clock::now() < start_time + std::chrono::seconds(5)) {
            glfwPollEvents();

            uniforms.time = static_cast<float>(glfwGetTime());
            context.queue.get().writeBuffer(context.uniform_buffer.get(), offsetof(wga::shader_type::uniforms, time),
                                            &uniforms.time,
                                            sizeof(wga::shader_type::uniforms::time));

            uniforms.model_matrix = [&uniforms] {
                float angle = uniforms.time;
                auto S = glm::scale(glm::mat4x4(1.0), glm::vec3(0.3f));
                auto T = glm::translate(glm::mat4x4(1.0), glm::vec3(0.0, 0.0, 0.0));
                auto R = glm::rotate(glm::mat4x4(1.0), angle, glm::vec3(0.0, 0.0, 1.0));
                return R * T * S;
            }();
            context.queue.get().writeBuffer(context.uniform_buffer.get(),
                                            offsetof(wga::shader_type::uniforms, model_matrix),
                                            &uniforms.model_matrix,
                                            sizeof(wga::shader_type::uniforms::model_matrix));


            auto next_texture = wga::object{context.swapchain.get().getCurrentTextureView()};
            if (!next_texture.get().operator bool()) {
                std::cerr << "Cannot acquire next swapchain texture\n";
                break;
            }

            wgpu::TextureDescriptor depth_texture_desc;
            depth_texture_desc.dimension = wgpu::TextureDimension::_2D;
            depth_texture_desc.format = context.depth_texture_format;
            depth_texture_desc.mipLevelCount = 1;
            depth_texture_desc.sampleCount = 1;
            depth_texture_desc.size = {width, height, 1};
            depth_texture_desc.usage = wgpu::TextureUsage::RenderAttachment;
            depth_texture_desc.viewFormatCount = 1;
            depth_texture_desc.viewFormats = reinterpret_cast<WGPUTextureFormat *>(&context.depth_texture_format);
            auto depth_texture = wga::object<wgpu::Texture, true>{
                    context.device.get().createTexture(depth_texture_desc)};

            wgpu::TextureViewDescriptor depth_texture_view_desc;
            depth_texture_view_desc.aspect = wgpu::TextureAspect::DepthOnly;
            depth_texture_view_desc.baseArrayLayer = 0;
            depth_texture_view_desc.arrayLayerCount = 1;
            depth_texture_view_desc.baseMipLevel = 0;
            depth_texture_view_desc.mipLevelCount = 1;
            depth_texture_view_desc.dimension = wgpu::TextureViewDimension::_2D;
            depth_texture_view_desc.format = context.depth_texture_format;
            auto depth_texture_view = wga::object{depth_texture.get().createView(depth_texture_view_desc)};

            wgpu::CommandEncoderDescriptor encoder_descriptor = {};
            encoder_descriptor.nextInChain = nullptr;
            encoder_descriptor.label = "My command encoder";
            auto encoder = wga::object{
                    context.device.get().createCommandEncoder(encoder_descriptor)};

            wgpu::RenderPassColorAttachment render_pass_color_attachment = {};
            render_pass_color_attachment.view = next_texture.get();
            render_pass_color_attachment.resolveTarget = nullptr;
            render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
            render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
            render_pass_color_attachment.clearValue = wgpu::Color{0.9, 0.1, 0.2, 1.0};

            wgpu::RenderPassDepthStencilAttachment render_pass_depth_stencil_attachment;
            render_pass_depth_stencil_attachment.view = depth_texture_view.get();
            render_pass_depth_stencil_attachment.depthClearValue = 1.0f;
            render_pass_depth_stencil_attachment.depthLoadOp = wgpu::LoadOp::Clear;
            render_pass_depth_stencil_attachment.depthStoreOp = wgpu::StoreOp::Store;
            render_pass_depth_stencil_attachment.depthReadOnly = false;
            render_pass_depth_stencil_attachment.stencilClearValue = 0;
            render_pass_depth_stencil_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
            render_pass_depth_stencil_attachment.stencilStoreOp = wgpu::StoreOp::Store;
            render_pass_depth_stencil_attachment.stencilReadOnly = true;

            wgpu::RenderPassDescriptor render_pass_desc = {};
            render_pass_desc.nextInChain = nullptr;
            render_pass_desc.colorAttachmentCount = 1;
            render_pass_desc.colorAttachments = &render_pass_color_attachment;
            render_pass_desc.depthStencilAttachment = &render_pass_depth_stencil_attachment;
            render_pass_desc.timestampWriteCount = 0;
            render_pass_desc.timestampWrites = nullptr;
            auto render_pass = wga::object{encoder.get().beginRenderPass(render_pass_desc)};

            render_pass.get().setPipeline(context.pipeline.get());

            render_pass.get().setVertexBuffer(0, model.vertex_buffer.get(), 0, model.vertex_data_size);

            uint32_t dynamic_offset = 0 * uniform_stride;
            render_pass.get().setBindGroup(0, bind_group.get(), 1, &dynamic_offset);
            render_pass.get().draw(model.index_count, 1, 0, 0);

            //dynamic_offset = 1 * uniform_stride;
            //render_pass.get().setBindGroup(0, context.bind_group.get(), 1, &dynamic_offset);
            //render_pass.get().drawIndexed(model.index_count, 1, 0, 0, 0);

            render_pass.get().end();

            wgpu::CommandBufferDescriptor command_buffer_desc = {};
            command_buffer_desc.nextInChain = nullptr;
            command_buffer_desc.label = "Command buffer";
            auto command = wga::object{encoder.get().finish(command_buffer_desc)};

            context.queue.get().submit(1, &command.get());

            context.swapchain.get().present();
        }

    } catch (const std::exception &exception) {
        std::cerr << "Exception: " << exception.what() << '\n';
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Unknown exception\n";
        //throw;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

#include <iostream>
#include <optional>

#include <cstdlib>

#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#ifndef WEBGPU_CPP_IMPLEMENTATION
// Removes unsused macro warning for WEBGPU_CPP_IMPLEMENTATION
#endif

#include <webgpu/webgpu.hpp>

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

        auto context = wga::setup(window, width, height, 2);

        // Note: member alignment
        wga::uniforms uniforms{{0.0f, 1.0f, 0.4f, 1.0f}, 1.0f};
        static_assert(sizeof(uniforms) % 16 == 0);

        context.queue.get().writeBuffer(context.uniform_buffer.get(), 0, &uniforms, sizeof(wga::uniforms));

        wgpu::SupportedLimits supported_limits;
        context.device.get().getLimits(&supported_limits);
        std::clog << "device.maxVertexAttributes: " << supported_limits.limits.maxVertexAttributes << '\n';

        //auto model = wga::create_model(context, "../data/models/webgpu.txt", 2);
        auto model = wga::create_model(context, "../data/models/pyramid.txt", 3);

        auto uniform_stride = get_uniform_buffer_stride(context.device);

        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();

            uniforms.time = static_cast<float>(glfwGetTime());
            context.queue.get().writeBuffer(context.uniform_buffer.get(), offsetof(wga::uniforms, time), &uniforms.time,
                                            sizeof(wga::uniforms::time));

            uniforms.color = {1.0f, 0.5f, 0.0f, 1.0f};
            context.queue.get().writeBuffer(context.uniform_buffer.get(), offsetof(wga::uniforms, time), &uniforms.time,
                                            sizeof(wga::uniforms::time));

            uniforms.time = static_cast<float>(glfwGetTime()) - 1.0f;
            uniforms.color = {0.0f, 0.5f, 1.0f, 1.0f};
            context.queue.get().writeBuffer(context.uniform_buffer.get(), uniform_stride, &uniforms,
                                            sizeof(wga::uniforms));


            auto next_texture = wga::object{context.swapchain.get().getCurrentTextureView()};
            if (!next_texture.get().operator bool()) {
                std::cerr << "Cannot acquire next swapchain texture\n";
                break;
            }

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

            wgpu::RenderPassDescriptor render_pass_desc = {};
            render_pass_desc.nextInChain = nullptr;
            render_pass_desc.colorAttachmentCount = 1;
            render_pass_desc.colorAttachments = &render_pass_color_attachment;
            render_pass_desc.depthStencilAttachment = nullptr;
            render_pass_desc.timestampWriteCount = 0;
            render_pass_desc.timestampWrites = nullptr;
            auto render_pass = wga::object{encoder.get().beginRenderPass(render_pass_desc)};

            render_pass.get().setPipeline(context.pipeline.get());
            render_pass.get().setVertexBuffer(0, model.vertex_buffer.get(), 0, model.point_data_size);
            render_pass.get().setIndexBuffer(model.index_buffer.get(), wgpu::IndexFormat::Uint32, 0,
                                             model.index_data_size);
            uint32_t dynamic_offset = 0 * uniform_stride;
            render_pass.get().setBindGroup(0, context.bind_group.get(), 1, &dynamic_offset);
            render_pass.get().drawIndexed(model.index_count, 1, 0, 0, 0);

            dynamic_offset = 1 * uniform_stride;
            render_pass.get().setBindGroup(0, context.bind_group.get(), 1, &dynamic_offset);
            render_pass.get().drawIndexed(model.index_count, 1, 0, 0, 0);

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

#include <iostream>
#include <optional>
#include <vector>

#include <cstdlib>
#include <cstdint>

#include <GLFW/glfw3.h>

#define WEBGPU_CPP_IMPLEMENTATION
#ifndef WEBGPU_CPP_IMPLEMENTATION
// Removes unsused macro warning for WEBGPU_CPP_IMPLEMENTATION
#endif

#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

#include <wga/wga.hpp>
#include <wga/geometry/geometry.hpp>


int main() {
    std::cout << "Hello, World!" << std::endl;

    try {
        wga::glfw_init glfw_init;

        static constexpr int width{640};
        static constexpr int height{480};
        auto window = wga::create_window(width, height);

        auto instance = wga::create_instance();
        std::clog << "WGPU instance: " << instance.get() << '\n';

        auto surface = wga::create_surface(instance, window.get());

        auto adapter = wga::request_adapter(instance, surface);
        std::clog << "Got adapter: " << adapter.get() << '\n';

        auto adapter_features = wga::get_adapter_features(adapter);
        if constexpr (false) {
            std::clog << "Adapter features:\n";
            for (const auto &feature: adapter_features) {
                std::clog << "\t" << feature << '\n';
            }
        }

        auto device = wga::get_device(adapter);
        std::clog << "Got device: " << device.get() << '\n';

        wgpu::SupportedLimits supportedLimits;
        adapter.get().getLimits(&supportedLimits);
        std::cout << "adapter.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

        device.get().getLimits(&supportedLimits);
        std::cout << "device.maxVertexAttributes: " << supportedLimits.limits.maxVertexAttributes << std::endl;

        auto queue = wga::object<wgpu::Queue>{device.get().getQueue()};

        auto on_queue_work_done = [](wgpu::QueueWorkDoneStatus status) {
            std::clog << "Queue work finished with status: " << status << '\n';
        };

        queue.get().onSubmittedWorkDone(on_queue_work_done);

        auto swapchain = wga::create_swapchain(surface, adapter, device, width, height);
        std::clog << "Swapchain: " << swapchain.get() << '\n';

        auto pipeline = wga::create_pipeline(surface, adapter, device);
        std::clog << "Pipeline: " << pipeline.get() << '\n';

        wgpu::SupportedLimits supported_limits;
        device.get().getLimits(&supported_limits);
        std::clog << "device.maxVertexAttributes: " << supported_limits.limits.maxVertexAttributes << '\n';

        std::vector<float> point_data;
        std::vector<std::uint32_t> index_data;

        if (!wga::geometry::load("../data/models/webgpu.txt", point_data, index_data)) {
            throw std::runtime_error("Could not load geometry from file!");
        }

        auto vertex_buffer = wga::create_buffer(device, wga::bytesize(point_data),
                                                wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex);
        queue.get().writeBuffer(vertex_buffer.get(), 0,
                                point_data.data(), wga::bytesize(point_data));

        auto index_buffer = wga::create_buffer(device, wga::bytesize(index_data),
                                               wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index);
        queue.get().writeBuffer(index_buffer.get(), 0,
                                index_data.data(), wga::bytesize(index_data));

        auto index_count = static_cast<std::uint32_t>(index_data.size());

        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();

            auto next_texture = wga::object<wgpu::TextureView>{swapchain.get().getCurrentTextureView()};
            if (!next_texture.get().operator bool()) {
                std::cerr << "Cannot acquire next swapchain texture\n";
                break;
            }

            wgpu::CommandEncoderDescriptor encoder_descriptor = {};
            encoder_descriptor.nextInChain = nullptr;
            encoder_descriptor.label = "My command encoder";
            auto encoder = wga::object<wgpu::CommandEncoder>{device.get().createCommandEncoder(encoder_descriptor)};

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
            auto render_pass = wga::object<wgpu::RenderPassEncoder>{encoder.get().beginRenderPass(render_pass_desc)};

            render_pass.get().setPipeline(pipeline.get());
            render_pass.get().setVertexBuffer(0, vertex_buffer.get(), 0, wga::bytesize(point_data));
            render_pass.get().setIndexBuffer(index_buffer.get(), wgpu::IndexFormat::Uint32, 0,
                                             wga::bytesize(index_data));

            render_pass.get().drawIndexed(index_count, 1, 0, 0, 0);

            render_pass.get().end();

            wgpu::CommandBufferDescriptor command_buffer_desc = {};
            command_buffer_desc.nextInChain = nullptr;
            command_buffer_desc.label = "Command buffer";
            auto command = wga::object<wgpu::CommandBuffer>{encoder.get().finish(command_buffer_desc)};

            queue.get().submit(1, &command.get());

            swapchain.get().present();
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

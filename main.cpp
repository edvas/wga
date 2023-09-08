#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

#include <wga/wga.hpp>

int main() {
    std::cout << "Hello, World!" << std::endl;

    try {
        wga::glfw_init glfw_init;

        static constexpr int width{640};
        static constexpr int height{480};
        auto window = wga::create_window(width, height);

        auto instance = wga::create_instance();
        std::clog << "WGPU instance: " << instance << '\n';

        auto surface = wga::create_surface(instance, window.get());

        auto adapter = wga::request_adapter(instance, surface);
        std::clog << "Got adapter: " << adapter << '\n';

        auto adapter_features = wga::get_adapter_features(adapter);
        std::clog << "Adapter features:\n";
        for (const auto &feature: adapter_features) {
            std::clog << "\t" << feature << '\n';
        }

        auto device = wga::get_device(adapter);
        std::clog << "Got device: " << device << '\n';

        auto queue = device.getQueue();

        auto on_queue_work_done = [](wgpu::QueueWorkDoneStatus status) {
            std::clog << "Queue work finished with status: " << status << '\n';
        };

        queue.onSubmittedWorkDone(on_queue_work_done);

        auto swapchain = wga::create_swapchain(surface, adapter, device, width, height);
        std::clog << "Swapchain: " << swapchain << '\n';
        //swap_chain.release();

        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();

            wgpu::TextureView next_texture = swapchain.getCurrentTextureView();
            if (!next_texture) {
                std::cerr << "Cannot acquire next swapchain texture\n";
                break;
            }
            std::clog << "next texture: " << next_texture << '\n';

            wgpu::CommandEncoderDescriptor encoder_descriptor = {};
            encoder_descriptor.nextInChain = nullptr;
            encoder_descriptor.label = "My command encoder";
            auto encoder = device.createCommandEncoder(encoder_descriptor);

            // Todo: verify that .release must be called with the c++ wrapper
#ifdef WEBGPU_BACKEND_DAWN
            encoder.release();
#endif

            wgpu::RenderPassColorAttachment render_pass_color_attachment = {};
            render_pass_color_attachment.view = next_texture;
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
            auto render_pass = encoder.beginRenderPass(render_pass_desc);
            render_pass.end();

            render_pass.release();
            next_texture.release();

            wgpu::CommandBufferDescriptor command_buffer_desc = {};
            command_buffer_desc.nextInChain = nullptr;
            command_buffer_desc.label = "Command buffer";
            auto command = encoder.finish(command_buffer_desc);
            encoder.release();

            std::clog << "Submitting command...\n";
            queue.submit(1, &command);
            command.release();

            swapchain.present();
        }

    } catch (std::exception &exception) {
        std::cerr << "Exception: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

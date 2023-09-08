#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

namespace {
    template<typename T, void(*F)(T)>
    struct deleter {
        auto operator()(T value) {
            F(value);
            std::clog << "Deleting with " << typeid(F).name() << '\n';
        }
    };

    wgpu::Adapter request_adapter(wgpu::Instance &instance, const wgpu::RequestAdapterOptions &options) {
        std::optional<wgpu::Adapter> result;
        const char *result_message;
        auto on_adapter_request_ended =
                [&result, &result_message](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter,
                                           const char *message) {
                    if (status == wgpu::RequestAdapterStatus::Success) {
                        result = {adapter};
                    } else {
                        result = std::nullopt;
                    }
                    result_message = message;
                };

        instance.requestAdapter(options, on_adapter_request_ended);

        if (!result) {
            std::cerr << "Could not get WebGPU adapter: " << result_message << '\n';
        }

        return result.value();
    }

    wgpu::Device request_device(wgpu::Adapter &adapter, const wgpu::DeviceDescriptor &descriptor) {
        std::optional<wgpu::Device> result;
        const char *result_message;
        auto on_device_request_ended =
                [&result, &result_message](wgpu::RequestDeviceStatus status, wgpu::Device device, const char *message) {
                    if (status == wgpu::RequestDeviceStatus::Success) {
                        result = device;
                    } else {
                        result = std::nullopt;
                    }
                    result_message = message;
                };

        adapter.requestDevice(descriptor, on_device_request_ended);

        if (!result) {
            std::cerr << "Could not get WebGPU adapter: " << result_message << '\n';
        }

        return result.value();
    }

}

int main() {
    std::cout << "Hello, World!" << std::endl;

    try {
        struct glfw_init {
            glfw_init() {
                if (auto r = glfwInit(); !r) {
                    static const std::string message =
                            "Could not initialize GLFW!, return code: " + std::to_string(r) + "\n";
                    std::cerr << message;
                    throw std::runtime_error(message);
                }
            }

            ~glfw_init() {
                glfwTerminate();
                std::clog << "Calling glfwTerminate \n";
            }
        } glfw_init;

        static constexpr int width{640};
        static constexpr int height{480};
        std::unique_ptr<GLFWwindow, deleter<GLFWwindow *, glfwDestroyWindow>> window([]() -> GLFWwindow * {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            auto *window = glfwCreateWindow(width, height, "WebGPU Example", nullptr, nullptr);
            if (!window) {
                static constexpr auto message = "Could not create window!\n";
                std::cerr << message;
                throw std::runtime_error(message);
            }
            return window;
        }());

        wgpu::InstanceDescriptor descriptor;
        descriptor.nextInChain = nullptr;

        wgpu::Instance instance = wgpu::createInstance(descriptor);
        if (!instance) {
            static const std::string message = "Could not create wgpu instance!\n";
            std::cerr << message;
            throw std::runtime_error(message);
        }

        std::clog << "WGPU instance: " << instance << '\n';

        wgpu::Surface surface = glfwGetWGPUSurface(instance, window.get());

        auto adapter = request_adapter(instance, WGPURequestAdapterOptions{.compatibleSurface = surface});
        std::clog << "Got adapter: " << adapter << '\n';

        auto feature_count = adapter.enumerateFeatures(nullptr);
        std::vector<wgpu::FeatureName> features(feature_count, wgpu::FeatureName::Undefined);
        adapter.enumerateFeatures(features.data());

        std::clog << "Adapter feature:\n";
        for (const auto &feature: features) {
            std::clog << "\t" << feature << '\n';
        }

        wgpu::DeviceDescriptor device_descriptor(wgpu::Default);
        device_descriptor.nextInChain = nullptr;
        device_descriptor.label = "My Device";
        device_descriptor.requiredFeaturesCount = 0;
        device_descriptor.requiredFeatures = nullptr;
        device_descriptor.defaultQueue.nextInChain = nullptr;
        device_descriptor.defaultQueue.label = "The default queue";
        auto device = request_device(adapter, device_descriptor);
        std::clog << "Got device: " << device << '\n';

        auto on_device_error = [](wgpu::ErrorType type, const char *message) {
            std::cerr << "Uncaptured device error: type " << type;
            if (message) {
                std::cerr << " (" << message << ")";
            }
            std::cerr << '\n';
        };

        device.setUncapturedErrorCallback(on_device_error);

        auto queue = device.getQueue();

        auto on_queue_work_done = [](wgpu::QueueWorkDoneStatus status) {
            std::clog << "Queue work finished with status: " << status << '\n';
        };

        queue.onSubmittedWorkDone(on_queue_work_done);

        wgpu::SwapChainDescriptor swap_chain_desc = {};
        swap_chain_desc.nextInChain = nullptr;
        swap_chain_desc.width = width;
        swap_chain_desc.height = height;
        swap_chain_desc.format = surface.getPreferredFormat(adapter);
        swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
        swap_chain_desc.presentMode = WGPUPresentMode_Fifo;

        wgpu::SwapChain swap_chain = device.createSwapChain(surface, swap_chain_desc);
        std::clog << "Swap chain: " << swap_chain << '\n';
        //swap_chain.release();

        while (!glfwWindowShouldClose(window.get())) {
            glfwPollEvents();

            wgpu::TextureView next_texture = swap_chain.getCurrentTextureView();
            if(!next_texture)
            {
                std::cerr << "Cannot acquire next swap chain texture\n";
                break;
            }
            std::clog << "next texture: " << next_texture << '\n';

            wgpu::CommandEncoderDescriptor encoder_descriptor = {};
            encoder_descriptor.nextInChain = nullptr;
            encoder_descriptor.label = "My command encoder";
            auto encoder = device.createCommandEncoder(encoder_descriptor);

            //encoder.insertDebugMarker("Do one thing");

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

            next_texture.release();

            wgpu::CommandBufferDescriptor command_buffer_desc = {};
            command_buffer_desc.nextInChain = nullptr;
            command_buffer_desc.label = "Command buffer";
            auto command = encoder.finish(command_buffer_desc);

            std::clog << "Submitting command...\n";
            queue.submit(1, &command);

            swap_chain.present();
        }

    } catch (std::exception &exception) {
        std::cerr << "Exception: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

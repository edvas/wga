#ifndef WGA_SRC_WGA_HPP
#define WGA_SRC_WGA_HPP

#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

namespace wga {
    template<typename T, void(*F)(T)>
    struct deleter {
        auto operator()(T value) {
            F(value);
            std::clog << "Deleting with " << typeid(F).name() << '\n';
        }
    };

    auto on_device_error(wgpu::ErrorType type, const char *message) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message) {
            std::cerr << " (" << message << ")";
        }
        std::cerr << '\n';
    };

    wgpu::Adapter request_adapter(wgpu::Instance &instance, wgpu::Surface &surface) {
        WGPURequestAdapterOptions options{.compatibleSurface = surface};
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
    };

    auto create_window(const int width, const int height) {
        return std::unique_ptr<GLFWwindow, deleter<GLFWwindow *, glfwDestroyWindow>>([&]() -> GLFWwindow * {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            auto *window = glfwCreateWindow(width, height, "WebGPU Example", nullptr, nullptr);
            if (!window) {
                static constexpr auto message = "Could not create window!\n";
                std::cerr << message;
                throw std::runtime_error(message);
            }
            return window;
        }());
    }

    auto create_instance() {
        wgpu::InstanceDescriptor descriptor;
        descriptor.nextInChain = nullptr;

        wgpu::Instance instance = wgpu::createInstance(descriptor);
        if (!instance) {
            static const std::string message = "Could not create wgpu instance!\n";
            std::cerr << message;
            throw std::runtime_error(message);
        }
        return instance;
    }

    auto create_surface(wgpu::Instance instance, GLFWwindow *window) {
        return wgpu::Surface{glfwGetWGPUSurface(instance, window)};
    }

    auto get_adapter_features(wgpu::Adapter &adapter) {
        auto feature_count = adapter.enumerateFeatures(nullptr);
        std::vector<wgpu::FeatureName> features(feature_count, wgpu::FeatureName::Undefined);
        adapter.enumerateFeatures(features.data());
        return features;
    }

    auto get_device(wgpu::Adapter &adapter) {
        wgpu::DeviceDescriptor device_descriptor(wgpu::Default);
        device_descriptor.nextInChain = nullptr;
        device_descriptor.label = "My Device";
        device_descriptor.requiredFeaturesCount = 0;
        device_descriptor.requiredFeatures = nullptr;
        device_descriptor.defaultQueue.nextInChain = nullptr;
        device_descriptor.defaultQueue.label = "The default queue";

        auto device = wga::request_device(adapter, device_descriptor);
        device.setUncapturedErrorCallback(on_device_error);

        return device;
    }

    auto create_swapchain(wgpu::Surface& surface, wgpu::Adapter& adapter, wgpu::Device& device, int width, int height)
    {
        wgpu::SwapChainDescriptor swapchain_desc = {};
        swapchain_desc.nextInChain = nullptr;
        swapchain_desc.width = width;
        swapchain_desc.height = height;
        swapchain_desc.format = surface.getPreferredFormat(adapter);
        swapchain_desc.usage = WGPUTextureUsage_RenderAttachment;
        swapchain_desc.presentMode = WGPUPresentMode_Fifo;

        return device.createSwapChain(surface, swapchain_desc);
    }
}

#endif
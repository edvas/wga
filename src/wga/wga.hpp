#ifndef WGA_SRC_WGA_HPP
#define WGA_SRC_WGA_HPP

#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>
#include <glm/glm.hpp>

#include <wga/type_info.hpp>
#include <wga/shaders.hpp>

namespace wga {
    template<template<typename...> class T, typename U, typename... V>
    std::size_t bytesize(const T<U, V...> &data) {
        return data.size() * sizeof(U);
    }

    template<typename T, void(*F)(T)>
    struct deleter {
        auto operator()(T value) {
            F(value);
            std::clog << "Deleting with " << wga::type_name(F) << '\n';
        }
    };

    template<typename T, bool Destroyable = false, bool Logging = true>
    struct object {
        using U = wga::object<T, Destroyable, Logging>;

        explicit object(T &&t_data)
                : data{std::forward<T>(t_data)} {
            if constexpr (Logging) {
                std::clog << "wga::object<" << wga::type_name(data) << ">(&&) " << t_data << '\n';
            }
        }

        object(const U &other) = delete;

        auto operator=(const U &other) -> U & = delete;

        object(U &&other) noexcept = default;

        auto operator=(U &&other) noexcept -> U & = default;

        ~object() {
            if constexpr (Logging) {
                std::clog << "wga::object<" << wga::type_name(data) << ">~()\n";
            }

            if constexpr (Destroyable) {
                data.destroy();
            }
            data.release();
        }

        [[nodiscard]] auto &get() noexcept {
            return data;
        }

        [[maybe_unused]] [[nodiscard]] const auto &get() const noexcept {
            return data;
        }

    private:
        T data;
    };

    struct uniforms {
        glm::mat4x4 projection_matrix;
        glm::mat4x4 view_matrix;
        glm::mat4x4 model_matrix;
        glm::vec4 color;
        float time{};
        [[maybe_unused]] float padding[3]{};
    };

    auto on_device_error(wgpu::ErrorType type, const char *message) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message) {
            std::cerr << " (" << message << ")";
        }
        std::cerr << '\n';
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

    using window_t = std::unique_ptr<GLFWwindow, deleter<GLFWwindow *, glfwDestroyWindow>>;

    auto create_window(int width, int height) {
        return window_t([&]() -> GLFWwindow * {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            auto *window = glfwCreateWindow(width, height, "WebGPU Example", nullptr, nullptr);
            if (!window) {
                static constexpr auto message = "Could not create window!\n";
                std::cerr << message;
                throw std::runtime_error(message);
            }
            glfwMakeContextCurrent(window);
            return window;
        }());
    }

    auto create_instance() {
        wgpu::InstanceDescriptor descriptor;
        descriptor.nextInChain = nullptr;

        wgpu::Instance instance = wgpu::createInstance(descriptor);
        if (!instance.operator bool()) {
            static const std::string message = "Could not create wgpu instance!\n";
            std::cerr << message;
            throw std::runtime_error(message);
        }
        return wga::object<wgpu::Instance>{std::forward<wgpu::Instance>(instance)};
    }

    auto create_surface(wga::object<wgpu::Instance> &instance, GLFWwindow *window) {
        return wga::object<wgpu::Surface>{{glfwGetWGPUSurface(instance.get(), window)}};
    }

    auto request_adapter(wga::object<wgpu::Instance> &instance, wga::object<wgpu::Surface> &surface) {
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

        wgpu::RequestAdapterOptions options{};
        options.compatibleSurface = surface.get();
        instance.get().requestAdapter(options, on_adapter_request_ended);

        if (!result) {
            std::cerr << "Could not get WebGPU adapter: " << result_message << '\n';
            throw std::runtime_error("Could not get WebGPU adapter!");
        }

        return wga::object<wgpu::Adapter>{std::forward<wgpu::Adapter>(result.value())};
    }

    [[maybe_unused]] auto get_adapter_features(wga::object<wgpu::Adapter> &adapter) {
        auto feature_count = adapter.get().enumerateFeatures(nullptr);
        std::vector<wgpu::FeatureName> features(feature_count, wgpu::FeatureName::Undefined);
        adapter.get().enumerateFeatures(features.data());
        return features;
    }

    auto get_device(wga::object<wgpu::Adapter> &adapter) {
        wgpu::SupportedLimits supported_limits;
        adapter.get().getLimits(&supported_limits);

        wgpu::RequiredLimits required_limits = wgpu::Default;
        required_limits.limits.maxVertexAttributes = 2;
        required_limits.limits.maxVertexBuffers = 1;
        required_limits.limits.maxBufferSize = 15 * 5 * sizeof(float);
        required_limits.limits.maxVertexBufferArrayStride = 6 * sizeof(float);
        required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
        required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
        required_limits.limits.maxInterStageShaderComponents = 3;
        required_limits.limits.maxBindGroups = 1;
        required_limits.limits.maxUniformBuffersPerShaderStage = 1;
        required_limits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
        required_limits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1;
        required_limits.limits.maxTextureDimension1D = 480;
        required_limits.limits.maxTextureDimension2D = 640;
        required_limits.limits.maxTextureArrayLayers = 1;

        wgpu::DeviceDescriptor device_descriptor = wgpu::Default;
        device_descriptor.label = "My Device";
        device_descriptor.requiredFeaturesCount = 0;
        device_descriptor.requiredFeatures = nullptr;
        device_descriptor.defaultQueue.label = "The default queue";
        device_descriptor.requiredLimits = &required_limits;

        std::optional<wgpu::Device> result;
        const char *result_message;
        auto on_device_request_ended =
                [&result, &result_message](wgpu::RequestDeviceStatus status, wgpu::Device device,
                                           const char *message) {
                    if (status == wgpu::RequestDeviceStatus::Success) {
                        result = device;
                    } else {
                        result = std::nullopt;
                    }
                    result_message = message;
                };

        adapter.get().requestDevice(device_descriptor, on_device_request_ended);

        if (!result) {
            std::cerr << "Could not get WebGPU adapter: " << result_message << '\n';
        }

        result.value().setUncapturedErrorCallback(on_device_error);

        return wga::object<wgpu::Device>{std::forward<wgpu::Device>(result.value())};
    }

    auto get_swapchain_format(wga::object<wgpu::Surface> &surface, wga::object<wgpu::Adapter> &adapter) {
        return surface.get().getPreferredFormat(adapter.get());
    }

    auto create_swapchain(wga::object<wgpu::Surface> &surface, wga::object<wgpu::Adapter> &adapter,
                          wga::object<wgpu::Device> &device,
                          std::uint32_t width, std::uint32_t height) {
        wgpu::SwapChainDescriptor swapchain_desc = wgpu::Default;
        swapchain_desc.width = width;
        swapchain_desc.height = height;
        swapchain_desc.format = get_swapchain_format(surface, adapter);
        swapchain_desc.usage = WGPUTextureUsage_RenderAttachment;
        swapchain_desc.presentMode = WGPUPresentMode_Fifo;

        return wga::object<wgpu::SwapChain>{device.get().createSwapChain(surface.get(), swapchain_desc)};
    }

    auto create_shader_module(const std::filesystem::path &path, wga::object<wgpu::Device> &device) {
        std::ifstream stream(path);
        if (!stream) {
            throw std::runtime_error("Could not load shader file: " + path.string());
        }

        std::stringstream ss;
        ss << stream.rdbuf();
        std::string source = ss.str();

        wgpu::ShaderModuleWGSLDescriptor wgsl_desc;
        wgsl_desc.chain.next = nullptr;
        wgsl_desc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        wgsl_desc.code = source.c_str();

        wgpu::ShaderModuleDescriptor shader_module_desc;
        shader_module_desc.label = "Shader module";
        shader_module_desc.nextInChain = &wgsl_desc.chain;
        shader_module_desc.hintCount = 0;
        shader_module_desc.hints = nullptr;
        std::clog << "Creating shader module\n";

        return wga::object<wgpu::ShaderModule>{device.get().createShaderModule(shader_module_desc)};
    }

    auto create_buffer(wga::object<wgpu::Device> &device, std::uint64_t size, wgpu::BufferUsageFlags usage) {
        wgpu::BufferDescriptor desc;
        desc.label = "Buffer\n";
        desc.usage = usage;
        desc.size = size;
        desc.mappedAtCreation = false;
        return wga::object<wgpu::Buffer, true>{device.get().createBuffer(desc)};
    }

    auto get_uniform_buffer_stride(wga::object<wgpu::Device> &device) {
        wgpu::SupportedLimits supported_limits;
        device.get().getLimits(&supported_limits);

        std::uint32_t step_size = supported_limits.limits.minUniformBufferOffsetAlignment;
        std::uint32_t target_size = sizeof(wga::uniforms);

        return step_size * (target_size / step_size + (target_size % step_size == 0 ? 0 : 1));
    }
}

#endif
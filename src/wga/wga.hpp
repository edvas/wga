#ifndef WGA_SRC_WGA_HPP
#define WGA_SRC_WGA_HPP

#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <type_traits>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <glfw3webgpu.h>

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
            std::clog << "Deleting with " << typeid(F).name() << '\n';
        }
    };

    template<typename T, bool Destroyable = false, bool Logging = false>
    struct object {
        explicit object(T &&t_data)
                : data{std::forward<T>(t_data)} {
            if constexpr (Logging) {
                std::clog << "wga::object<" << typeid(T).name() << ">(&&)\n";
            }
        }

        ~object() {
            if constexpr (Logging) {
                std::clog << "wga::object<" << typeid(T).name() << ">~()\n";
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

    auto create_window(const int width, const int height) {
        return std::unique_ptr<GLFWwindow, deleter<GLFWwindow *, glfwDestroyWindow>>([&]() -> GLFWwindow * {
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

    auto get_adapter_features(wga::object<wgpu::Adapter> &adapter) {
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
        required_limits.limits.maxBufferSize = 6 * 5 * sizeof(float);
        required_limits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
        required_limits.limits.minStorageBufferOffsetAlignment = supported_limits.limits.minStorageBufferOffsetAlignment;
        required_limits.limits.minUniformBufferOffsetAlignment = supported_limits.limits.minUniformBufferOffsetAlignment;
        required_limits.limits.maxInterStageShaderComponents = 3;

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

    auto create_pipeline(wga::object<wgpu::Surface> &surface, wga::object<wgpu::Adapter> &adapter,
                         wga::object<wgpu::Device> &device) {
        wgpu::ShaderModuleWGSLDescriptor wgsl_desc;
        wgsl_desc.chain.next = nullptr;
        wgsl_desc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        wgsl_desc.code = basic_color_shader_source;

        wgpu::ShaderModuleDescriptor shader_module_desc;
        shader_module_desc.label = "Shader module";
        shader_module_desc.nextInChain = &wgsl_desc.chain;
        shader_module_desc.hintCount = 0;
        shader_module_desc.hints = nullptr;
        std::clog << "Creating shader module\n";
        auto shader_module = device.get().createShaderModule(shader_module_desc);
        std::clog << "Shader module: " << shader_module << '\n';

        wgpu::BlendState blend_state;
        blend_state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        blend_state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blend_state.color.operation = wgpu::BlendOperation::Add;
        blend_state.alpha.srcFactor = wgpu::BlendFactor::Zero;
        blend_state.alpha.dstFactor = wgpu::BlendFactor::One;
        blend_state.alpha.operation = wgpu::BlendOperation::Add;

        wgpu::ColorTargetState color_target;
        color_target.format = get_swapchain_format(surface, adapter);
        color_target.blend = &blend_state;
        color_target.writeMask = wgpu::ColorWriteMask::All;

        wgpu::FragmentState fragment_state;
        fragment_state.module = shader_module;
        fragment_state.entryPoint = "fs_main";
        fragment_state.constantCount = 0;
        fragment_state.constants = nullptr;
        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target;

        wgpu::VertexAttribute vertex_pos_attrib;
        vertex_pos_attrib.shaderLocation = 0; // @location(0)
        vertex_pos_attrib.format = wgpu::VertexFormat::Float32x2;
        vertex_pos_attrib.offset = 0;

        wgpu::VertexAttribute vertex_color_attrib;
        vertex_color_attrib.shaderLocation = 1; // @location(1)
        vertex_color_attrib.format = wgpu::VertexFormat::Float32x3;
        vertex_color_attrib.offset = 2 * sizeof(float);

        std::vector<wgpu::VertexAttribute> vertex_attrib{vertex_pos_attrib, vertex_color_attrib};

        wgpu::VertexBufferLayout vertex_buffer_layout;
        vertex_buffer_layout.attributeCount = static_cast<std::uint32_t>(vertex_attrib.size());
        vertex_buffer_layout.attributes = vertex_attrib.data();
        vertex_buffer_layout.arrayStride = 5 * sizeof(float);
        vertex_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;

        wgpu::PipelineLayoutDescriptor pipeline_layout_desc = wgpu::Default;
        pipeline_layout_desc.label = "Pipeline layout";
        pipeline_layout_desc.bindGroupLayoutCount = 0;
        pipeline_layout_desc.bindGroupLayouts = nullptr;
        wgpu::PipelineLayout layout = device.get().createPipelineLayout(pipeline_layout_desc);

        wgpu::RenderPipelineDescriptor desc;
        desc.label = "Render pipeline";
        desc.vertex.bufferCount = 1;
        desc.vertex.buffers = &vertex_buffer_layout;
        desc.vertex.module = shader_module;
        desc.vertex.entryPoint = "vs_main";
        desc.vertex.constantCount = 0;
        desc.vertex.constants = nullptr;
        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        desc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        desc.primitive.frontFace = wgpu::FrontFace::CCW;
        desc.primitive.cullMode = wgpu::CullMode::None; // Todo: ::Front
        desc.fragment = &fragment_state;
        desc.depthStencil = nullptr;
        desc.multisample.count = 1;
        desc.multisample.mask = ~0u;
        desc.multisample.alphaToCoverageEnabled = false;
        desc.layout = layout;

        return wga::object<wgpu::RenderPipeline>{device.get().createRenderPipeline(desc)};
    }

    auto create_buffer(wga::object<wgpu::Device> &device, std::uint64_t size, wgpu::BufferUsageFlags usage) {
        wgpu::BufferDescriptor desc;
        desc.label = "Buffer\n";
        desc.usage = usage;
        desc.size = size;
        desc.mappedAtCreation = false;

        wga::object<wgpu::Buffer, true> buffer{device.get().createBuffer(desc)};
        return buffer;
    }
}

#endif
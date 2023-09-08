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

    auto get_swapchain_format(wgpu::Surface &surface, wgpu::Adapter &adapter) {
        return surface.getPreferredFormat(adapter);
    }

    auto create_swapchain(wgpu::Surface &surface, wgpu::Adapter &adapter, wgpu::Device &device, int width, int height) {
        wgpu::SwapChainDescriptor swapchain_desc = {};
        swapchain_desc.nextInChain = nullptr;
        swapchain_desc.width = width;
        swapchain_desc.height = height;
        swapchain_desc.format = get_swapchain_format(surface, adapter);
        swapchain_desc.usage = WGPUTextureUsage_RenderAttachment;
        swapchain_desc.presentMode = WGPUPresentMode_Fifo;

        return device.createSwapChain(surface, swapchain_desc);
    }

    static constexpr const char *triangle_shader_source = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f {
    var p = vec2f(0.0, 0.0);
    if (in_vertex_index == 0u) {
        p = vec2f(-0.5, -0.5);
    } else if (in_vertex_index == 1u) {
        p = vec2f(0.5, -0.5);
    } else {
        p = vec2f(0.0, 0.5);
    }
    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f {
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

    auto create_pipeline(wgpu::Surface &surface, wgpu::Adapter &adapter, wgpu::Device &device) {
        wgpu::ShaderModuleWGSLDescriptor wgsl_desc;
        wgsl_desc.chain.next = nullptr;
        wgsl_desc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
        wgsl_desc.code = triangle_shader_source;

        wgpu::ShaderModuleDescriptor shader_module_desc;
        shader_module_desc.nextInChain = &wgsl_desc.chain;
        shader_module_desc.hintCount = 0;
        shader_module_desc.hints = nullptr;
        auto shader_module = device.createShaderModule(shader_module_desc);

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

        wgpu::RenderPipelineDescriptor desc;
        desc.vertex.bufferCount = 0;
        desc.vertex.buffers = nullptr;
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
        desc.layout = nullptr;
        auto pipeline = device.createRenderPipeline(desc);
        return pipeline;
    }
}

#endif
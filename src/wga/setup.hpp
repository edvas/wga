//
// Created by edvas on 9/10/23.
//

#ifndef WGA_SETUP_HPP
#define WGA_SETUP_HPP

#include <memory>

#include <wga/wga.hpp>
#include <wga/callbacks.hpp>

namespace wga {
    struct context {
        wgpu::TextureFormat depth_texture_format;
        wga::object<wgpu::Instance> instance;
        wga::object<wgpu::Surface> surface;
        wga::object<wgpu::Adapter> adapter;
        wga::object<wgpu::Device> device;
        wga::object<wgpu::SwapChain> swapchain;
        wga::object<wgpu::Buffer, true> uniform_buffer;
        wga::object<wgpu::BindGroupLayout> bind_group_layout;
        wga::object<wgpu::BindGroup> bind_group;
        wga::object<wgpu::RenderPipeline> pipeline;
        wga::object<wgpu::Queue> queue;

        context() = delete;
        ~context() = default;
        context(const context&) = delete;
        context(context&&) = default;
        auto operator=(const context&) = delete;
        auto operator=(context&&) noexcept -> wga::context& = delete;
    };

    auto create_queue(wga::object<wgpu::Device> &device) {
        auto queue = device.get().getQueue();
        queue.onSubmittedWorkDone(wga::on_queue_work_done);
        return wga::object{std::forward<wgpu::Queue>(queue)};
    }

    auto create_bind_group_layout(wga::object<wgpu::Device> &device) {
        wgpu::BindGroupLayoutEntry binding_layout = wgpu::Default;
        binding_layout.binding = 0;
        binding_layout.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        binding_layout.buffer.type = wgpu::BufferBindingType::Uniform;
        binding_layout.buffer.minBindingSize = sizeof(wga::shader_type::uniforms);
        binding_layout.buffer.hasDynamicOffset = true;

        wgpu::BindGroupLayoutDescriptor bind_group_layout_desc{};
        bind_group_layout_desc.entryCount = 1;
        bind_group_layout_desc.entries = &binding_layout;
        return wga::object{device.get().createBindGroupLayout(bind_group_layout_desc)};
    }

    auto create_bind_group(wga::object<wgpu::Device> &device, wga::object<wgpu::Buffer, true> &uniform_buffer,
                           wga::object<wgpu::BindGroupLayout> &bind_group_layout) {

        wgpu::BindGroupEntry binding{};
        binding.binding = 0;
        binding.buffer = uniform_buffer.get();
        binding.offset = 0;
        binding.size = sizeof(wga::shader_type::uniforms);

        wgpu::BindGroupDescriptor bind_group_desc{};
        bind_group_desc.layout = bind_group_layout.get();
        bind_group_desc.entryCount = 1;
        bind_group_desc.entries = &binding;
        auto bind_group = device.get().createBindGroup(bind_group_desc);
        return wga::object{std::forward<wgpu::BindGroup>(bind_group)};
    }

    auto create_pipeline(wga::object<wgpu::Surface> &surface, wga::object<wgpu::Adapter> &adapter,
                         wga::object<wgpu::Device> &device,
                         wga::object<wgpu::BindGroupLayout> &bind_group_layout, wgpu::TextureFormat &format) {

        auto shader_module = wga::create_shader_module("../data/shaders/basic_color.wgsl", device);

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
        fragment_state.module = shader_module.get();
        fragment_state.entryPoint = "fs_main";
        fragment_state.constantCount = 0;
        fragment_state.constants = nullptr;
        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target;

        wgpu::VertexAttribute vertex_pos_attrib;
        vertex_pos_attrib.shaderLocation = 0; // @location(0)
        vertex_pos_attrib.format = wgpu::VertexFormat::Float32x3;
        vertex_pos_attrib.offset = offsetof(wga::shader_type::vertex_attributes, position);

        wgpu::VertexAttribute vertex_normal_attrib;
        vertex_normal_attrib.shaderLocation = 1; // @location(1)
        vertex_normal_attrib.format = wgpu::VertexFormat::Float32x3;
        vertex_normal_attrib.offset = offsetof(wga::shader_type::vertex_attributes, normal);

        wgpu::VertexAttribute vertex_color_attrib;
        vertex_color_attrib.shaderLocation = 2; // @location(2)
        vertex_color_attrib.format = wgpu::VertexFormat::Float32x3;
        vertex_color_attrib.offset = offsetof(wga::shader_type::vertex_attributes, color);

        std::vector<wgpu::VertexAttribute> vertex_attrib{
                vertex_pos_attrib,
                vertex_normal_attrib,
                vertex_color_attrib};

        wgpu::VertexBufferLayout vertex_buffer_layout;
        vertex_buffer_layout.attributeCount = static_cast<std::uint32_t>(vertex_attrib.size());
        vertex_buffer_layout.attributes = vertex_attrib.data();
        vertex_buffer_layout.arrayStride = sizeof(wga::shader_type::vertex_attributes);
        vertex_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;

        wgpu::PipelineLayoutDescriptor pipeline_layout_desc = wgpu::Default;
        pipeline_layout_desc.label = "Pipeline layout";
        pipeline_layout_desc.bindGroupLayoutCount = 1;
        pipeline_layout_desc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout *>(&bind_group_layout.get());
        wgpu::PipelineLayout layout = device.get().createPipelineLayout(pipeline_layout_desc);

        wgpu::DepthStencilState depth_stencil_state = wgpu::Default;
        depth_stencil_state.depthCompare = wgpu::CompareFunction::Less;
        depth_stencil_state.depthWriteEnabled = true;
        depth_stencil_state.format = format;
        depth_stencil_state.stencilReadMask = 0;
        depth_stencil_state.stencilWriteMask = 0;

        wgpu::RenderPipelineDescriptor desc;
        desc.label = "Render pipeline";
        desc.vertex.bufferCount = 1;
        desc.vertex.buffers = &vertex_buffer_layout;
        desc.vertex.module = shader_module.get();
        desc.vertex.entryPoint = "vs_main";
        desc.vertex.constantCount = 0;
        desc.vertex.constants = nullptr;
        desc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
        desc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;
        desc.primitive.frontFace = wgpu::FrontFace::CCW;
        desc.primitive.cullMode = wgpu::CullMode::None; // Todo: ::Front
        desc.fragment = &fragment_state;
        desc.depthStencil = &depth_stencil_state;
        desc.multisample.count = 1;
        desc.multisample.mask = ~0u;
        desc.multisample.alphaToCoverageEnabled = false;
        desc.layout = layout;

        return wga::object<wgpu::RenderPipeline>{device.get().createRenderPipeline(desc)};
    }

    auto setup(wga::window_t &window, std::uint32_t width, std::uint32_t height,
               std::uint32_t uniforms_count) -> wga::context {
        wga::context context{
                wgpu::TextureFormat::Depth24Plus,
                wga::create_instance(),
                wga::create_surface(context.instance, window.get()),
                wga::request_adapter(context.instance, context.surface),
                wga::get_device(context.adapter),
                wga::create_swapchain(context.surface, context.adapter, context.device, width, height),
                wga::create_buffer(context.device, uniforms_count * get_uniform_buffer_stride(context.device),
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform),
                wga::create_bind_group_layout(context.device),
                wga::create_bind_group(context.device, context.uniform_buffer, context.bind_group_layout),
                wga::create_pipeline(context.surface, context.adapter, context.device, context.bind_group_layout,
                                     context.depth_texture_format),
                wga::create_queue(context.device)
        };

        return context;
    }

}

#endif //WGA_SETUP_HPP

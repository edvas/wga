//
// Created by edvas on 9/10/23.
//

#ifndef WGA_SETUP_HPP
#define WGA_SETUP_HPP

#include <wga/wga.hpp>
#include <wga/callbacks.hpp>

namespace wga {
    struct context {
        wga::object<wgpu::Instance> instance;
        wga::object<wgpu::Surface> surface;
        wga::object<wgpu::Adapter> adapter;
        wga::object<wgpu::Device> device;
        wga::object<wgpu::SwapChain> swapchain;
        wga::object<wgpu::RenderPipeline> pipeline;
        wga::object<wgpu::Queue> queue;
    };

    auto create_queue(wga::object<wgpu::Device>& device)
    {
        auto queue = wga::object{device.get().getQueue()};
        queue.get().onSubmittedWorkDone(wga::on_queue_work_done);
        return queue;
    }

    auto setup(wga::window_t &window, std::uint32_t width, std::uint32_t height) -> wga::context {
        wga::context context{
                wga::create_instance(),
                wga::create_surface(context.instance, window.get()),
                wga::request_adapter(context.instance, context.surface),
                wga::get_device(context.adapter),
                wga::create_swapchain(context.surface, context.adapter, context.device, width, height),
                wga::create_pipeline(context.surface, context.adapter, context.device),
                wga::create_queue(context.device)
        };

        return context;
    }


}

#endif //WGA_SETUP_HPP

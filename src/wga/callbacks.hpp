//
// Created by edvas on 9/10/23.
//

#ifndef WGA_CALLBACKS_HPP
#define WGA_CALLBACKS_HPP

#include <iostream>

namespace wgpu {
    class QueueWorkDoneStatus;
}

namespace wga {
    auto on_queue_work_done(wgpu::QueueWorkDoneStatus status) {
        std::clog << "Queue work finished with status: " << status << '\n';
    }
}

#endif //WGA_CALLBACKS_HPP

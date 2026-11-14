// SPDX-License-Identifier: GPL-2.0-or-later
#include "ps4rt_screenshot.h"

#include <atomic>
#include <mutex>

namespace Ps4RtScreenshot {

namespace {
std::atomic<unsigned> requests{0};
std::mutex name_lock;
std::string pending_name;
}

void Request() {
    requests.fetch_add(1, std::memory_order_relaxed);
}

void RequestNamed(const char* stem) {
    std::string clean;
    for (const char* c = stem ? stem : ""; *c && clean.size() < 64; ++c) {
        const char ch = *c;
        const bool ok = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') ||
                        ch == '-' || ch == '_';
        if (ok) {
            clean += ch;
        }
    }
    {
        std::lock_guard lock{name_lock};
        pending_name = clean;
    }
    Request();
}

std::string TakeName() {
    std::lock_guard lock{name_lock};
    std::string name;
    name.swap(pending_name);
    return name;
}

unsigned Consume() {
    return requests.exchange(0, std::memory_order_acq_rel);
}

} // namespace Ps4RtScreenshot

/// Crowbar's "capture <stem>" request (src/runtime_crowbar_pause.c).
extern "C" int ps4rtgpu_capture_named(const char* stem) {
    Ps4RtScreenshot::RequestNamed(stem);
    return 0;
}

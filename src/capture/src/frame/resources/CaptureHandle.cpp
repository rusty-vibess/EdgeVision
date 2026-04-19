#include "frame/resources/CaptureHandle.hpp"

namespace edgevision::capture::frame {

    CaptureHandle::CaptureHandle(const K4aInterface& api, k4a_capture_t capture)
        : m_api(api), m_capture(capture) {}

    CaptureHandle::~CaptureHandle() {
        if (m_capture != nullptr) {
            m_api.captureRelease(m_capture);
        }
    }

    k4a_capture_t CaptureHandle::get() const {
        return m_capture;
    }

} // namespace edgevision::capture::frame

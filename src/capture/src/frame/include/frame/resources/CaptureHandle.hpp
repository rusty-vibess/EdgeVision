#pragma once

#include <k4a/k4a.h>

#include "capture/interfaces/K4aInterface.hpp"

namespace edgevision::capture::frame {

    class CaptureHandle final {
      public:
        CaptureHandle(const K4aInterface& api, k4a_capture_t capture);
        ~CaptureHandle();

        CaptureHandle(const CaptureHandle&) = delete;
        CaptureHandle& operator=(const CaptureHandle&) = delete;
        CaptureHandle(CaptureHandle&&) = delete;
        CaptureHandle& operator=(CaptureHandle&&) = delete;

        [[nodiscard]] k4a_capture_t get() const;

      private:
        const K4aInterface& m_api;
        k4a_capture_t m_capture = nullptr;
    };

} // namespace edgevision::capture::frame

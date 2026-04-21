#pragma once

#include <k4a/k4a.h>

#include "capture/interfaces/K4aInterface.hpp"

namespace edgevision::capture::frame {

    class ImageHandle final {
      public:
        ImageHandle(const K4aInterface& api, k4a_image_t image);
        ~ImageHandle();

        ImageHandle(const ImageHandle&) = delete;
        ImageHandle& operator=(const ImageHandle&) = delete;

        ImageHandle(ImageHandle&& other) noexcept;
        ImageHandle& operator=(ImageHandle&&) = delete;

        [[nodiscard]] k4a_image_t get() const;

      private:
        const K4aInterface& m_api;
        k4a_image_t m_image = nullptr;
    };

} // namespace edgevision::capture::frame

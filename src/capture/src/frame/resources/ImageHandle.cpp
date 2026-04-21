#include "frame/resources/ImageHandle.hpp"

namespace edgevision::capture::frame {

    ImageHandle::ImageHandle(const K4aInterface& api, k4a_image_t image)
        : m_api(api), m_image(image) {}

    ImageHandle::~ImageHandle() {
        if (m_image != nullptr) {
            m_api.imageRelease(m_image);
        }
    }

    ImageHandle::ImageHandle(ImageHandle&& other) noexcept
        : m_api(other.m_api), m_image(other.m_image) {
        other.m_image = nullptr;
    }

    k4a_image_t ImageHandle::get() const {
        return m_image;
    }

} // namespace edgevision::capture::frame

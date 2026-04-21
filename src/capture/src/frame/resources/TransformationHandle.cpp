#include "frame/resources/TransformationHandle.hpp"

namespace edgevision::capture::frame {

    TransformationHandle::TransformationHandle(
        const K4aInterface& api,
        k4a_transformation_t transformation
    )
        : m_api(api), m_transformation(transformation) {}

    TransformationHandle::~TransformationHandle() {
        if (m_transformation != nullptr) {
            m_api.transformationDestroy(m_transformation);
        }
    }

    k4a_transformation_t TransformationHandle::get() const {
        return m_transformation;
    }

} // namespace edgevision::capture::frame

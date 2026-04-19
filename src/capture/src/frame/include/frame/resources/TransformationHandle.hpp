#pragma once

#include <k4a/k4a.h>

#include "capture/interfaces/K4aInterface.hpp"

namespace edgevision::capture::frame {

    class TransformationHandle final {
      public:
        TransformationHandle(const K4aInterface& api, k4a_transformation_t transformation);
        ~TransformationHandle();

        TransformationHandle(const TransformationHandle&) = delete;
        TransformationHandle& operator=(const TransformationHandle&) = delete;
        TransformationHandle(TransformationHandle&&) = delete;
        TransformationHandle& operator=(TransformationHandle&&) = delete;

        [[nodiscard]] k4a_transformation_t get() const;

      private:
        const K4aInterface& m_api;
        k4a_transformation_t m_transformation = nullptr;
    };

} // namespace edgevision::capture::frame

#pragma once

#include "ITMLib/Objects/Camera/ITMRGBDCalib.h"
#include "ITMLib/Objects/Views/ITMView.h"
#include "model/frame/types/frame.hpp"

namespace edgevision::builder {

    enum class ViewConversionFailure {
        None,
        MismatchedImageSizes,
        InvalidRgbLayout,
        InvalidDepthLayout,
    };

    struct ViewConversionResult {
        ViewConversionFailure failure = ViewConversionFailure::None;

        [[nodiscard]] bool converted() const {
            return failure == ViewConversionFailure::None;
        }
    };

    [[nodiscard]] ITMLib::ITMRGBDCalib makeRgbdCalib(const edgevision::model::frame::Frame& frame);

    [[nodiscard]] ViewConversionResult copyFrameToView(
        ITMLib::ITMView& view,
        const edgevision::model::frame::Frame& frame,
        MemoryDeviceType memoryType
    );

} // namespace edgevision::builder

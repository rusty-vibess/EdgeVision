#include "builder/utils/InfiniTamViewConverter.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif

namespace edgevision::builder {
    namespace {
        constexpr int kRgbBytesPerPixel = 4;
        constexpr int kDepthBytesPerPixel = 2;
        constexpr int kNeonPixelsPerBatch = 16;
        constexpr float kMillimetresToMetres = 0.001f;

        [[nodiscard]] bool hasMatchingImageSizes(const edgevision::model::frame::Frame& frame) {
            return frame.rgb.size == frame.depth.size;
        }

        [[nodiscard]] bool hasValidRgbLayout(const edgevision::model::frame::Frame& frame) {
            return frame.rgb.strideBytes >= frame.rgb.size.width * kRgbBytesPerPixel;
        }

        [[nodiscard]] bool hasValidDepthLayout(const edgevision::model::frame::Frame& frame) {
            return frame.depth.strideBytes >= frame.depth.size.width * kDepthBytesPerPixel;
        }

        void swizzleBgraRowToRgba(
            const std::byte* sourceRow,
            Vector4u* destinationRow,
            int width
        ) {
            const auto* sourceBytes = reinterpret_cast<const std::uint8_t*>(sourceRow);
            auto* destinationBytes = reinterpret_cast<std::uint8_t*>(destinationRow);
            int column = 0;

#if defined(__aarch64__) || defined(__ARM_NEON)
            const uint8x16_t opaqueAlpha = vdupq_n_u8(0xFF);
            for (; column <= width - kNeonPixelsPerBatch; column += kNeonPixelsPerBatch) {
                const uint8x16x4_t bgra =
                    vld4q_u8(sourceBytes + static_cast<std::size_t>(column) * kRgbBytesPerPixel);

                uint8x16x4_t rgba{};
                rgba.val[0] = bgra.val[2];
                rgba.val[1] = bgra.val[1];
                rgba.val[2] = bgra.val[0];
                rgba.val[3] = opaqueAlpha;
                vst4q_u8(
                    destinationBytes + static_cast<std::size_t>(column) * kRgbBytesPerPixel, rgba
                );
            }
#endif

            for (; column < width; ++column) {
                const std::size_t sourceOffset =
                    static_cast<std::size_t>(column) * kRgbBytesPerPixel;
                destinationBytes[sourceOffset] = sourceBytes[sourceOffset + 2];
                destinationBytes[sourceOffset + 1] = sourceBytes[sourceOffset + 1];
                destinationBytes[sourceOffset + 2] = sourceBytes[sourceOffset];
                destinationBytes[sourceOffset + 3] = 0xFF;
            }
        }
    } // namespace

    ITMLib::ITMRGBDCalib makeRgbdCalib(const edgevision::model::frame::Frame& frame) {
        ITMLib::ITMRGBDCalib calib{};
        calib.intrinsics_rgb.SetFrom(
            frame.rgb.size.width,
            frame.rgb.size.height,
            frame.intrinsics.fx,
            frame.intrinsics.fy,
            frame.intrinsics.cx,
            frame.intrinsics.cy
        );
        calib.intrinsics_d.SetFrom(
            frame.depth.size.width,
            frame.depth.size.height,
            frame.intrinsics.fx,
            frame.intrinsics.fy,
            frame.intrinsics.cx,
            frame.intrinsics.cy
        );
        return calib;
    }

    ViewConversionResult copyFrameToView(
        ITMLib::ITMView& view,
        const edgevision::model::frame::Frame& frame,
        MemoryDeviceType memoryType
    ) {
        if (!hasMatchingImageSizes(frame)) {
            return ViewConversionResult{ViewConversionFailure::MismatchedImageSizes};
        }

        if (!hasValidRgbLayout(frame)) {
            return ViewConversionResult{ViewConversionFailure::InvalidRgbLayout};
        }

        if (!hasValidDepthLayout(frame)) {
            return ViewConversionResult{ViewConversionFailure::InvalidDepthLayout};
        }

        auto* rgbData = view.rgb->GetData(MEMORYDEVICE_CPU);
        const auto* rgbSource = reinterpret_cast<const std::byte*>(frame.rgb.buffer.data);
        for (int row = 0; row < frame.rgb.size.height; ++row) {
            const auto* sourceRow =
                rgbSource + static_cast<std::size_t>(row) * frame.rgb.strideBytes;
            auto* destinationRow = rgbData + row * frame.rgb.size.width;
            swizzleBgraRowToRgba(sourceRow, destinationRow, frame.rgb.size.width);
        }

        auto* depthData = view.depth->GetData(MEMORYDEVICE_CPU);
        const auto* depthSource = reinterpret_cast<const std::byte*>(frame.depth.buffer.data);
        for (int row = 0; row < frame.depth.size.height; ++row) {
            const auto* sourceRow = reinterpret_cast<const std::uint16_t*>(
                depthSource + static_cast<std::size_t>(row) * frame.depth.strideBytes
            );

            for (int column = 0; column < frame.depth.size.width; ++column) {
                depthData[row * frame.depth.size.width + column] =
                    static_cast<float>(sourceRow[column]) * kMillimetresToMetres;
            }
        }

        view.depthConfidence->Clear();
        if (memoryType == MEMORYDEVICE_CUDA) {
            view.rgb->UpdateDeviceFromHost();
            view.depth->UpdateDeviceFromHost();
            view.depthConfidence->UpdateDeviceFromHost();
        }

        return ViewConversionResult{};
    }

} // namespace edgevision::builder

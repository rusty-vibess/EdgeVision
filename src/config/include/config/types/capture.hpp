#pragma once

#include <cstdint>

namespace edgevision::config {

    enum class CaptureColorFormat {
        Bgra32,
    };

    enum class CaptureColorResolution {
        P720,
        P1080,
        P1440,
        P1536,
        P2160,
        P3072,
    };

    enum class CaptureDepthMode {
        Nfov2x2Binned,
        NfovUnbinned,
        Wfov2x2Binned,
        WfovUnbinned,
        PassiveIr,
    };

    enum class CaptureFrameRate {
        Fps5,
        Fps15,
        Fps30,
    };

    struct CaptureCameraConfig {
        std::uint32_t deviceIndex = 0;
        CaptureColorFormat colorFormat = CaptureColorFormat::Bgra32;
        CaptureColorResolution colorResolution = CaptureColorResolution::P720;
        CaptureDepthMode depthMode = CaptureDepthMode::NfovUnbinned;
        CaptureFrameRate frameRate = CaptureFrameRate::Fps30;
        bool synchronizedImagesOnly = true;
    };

    struct CaptureRuntimeConfig {
        int captureTimeoutMs = 100;
        int failureBackoffMs = 10;
    };

    struct CaptureConfig {
        bool enabled = false;
        CaptureCameraConfig camera{};
        CaptureRuntimeConfig runtime{};
    };

} // namespace edgevision::config

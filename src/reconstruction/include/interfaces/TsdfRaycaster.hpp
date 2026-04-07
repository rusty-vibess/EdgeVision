#pragma once

#include "types/TsdfTypes.hpp"

class ITsdfRaycaster {
  public:
    virtual ~ITsdfRaycaster();

    [[nodiscard]] virtual TsdfRaycastBuffers raycast(
        const RgbdFrameView& frame,
        bool useCameraDepth
    ) const = 0;
};

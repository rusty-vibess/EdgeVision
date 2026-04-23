#pragma once

#include <cstdint>

namespace edgevision::model::scene {

    using SceneVersionId = std::uint64_t;

    struct VoxelCoordinate {
        int x = 0;
        int y = 0;
        int z = 0;
    };

    [[nodiscard]] constexpr bool operator==(
        const VoxelCoordinate& lhs,
        const VoxelCoordinate& rhs
    ) {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    [[nodiscard]] constexpr bool operator!=(
        const VoxelCoordinate& lhs,
        const VoxelCoordinate& rhs
    ) {
        return !(lhs == rhs);
    }

    struct VoxelRegion {
        VoxelCoordinate origin{};
        VoxelCoordinate extent{};
        bool isWholeScene = true;
    };

    [[nodiscard]] constexpr VoxelRegion wholeSceneRegion() {
        return VoxelRegion{};
    }

    [[nodiscard]] constexpr VoxelRegion boundedVoxelRegion(
        VoxelCoordinate origin,
        VoxelCoordinate extent
    ) {
        return VoxelRegion{origin, extent, false};
    }

    [[nodiscard]] constexpr bool operator==(const VoxelRegion& lhs, const VoxelRegion& rhs) {
        return lhs.origin == rhs.origin && lhs.extent == rhs.extent
            && lhs.isWholeScene == rhs.isWholeScene;
    }

    [[nodiscard]] constexpr bool operator!=(const VoxelRegion& lhs, const VoxelRegion& rhs) {
        return !(lhs == rhs);
    }

} // namespace edgevision::model::scene

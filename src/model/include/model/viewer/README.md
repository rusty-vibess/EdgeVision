# Model Viewer API

Public viewer model state lives in this directory.

## Public API

### Stores

- `ViewerPoseStore`
  - `update(ViewerPose)`
  - `latest()`
  - `waitForNewer(ViewerPoseGeneration, timeout)`
- `RenderOutputStore`
  - `publish(RenderOutput)`
  - `latest()`
  - `recent(count)`

### Public Value Types

- Public viewer value/types live under `types/`
  - `types/identity.hpp`
  - `types/pose.hpp`
  - `types/output.hpp`

## Flow

- producers publish view requests via `ViewerPoseStore`
- `ViewerPoseStore` preserves the latest pose snapshot
- generations advance monotonically so background consumers can wait for newer poses
- viewer code publishes rendered RGB frames via `RenderOutputStore`
- `RenderOutputStore` retains the latest output plus a bounded recent history

## Other Notes

- `ViewerPose` stores camera pose, intrinsics, requested image size, and update generation.
- `RenderOutput` stores contiguous RGB bytes plus the pose generation and scene version used.
- stores own synchronization; value types remain plain transport objects

## TL;DR

- `ViewerPoseStore` is the synchronized handoff point for view requests.
- `RenderOutputStore` is the synchronized handoff point for rendered frames.
- generations and bounded history are part of the public contract.

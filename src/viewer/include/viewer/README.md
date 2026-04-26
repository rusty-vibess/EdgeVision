# Viewer API

Public viewer control types live in this directory.

## Public API

### Background Runner

- `SceneViewerRunner`
  - `start()`
  - `requestStop()`
  - `join()`
  - `running() const`
  - `status() const`

### Public Status Types

- `types/runner.hpp`

## Internal Implementation

### Internal Types

- `SceneViewer`
  - private synchronous worker owned by `SceneViewerRunner`

## Flow

- `SceneViewerRunner` waits for or samples `ViewerPose` snapshots from `ViewerPoseStore`
- the internal `SceneViewer` worker acquires `SharedScene::read()` for one supplied pose
- the internal InfiniTAM pipeline keeps free-view render state warm across renders
- rendered RGB outputs are published into `RenderOutputStore`
- `SceneViewerRunner` drives the private `SceneViewer` worker according to
  `ViewerRuntimeConfig.loopPolicy`

## Other Notes

- `PoseDriven` waits for newer pose generations before rendering.
- `LiveLoop` repeatedly renders the latest available pose at the configured period.
- this module does not own networking or `main.cpp` integration in this pass.

## TL;DR

- `SceneViewerRunner` is the public lifecycle wrapper for background rendering loops.
- `SceneViewer` is now a private worker owned by the runner.
- output publication still happens through `model/viewer` stores.

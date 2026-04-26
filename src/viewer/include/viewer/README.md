# Viewer API

Public viewer control types live in this directory.

## Public API

### Render Worker

- `SceneViewer`
  - `renderLatestPose()`
  - `renderPose(const ViewerPose&)`

### Background Runner

- `ViewerRunner`
  - `start()`
  - `requestStop()`
  - `join()`
  - `running() const`
  - `status() const`

### Public Status Types

- `types/runner.hpp`

## Flow

- `SceneViewer` reads `ViewerPose` snapshots and acquires `SharedScene::read()`
- the internal InfiniTAM pipeline keeps free-view render state warm across renders
- rendered RGB outputs are published into `RenderOutputStore`
- `ViewerRunner` drives `SceneViewer` according to `ViewerRuntimeConfig.loopPolicy`

## Other Notes

- `PoseDriven` waits for newer pose generations before rendering.
- `LiveLoop` repeatedly renders the latest available pose at the configured period.
- this module does not own networking or `main.cpp` integration in this pass.

## TL;DR

- `SceneViewer` is the synchronous render facade.
- `ViewerRunner` is the lifecycle wrapper for background rendering loops.
- output publication still happens through `model/viewer` stores.

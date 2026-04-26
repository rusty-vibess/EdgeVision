# Model Scene API

Public scene API lives in this directory.

## Public API

### Facade

- `SharedScene`
  - `read()`
  - `write()`
  - `version()`
  - `publish(SceneWriteAccess&)`

### Granted Access Types

- `SceneReadAccess`
  - `version()`
  - `scene() const`
- `SceneWriteAccess`
  - `version()`
  - `scene()`

### Public Value Types

- Public scene value/types live under `types/`
  - `types/access.hpp`
  - `types/infinitam.hpp`
  - `types/version.hpp`

## Internal Implementation

### Synchronization

- `SceneAccessLease`
  - internal only
  - owns synchronization, admission, release, access mode, and visible version

## Read Flow

- caller requests `SharedScene::read()`
- blocking admission is handled internally by `SceneAccessLease`
- caller receives `SceneReadAccess`
- caller inspects the current committed scene
- access destruction releases the read lease

## Write Flow

- caller requests `SharedScene::write()`
- blocking admission is handled internally by `SceneAccessLease`
- caller receives `SceneWriteAccess`
- caller mutates the shared scene
- caller commits with `publish(writeAccess)`
- committed scene version increments
- access destruction releases the write lease

## Other Notes

- Access wrappers contain no synchronization logic.
- `SharedScene` read scheduling is configured at construction time via `config::SceneConfig`.
- `publish()` is the explicit commit point for written scene state.
- Readers observe the latest committed scene state.

## TL;DR

- `SharedScene` is the facade.
- `SceneAccessLease` handles synchronization internally.
- `SceneReadAccess` / `SceneWriteAccess` mean access has already been granted.

# Config API

The public configuration API for EdgeVision lives in this directory.

## Public API

### Facade

- `parseCommandLine(...)`
- `CommandLineParseResult`

### Public Value Types

- `AppConfig`
- `types/image.hpp`
- `types/capture.hpp`
- `types/streaming.hpp`
- `types/viewer.hpp`
- `types/builder.hpp`
- `types/scene.hpp`
- `types/debug.hpp`

## Flow

### Input Flow

- `parseCommandLine(...)` starts from `AppConfig` defaults and applies CLI overrides.
- `AppConfig.imageSize` is the shared runtime pixel size for capture, viewer, and WebRTC.
- capture derives its hardware color resolution from `AppConfig.imageSize`.
- WebRTC uses `AppConfig.imageSize` for pipeline caps and outgoing frame validation.

## Other Notes

- `CaptureCameraConfig` no longer owns a separate color-resolution field.
- `WebRtcStreamingConfig` no longer owns separate width/height fields.

## TL;DR

- `AppConfig` is the top-level config surface.
- `AppConfig.imageSize` is the single source of truth for runtime image size.
- transport and capture settings still live in their domain-specific config structs.

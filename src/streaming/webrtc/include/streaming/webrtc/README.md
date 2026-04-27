# WebRTC Streaming API

Public transport control for remote viewer output lives in this directory.

## Public API

### Lifecycle

- `WebRtcServer`
  - `start()`
  - `stop()`
- `startWebRtcServer(...)`

## Flow

- incoming client pose messages carry a pose matrix only
- WebRTC resets the relative-pose baseline when a client connects
- relative pose application is handled by `model::viewer::ViewerPoseStore`
- rendered frames are read from `model::viewer::RenderOutputStore`
- WebRTC transports the latest rendered RGB frame as H.264 over `webrtcbin`

## Other Notes

- `WebRtcServer` does not read `FrameStore`, `SharedScene`, or renderer callbacks directly.
- `WebRtcServer` does not compose poses or reconstruct `ViewerPose` values directly.
- transport sizing, bitrate, and signalling settings come from `config::WebRtcStreamingConfig`
- TCP streaming remains a separate transport option in `streaming/`

## TL;DR

- pose input goes through `ViewerPoseStore`
- rendered output comes from `RenderOutputStore`
- WebRTC owns transport only, not rendering policy

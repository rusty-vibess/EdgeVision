# streaming/webrtc

Server-side WebRTC streaming module for EdgeVision on Jetson Orin Nano. Publishes two H.264 tracks (RGB + TSDF) at 854x480@30 / 1.5 Mbps over `webrtcbin` (NVENC via `nvv4l2h264enc`), with a single-client JSON signalling listener on a tiny WebSocket port (default `:6689`). Pose updates flow back from the client over a DataChannel and drive the TSDF raycast pose for the next frame. Construct `WebRtcServer` once at boot with the `FrameStore` and the (forthcoming) `SharedScene`, call `start()`, and the rest is automatic.

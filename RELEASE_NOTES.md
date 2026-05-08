# Release Notes

## v0.0.1 - Presentation/Showcase Pre-Release

This checkpoint is approximately the EdgeVision build used for the Presentation/Showcase.

### Status

- Pre-release showcase checkpoint.
- Relatively stable for the demonstrated workflow.
- Not the final v1 build.

### Included

- Azure Kinect/K4A RGB-D capture path.
- InfiniTAM TSDF reconstruction and CUDA-backed rendering pipeline.
- Background capture, builder, and viewer runners.
- WebRTC signalling and H.264 video transport path.
- Data-channel pose updates for remote viewing.
- Legacy TCP render-server module.
- Jetson cross-build workflow with sysroot and overlay support.

### Known Issues

- WebRTC can establish a stable connection and then repeatedly disconnect.
- Runtime and build setup are Jetson/JetPack specific.
- Capture is currently required at application startup.
- Legacy TCP streaming options are parsed, but the showcase runtime path is WebRTC.
- Bundled test environment propagation needs cleanup.

### Next

- v1: fix server/streaming stability and complete open-source release hygiene.
- v2: pivot toward the FPGA build.

# EdgeVision

EdgeVision is a Jetson-targeted RGB-D reconstruction and remote viewing prototype. It captures synchronized Azure Kinect/K4A colour and depth frames, integrates them into an InfiniTAM TSDF scene with CUDA support, renders free-view RGB output, and exposes that output to a remote viewer over WebRTC.

## Status

**v0.0.1 pre-release.**

This repository is the public checkpoint for the Presentation/Showcase build. It is relatively stable for the showcased workflow, but it is not the final v1 build.

Known headline issue: the WebRTC path can establish a stable connection and then repeatedly disconnect. v1 is expected to focus on server and streaming stability. v2 is expected to pivot toward the FPGA build.

## What It Does

- Captures synchronized RGB-D frames from an Azure Kinect/K4A device.
- Aligns, validates, and stores frames for reconstruction.
- Integrates incoming frames into an InfiniTAM TSDF scene.
- Maintains thread-safe scene, frame, viewer-pose, and render-output state.
- Renders RGB views from the reconstructed scene.
- Streams rendered output over WebRTC, with pose updates arriving over the WebRTC data channel.
- Includes a legacy TCP render server module for the older remote-viewer protocol; the showcase runtime path is WebRTC.

## Architecture

- `src/capture`: K4A device access, capture handles, RGB-D frame assembly, validation, alignment, and background ingest into `FrameStore`.
- `src/model`: synchronized frame, scene, scene-version, viewer-pose, and render-output stores shared across runtime threads.
- `src/builder`: frame consumption and InfiniTAM integration into the shared TSDF scene.
- `src/viewer`: InfiniTAM free-view rendering from `ViewerPoseStore` into `RenderOutputStore`.
- `src/streaming`: legacy single-client TCP render server module, not currently started by `src/main.cpp`.
- `src/streaming/webrtc`: WebSocket signalling, GStreamer `webrtcbin` pipeline setup, H.264 video transport, and data-channel pose updates.
- `src/app`: runtime helpers for viewer-pose seeding and debug frame dumps.

Useful local API entrypoints:

- [config API](src/config/include/config/README.md)
- [model scene API](src/model/include/model/scene/README.md)
- [model viewer API](src/model/include/model/viewer/README.md)
- [viewer API](src/viewer/include/viewer/README.md)
- [WebRTC streaming API](src/streaming/webrtc/include/streaming/webrtc/README.md)
- [third-party layout](third_party/README.md)

## Requirements

This project is target-specific and assumes a Jetson runtime environment.

- NVIDIA Jetson target userspace, currently mirrored as a JetPack `r36.4.4` sysroot.
- CUDA in the target sysroot. The current toolchain expects CUDA 12.6 under `/usr/local/cuda-12.6`.
- Azure Kinect/K4A hardware and K4A runtime dependencies.
- OpenGL via GLX and an active X11 `DISPLAY`; K4A requires this even for headless-style runs.
- CMake 3.22+ for the full workflow, Ninja, GCC/G++ `aarch64-linux-gnu`, `rsync`, and `symlinks`.
- Docker/devcontainer workflow for the cross-build environment.
- GStreamer/WebRTC dependencies from the Jetson sysroot.
- A sysroot overlay mounted at `/opt/jetson-sysroot-overlay`.

The devcontainer is defined in `.devcontainer/` and `docker-compose.yml`. It mounts:

```text
./sysroots/r36.4.4              -> /opt/jetson-sysroot
./third_party/r36.4.4-overlay   -> /opt/jetson-sysroot-overlay
```

## Workflow

Recommended order:

1. Create or update the Jetson sysroot.
2. Populate `third_party/<SYSROOT_VERSION>-overlay`.
3. Add required Python wheels and target-platform binaries.
4. Build third-party source dependencies into the overlay.
5. Run toolchain smoke tests where applicable.
6. Build the project.
7. Build a bundle and run it on the Jetson.

## Sysroot

EdgeVision cross-compiles against a Jetson sysroot so the toolchain sees the same headers, libraries, and linker layout as the target device.

### Create The Sysroot

From the host:

```bash
/opt/homebrew/bin/rsync -aH --numeric-ids --info=progress2 \
  --exclude={"/dev/*","/proc/*","/sys/*","/run/*","/tmp/*","/media/*","/mnt/*","/lost+found"} \
  <USER>@<JETSON_IP>:/ ./sysroots/<SYSROOT_VERSION>
```

Result:

```text
./sysroots/<SYSROOT_VERSION>
```

### Post-Process The Sysroot

Run these inside the cross-compilation container. The build expects:

```text
Sysroot: /opt/jetson-sysroot
Overlay: /opt/jetson-sysroot-overlay
```

Rewrite absolute symlinks in the rsynced filesystem:

```bash
symlinks -crv ./sysroots/<SYSROOT_VERSION>
```

Preview and then apply `update-alternatives` rewrites:

```bash
SYSROOT=/workspaces/repo/sysroots/<SYSROOT_VERSION> DRY_RUN=1 \
  /workspaces/repo/scripts/rewrite-alternatives.sh
```

Once the output looks correct, rerun with `DRY_RUN=0` or unset.

### Overlay

Use the overlay for Jetson-specific Python wheels, project-pinned binaries or libraries, source-built dependency installs, and anything that should not be copied directly from the device snapshot.

Known dependency: PyTorch Jetson wheel.

```bash
pip install torch==2.8.0 --index-url https://pypi.jetson-ai-lab.io/jp6/cu126
```

Copy into the matching overlay:

```text
From: <VENV>/lib/python3.10/site-packages/torch
To:   third_party/<SYSROOT_VERSION>-overlay/site-packages/torch
```

See [third_party/README.md](third_party/README.md) for overlay layout details.

## Build From Source

These commands are intended to run inside the devcontainer or equivalent cross-build environment.

### Build Third-Party Dependencies

```bash
cmake -S cmake/third_party \
  -B build-deps \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/jetson-sysroot-overlay/install \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
&& cmake --build build-deps --target install
```

### Build Project Debug

`Debug` is the default CMake build type and enables address/undefined sanitizers for project code.

```bash
cmake -S . \
  -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build -j4
```

### Build Project RelWithDebInfo

Use this for runtime validation when sanitizer noise from CUDA or target libraries gets in the way.

```bash
cmake -S . \
  -B build-relwithdebinfo \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_TESTS=OFF \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build-relwithdebinfo -j4
```

### Bundle

```bash
cmake --build build --target bundle
cmake --build build-relwithdebinfo --target bundle
```

The bundle target creates a self-contained runtime directory under the build directory, including the `EdgeVision` binary and selected runtime libraries from the overlay.

### Patches

Some dependencies require source changes. Keep those diffs in `patches/` and ensure they are collected by the third-party build.

```bash
diff -u .../<FILE>.orig .../<FILE> > /workspaces/repo/patches/<FILE>-<STUB>.patch
```

## Running

Run from the bundle root on the Jetson:

```bash
sudo env LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}" ./bin/EdgeVision
```

Usage:

```text
./EdgeVision [--port 6688] [--enable-tcp-streaming]
             [--webrtc-port 6689] [--disable-webrtc]
             [--webrtc-stun url|none]
             [--disable-capture]
             [--read-policy greedy|balanced]
             [--viewer-policy event|hot-loop]
             [--enable-debug]
             [--debug-frames N]
```

Current CLI options:

- `--port <1-65535>`: set the legacy TCP render-server config port. Default: `6688`.
- `--enable-tcp-streaming`: set the legacy TCP streaming config flag. The current `src/main.cpp` path does not start the TCP server.
- `--webrtc-port <1-65535>`: set the WebRTC signalling port. Default: `6689`.
- `--disable-webrtc`: disable WebRTC. Default: enabled.
- `--webrtc-stun <url|none>`: set a STUN server, or use `none` for direct/local links. Default: none.
- `--disable-capture`: parsed, but the current application exits because capture is required.
- `--read-policy <greedy|balanced>`: set shared-scene read scheduling. Default: `greedy`.
- `--viewer-policy <event|hot-loop>`: choose event-driven rendering or periodic rendering. Default: `event`.
- `--enable-debug`: enable viewer frame dumping. Default dump count: `5` unless `--debug-frames` is set.
- `--debug-frames <N>`: enable debug dumping and stop after `N` rendered frames.

Runtime defaults include a `1280x720` image size and WebRTC signalling on `0.0.0.0:6689`.

### GLX/X11 Requirement

`k4a` requires OpenGL. OpenGL via GLX requires an active X11 `DISPLAY` context.

On Jetson, X will not start unless a display is detected. In a headless setup, either plug the Jetson into a monitor via DisplayPort or use a headless DP dongle that spoofs EDID.

Creating the required `DISPLAY` context from a terminal:

```bash
sudo systemctl stop gdm3
ls /tmp/.X11-unix/
Xorg :0 -nolisten tcp &
export DISPLAY=:0
glxinfo | grep "OpenGL renderer"
```

You should see the NVIDIA/Tegra renderer, not `llvmpipe`.

The local shell setup may also provide:

```bash
spinup_openGL.sh
```

### Direct Viewer To Jetson Ethernet

Set static IPs on the direct Ethernet link:

```text
Viewer host: 192.168.50.1/24
Jetson:      192.168.50.2/24
```

Jetson setup:

```bash
sudo nmcli con add type ethernet ifname enP8p1s0 con-name viewer-direct \
  ipv4.method manual ipv4.addresses 192.168.50.2/24 ipv6.method ignore
sudo nmcli con up viewer-direct
```

Quick link tests:

```bash
# Viewer: nc -lv 5001
echo "hello" | nc 192.168.50.1 5001

# Viewer: nc -ul 5002
echo "udp" | nc -u -w1 192.168.50.1 5002
```

Direct-link WebRTC run command:

```bash
sudo env \
  OPENSSL_CONF="<OPENSSL_CONF_PATH>" \
  EDGEVISION_WEBRTC_DIRECT_IPV4=192.168.50.2,192.168.50.1 \
  G_TLS_GNUTLS_PRIORITY="NORMAL:%COMPAT" \
  LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}" \
  ./bin/EdgeVision --viewer-policy hot-loop --read-policy balanced --webrtc-stun none
```

Notes:

- `--webrtc-stun none` keeps ICE on the direct `192.168.50.x` link.
- `EDGEVISION_WEBRTC_DIRECT_IPV4` filters ICE to the Jetson/viewer host candidates.
- `EDGEVISION_WEBRTC_DISABLE_DATA_CHANNEL=1` disables the data channel for video-only debugging.
- `OPENSSL_CONF` is only needed when using a custom OpenSSL compatibility config.

Pose updates sent over the data channel use:

```json
{"type":"pose","matrix":[16 floats]}
```

The matrix must use the InfiniTAM/ORUtils layout:

```text
[
  r00, r10, r20, 0,
  r01, r11, r21, 0,
  r02, r12, r22, 0,
  x,   y,   z,   1
]
```

Translation is `matrix[12]`, `matrix[13]`, and `matrix[14]`.

## Tests

Tests are built by default. Disable them with:

```bash
-DENABLE_TESTS=OFF
```

Bundled tests are included by:

```bash
cmake --build build --target bundle
```

Run tests from the target bundle directory:

```bash
ctest --test-dir tests --output-on-failure
```

For CUDA-heavy validation without debug sanitizers:

```bash
cmake -S . \
  -B build-relwithdebinfo-tests \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_TESTS=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build-relwithdebinfo-tests -j3
```

If using the debug bundle, rerun tests with the CUDA leak-sanitizer suppression file:

```bash
cd build/bundle
LSAN_OPTIONS="suppressions=$(pwd)/cmake/suppressions/lsan_cuda.supp:print_suppressions=1:verbosity=1" \
ctest --test-dir tests --output-on-failure
```

For CUDA-specific diagnostics:

```bash
for tool in memcheck racecheck initcheck synccheck; do
    compute-sanitizer --tool "$tool" \
      --leak-check full \
      --report-api-errors all \
      --log-file "sanitizer_${tool}_log.txt" \
      "./tests/<TEST_BIN>" >> "sanitizer_${tool}_log.txt" 2>&1 || break
done
```

`ASAN_OPTIONS=alloc_dealloc_mismatch=0` is currently needed for the `builder_tests` InfiniTAM upstream allocation mismatch.

### Toolchain Smoke Tests

These commands are retained from the current workflow, but the smoke-test setup needs updating after recent linking changes.

Configure:

```bash
cmake -S /workspaces/repo/toolchain-tests \
  -B /workspaces/repo/toolchain-tests/build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DTORCH_RUNTIME_LIB_DIR=/site-packages/torch/lib
```

If reconfiguring from scratch:

```bash
rm -rf /workspaces/repo/toolchain-tests/build
```

Build:

```bash
cmake --build /workspaces/repo/toolchain-tests/build
```

## Container Workflow

The recommended container path is the VS Code Dev Containers extension.

Manual lifecycle:

```bash
docker buildx create --name docker-platform-builder --use
docker buildx inspect --bootstrap

docker compose build
docker compose up
docker exec -it jetson-dev bash
```

## Runtime Debugging

From the bundle root:

```bash
sudo LD_LIBRARY_PATH="$PWD/lib" gdb --args ./bin/EdgeVision --enable-debug --debug-frames 15
```

With `perf`:

```bash
sudo env LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}" \
  perf stat -d ./bin/EdgeVision --enable-debug --viewer-policy hot-loop --debug-frames 15
```

WebRTC/GStreamer logging:

```bash
sudo env \
  GST_DEBUG_NO_COLOR=1 \
  GST_DEBUG_FILE=/tmp/edgevision-media.log \
  GST_DEBUG="appsrc:4,x264enc:4,h264parse:4,rtph264pay:5,webrtc*:5,basesrc:5" \
  OPENSSL_CONF="<OPENSSL_CONF_PATH>" \
  EDGEVISION_WEBRTC_DIRECT_IPV4=192.168.50.2,192.168.50.1 \
  G_TLS_GNUTLS_PRIORITY="NORMAL:%COMPAT" \
  LD_LIBRARY_PATH="$PWD/lib:${LD_LIBRARY_PATH:-}" \
  ./bin/EdgeVision --viewer-policy hot-loop --read-policy balanced --webrtc-stun none
```

Useful log slice:

```bash
grep -Ei "not-negotiated|Internal data stream error|dtls|ssl|fatal|error|rtph264pay|src_video" \
  /tmp/edgevision-media.log | tail -200
```

Direct-link DTLS/ICE capture:

```bash
sudo tcpdump -Z root -i any -s 0 -w /tmp/edgevision-dtls.pcap \
  'udp and host 192.168.50.1 and host 192.168.50.2'
```

Jetson performance helpers:

```bash
sudo nvpmodel -q
sudo nvpmodel -m 0
sudo jetson_clocks
sudo jetson_clocks --show
```

## Known Limitations

- WebRTC can connect and then repeatedly disconnect; this is the main v0.0.1 server issue.
- This is a showcase checkpoint, not the final v1 release.
- The build and runtime are tightly coupled to Jetson, JetPack `r36.4.4`, CUDA, K4A, and GLX/X11.
- Capture is currently required; `--disable-capture` is parsed but exits at startup.
- Legacy TCP streaming options are parsed, but the current `src/main.cpp` path does not start the TCP server.
- Toolchain smoke tests are marked as needing update after recent linking changes.
- Bundled test environment propagation still needs cleanup.
- Third-party licensing needs a full audit before this repository is presented as contributor-ready open source.

## Roadmap

- v1: fix WebRTC/server stability, tighten bundled runtime/test environment handling, and complete release hygiene.
- v2: pivot the build toward the FPGA implementation.

## Release Notes

See [RELEASE_NOTES.md](RELEASE_NOTES.md).

## Licence

Licence TBD. Public visibility of this repository does not yet grant permission to use, copy, modify, or redistribute the code. Choose and add a real `LICENSE` before encouraging external reuse or contributions.

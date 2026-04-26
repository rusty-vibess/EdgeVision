### Workflow (Recommended)

1. Create or update the sysroot (see "Sysroot").
2. Populate the overlay in `third_party/<SYSROOT_VERSION>-overlay`.
3. Add Python wheels (see "Sysroot" → Overlay).
4. Build source deps (see "Building" → Build Deps).
5. Run toolchain smoke tests.
6. Build the project.

---

### Sysroot

We cross-compile against a Jetson sysroot (JetPack r36.4.4 at time of writing) so the toolchain always sees the same headers, libraries, and linker layout—independent of the host.

The sysroot is built by mirroring the Jetson root filesystem with `rsync`, then applying deterministic cleanup to make it relocatable.

---

#### 1) Create the sysroot (host)

```bash
# Or equivalent rsync binary
#
# --numeric-ids preserves UID/GID consistency between host and target
/opt/homebrew/bin/rsync -aH --numeric-ids --info=progress2 \
  --exclude={"/dev/*","/proc/*","/sys/*","/run/*","/tmp/*","/media/*","/mnt/*","/lost+found"} \
  <USER>@<JETSON_IP>:/ ./sysroots/<SYSROOT_VERSION>
```

* Result: `./sysroots/<SYSROOT_VERSION>` is a snapshot of the Jetson userspace
* Exclusions: virtual filesystems + runtime-only mounts

---

#### 2) Post-process (container)

Run these inside the cross-compilation container. The build expects:

* Sysroot: `/opt/jetson-sysroot`
* Overlay: `/opt/jetson-sysroot-overlay`

The rsynced filesystem contains absolute symlinks (e.g. `/lib/...`) which break relocation.

```bash
symlinks -crv ./sysroots/<SYSROOT_VERSION>
```

`update-alternatives` stores absolute paths that survive both `rsync` and symlink rewriting.

```bash
# DRY_RUN=1 previews changes without mutating files
SYSROOT=/workspaces/repo/sysroots/<SYSROOT_VERSION> DRY_RUN=1 \
  /workspaces/repo/scripts/rewrite-alternatives.sh
```

* Mainly affects toolchain-adjacent binaries (`gcc`, `ld`, `python`)
* Once the output looks sane, rerun with `DRY_RUN=0` (or unset)

---

#### 3) Overlay

Not everything should come from the device snapshot. Use the overlay for:

* Jetson-specific Python wheels
* Project-pinned binaries or libraries
* Anything awkward or unstable to reproduce via rsync

The overlay is mounted alongside the sysroot during compilation. If you ship shared libraries via overlay, ensure the final binaries can locate them on-device (typically via `RPATH` or loader configuration).

**Known dependency: PyTorch (Jetson wheel)**

Install into a Jetson-compatible venv:

```bash
pip install torch==2.8.0 --index-url https://pypi.jetson-ai-lab.io/jp6/cu126
```

Copy into the project overlay:

* From: `<VENV>/lib/python3.10/site-packages/torch`
* To: `third_party/<SYSROOT_VERSION>-overlay/site-packages/torch`

See [third_party/README.md](third_party/README.md) for overlay layout details.

---

### Building

#### Build Deps

```bash
cmake -S cmake/third_party \
  -B build-deps \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/jetson-sysroot-overlay/install \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
&& cmake --build build-deps --target install
```

#### Build Project Debug

```bash
cmake -S . \
  -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  && cmake --build build -j2
```

#### Build Project RelWithDebInfo

```bash
cmake -S . \
  -B build-relwithdebinfo \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_TESTS=OFF \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build-relwithdebinfo -j2
```

#### Bundle

```bash
cmake --build build --target bundle
cmake --build build-relwithdebinfo --target bundle
```

#### Patches

Some deps require source changes. Use patches to capture those diffs, place them in
`patches/`, and ensure they are collected and applied by the build.

Generate patches like this:

```bash
diff -u   .../<FILE>.orig   .../<FILE>   > /workspaces/repo/patches/<FILE>-<STUB>.patch
```

---

### Tests

Tests are built by default. If building for `Release` or you do not want tests ensure to pass `-DENABLE_TESTS=OFF` at configuration time.

Tests if available will automatically be bundled by `cmake --build build --target bundle`.

To run tests utilise `ctest --test-dir tests --output-on-failure` inside the target bundle directory.

### CUDA Tests And Sanitizers

`Debug` is the default development build and enables sanitizers across project code and tests. This is useful for application debugging, but CUDA runtime/device code can produce sanitizer noise that is not always actionable.

Use a separate `RelWithDebInfo` build directory when validating tests (especially CUDA heavy ones) without sanitizers:

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

If you still want the `Debug` sanitizer coverage, the `Debug` bundle includes `cmake/suppressions`. From the bundle root you can rerun tests with the CUDA suppression file:

```bash
cd build/bundle
LSAN_OPTIONS="suppressions=$(pwd)/cmake/suppressions/lsan_cuda.supp:print_suppressions=1:verbosity=1" \
ctest --test-dir tests --output-on-failure
```

Finally, for CUDA-specific diagnostics, run `compute-sanitizer` directly against the target test binary (`RelWithDebInfo` is best to avoid conflict with CUDA sanitizers and GCC ones):

```bash
for tool in memcheck racecheck initcheck synccheck; do
  compute-sanitizer --tool "$tool" --leak-check full --report-api-errors all "./tests/<TEST_BINARY>" || break
done
```

`ASAN_OPTIONS=alloc_dealloc_mismatch=0` to stop builder_tests error in InfiniTAM upstream.

---

### Toolchain Smoke Tests (NEEDS UPDATE)

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

---

### Runtime Info

`k4a` requires OpenGL.
OpenGL (via GLX) requires an active X11 `DISPLAY` context.

On Jetson, X will not start unless a display is detected. In a headless build, you must either:
* Plug the Jetson into a monitor via DisplayPort, or
* Use a headless DP dongle that spoofs EDID

There is no runtime-only software workaround if the dependency requires GLX.

Creating the required DISPLAY context from terminal:

```bash
# JetPack boots gdm3 automatically when a display is detected — stop it
sudo systemctl stop gdm3

# (Optional) prevent it restarting on reboot
# sudo systemctl disable gdm3

# Confirm no X11 instances are running
ls /tmp/.X11-unix/
# Start a minimal X server
Xorg :0 -nolisten tcp &
# Set DISPLAY for this shell
export DISPLAY=:0
# Confirm hardware OpenGL is active
glxinfo | grep "OpenGL renderer"
```

You should see the NVIDIA / Tegra renderer — not `llvmpipe`.

---

### Runtime Debugging And Performance

Perf is available on the Jetson form metric dumping.

For gdb:

```bash
sudo gdb --args ./bin/EdgeVision --enable-capture
```

```gdb
set env LD_LIBRARY_PATH /home/rusty/dev/latest_edgevision_bundle/bundle/lib
# Then standard gdb style debugging from there
# Useful functions to stop on
edgevision::model::frame::FrameStore::submitFrame(edgevision::model::frame::Frame)
# Useful tooling for dbg
dump binary memory /output/file.bin hex hex + offset
```

Cache may saturate memory on jetson:

```bash
# When cache collects on Jetson
sudo sync
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

---

### Container Workflow

Since this repo uses `.devcontainer`, it’s recommended to let the VS Code extension handle container lifecycle.

Information for manual container management:

```bash
# Navigate to this project
cd path/to/repo

# Buildx is required to use the `platform:` field in docker-compose.yml
docker buildx create --name docker-platform-builder --use
docker buildx inspect --bootstrap

docker compose build
docker compose up
```

Shell access:

```bash
docker exec -it jetson-dev bash
```

---

### Dev Info

For LSP support, you need a `compile_commands.json` at the repo root. The simplest
way is to configure with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` and then symlink the
generated file.

```bash
# From repo root
ln -s .../build/compile_commands.json .
```

Then restart clangd extension in vscode.

---

### Cheatsheet

Quick commands you may want to grab:

```bash
# TODO: Toolchain-tests need to be updated to new linking methods
# Configure + build toolchain tests
cmake -S /workspaces/repo/toolchain-tests \
  -B /workspaces/repo/toolchain-tests/build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DTORCH_RUNTIME_LIB_DIR=/site-packages/torch/lib \
  && cmake --build /workspaces/repo/toolchain-tests/build

# Configure + build third-party deps into the overlay
cmake -S cmake/third_party \
  -B build-deps \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/opt/jetson-sysroot-overlay/install \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
&& cmake --build build-deps --target install

# Configure + build project Debug
cmake -S . \
  -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build -j3

# Configure + build RelWithDebInfo
cmake -S . \
  -B build-relwithdebinfo \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_TESTS=OFF \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
&& cmake --build build-relwithdebinfo -j3 --target bundle

# Individual build commands
cmake --build /workspaces/repo/toolchain-tests/build
cmake --build build-deps --target install
cmake --build build
# Build to bundle for export
cmake --build build --target bundle
# scp bundle to jetson
scp -r build/bundle nano:dev/latest_edgevision_bundle
# scp just latest bin to jetson (saves writes)
scp -r build/bundle/bin/* nano:dev/latest_edgevision_bundle/bundle/bin
# Run programme from bundle root
sudo LD_LIBRARY_PATH="$PWD/lib:$LD_LIBRARY_PATH" ./bin/EdgeVision --enable-capture
# Debug
gdb --args ./bin/EdgeVision --enable-capture
```

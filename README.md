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

---

### Toolchain Smoke Tests

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

### Building

#### Build Deps

```bash
cmake -S cmake/third_party \
  -B build-deps \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/path/to/install \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
&& cmake --build build-deps --target install
```

#### Patches
Some deps require source changes. Use patches to capture those diffs, place them in
`patches/`, and ensure they are collected and applied by the build.

Generate patches like this:
```bash
diff -u   .../<FILE>.orig   .../<FILE>   > /workspaces/repo/patches/<FILE>-<STUB>.patch
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

---

### Cheatsheet

Quick commands you may want to grab:

```bash
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

# Configure + build project
cmake -S . \
  -B build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  -DTORCH_RUNTIME_LIB_DIR=/site-packages/torch/lib \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  && cmake --build build

# Individual build commands
cmake --build /workspaces/repo/toolchain-tests/build
cmake --build build-deps --target install
cmake --build build
```

Then restart clangd extension in vscode.

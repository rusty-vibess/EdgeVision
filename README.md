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
  <USER>@<JETSON_IP>:/ ./sysroots/<VERSION>
````

* Result: `./sysroots/<VERSION>` is a snapshot of the Jetson userspace
* Exclusions: virtual filesystems + runtime-only mounts

---

#### 2) Post-process (container)

Run these inside the cross-compilation container. The build expects:

* Sysroot: `/opt/jetson-sysroot`
* Overlay: `/opt/jetson-sysroot-overlay`

The rsynced filesystem contains absolute symlinks (e.g. `/lib/...`) which break relocation.

```bash
symlinks -crv ./sysroots/<VERSION>
```

`update-alternatives` stores absolute paths that survive both `rsync` and symlink rewriting.

```bash
# DRY_RUN=1 previews changes without mutating files
SYSROOT=/workspaces/repo/sysroots/<VERSION> DRY_RUN=1 \
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
* To: `third_party/<VERSION>-overlay/site-packages/torch`

---

### Toolchain smoke tests

Configure:

```bash
cmake -S /workspaces/repo/toolchain-tests \
  -B /workspaces/repo/toolchain-tests/build \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
  # Overlay libs may require paths being specified for RPath linking
  -DTORCH_RUNTIME_LIB_DIR=/home/rusty/dev/site-packages/torch/lib
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

### Container workflow
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

For LSP support you'll require a `compile_commands.json` at root, the simplest way to get this is when we configure our cmake pass the `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` flag and then create a symbolic link to the produced file.

```bash
# From repo root
ln -s .../build/compile_commands.json .
```

Then restart clangd extension in vscode.

### Third Party

External dependencies live here when they are not expected to come from the system
package manager. Keep source/vendor material separate from target-platform binaries.

---

#### Dependency categories

1. **Vendored source or header-only libraries**

   Small upstream libraries copied directly into the repo live under:

   ```bash
   third_party/<VENDOR>/<PROJECT>/
   ```

   Keep the upstream licence beside the copied files. Header-only libraries can be included directly by project targets. Source libraries may still need a normal buildstep before they are usable.

2. **Source-built dependencies**

   Use `cmake/third_party/CMakeLists.txt` to fetch, build and install source deps. See the root README build section for the exact command. Add the resulting `install/` directory to `CMAKE_PREFIX_PATH` so consumers can find these packages.


3. **Prebuilt target-platform binaries**

   Binary libraries that cannot reasonably be rebuilt or fetched should stay local.
   Do not commit proprietary runtime binaries. Store them in an ignored directory such
   as:

   ```bash
   third_party/libs_<platform>/
   ```

   Pass that directory to the third-party configure step when it needs to be mirrored
   into the sysroot overlay:

   ```bash
   cmake -S cmake/third_party -B build-deps \
     -DEDGEVISION_THIRD_PARTY_LIB_DIR="$PWD/third_party/libs_<platform>"
   ```

   Treat these as platform-specific artefacts. If your target OS, distro version,
   architecture or ABI differs, acquire or build matching binaries instead of
   reusing these blindly.

---

#### Sysroot overlay workflow

`docker-compose.yml` mounts `third_party/<SYSROOT_VERSION>-overlay` at
`/opt/jetson-sysroot-overlay`.

Recommended order:

1. Create or update the sysroot. See the root README, "Sysroot".
2. Ensure the matching overlay directory exists:

   ```bash
   third_party/<SYSROOT_VERSION>-overlay/
   ```

3. Add Python wheels and/or install built source deps into the overlay.
4. Use the overlay in builds by setting `CMAKE_PREFIX_PATH` to the overlay `install/`directory.

---

#### Python wheels

Install into a local venv, then copy the package into the overlay. Ensure the wheel
matches the target architecture.

```bash
third_party/<SYSROOT_VERSION>-overlay/site-packages/<MODULE_NAME>
```

Example:

```bash
cp -r <VENV_PATH>/lib/python3.10/site-packages/torch \
  third_party/<SYSROOT_VERSION>-overlay/site-packages/torch
```

---

#### Intended layout

* Vendored source/header-only deps: `third_party/<VENDOR>/<PROJECT>/`
* Prebuilt target binaries: `third_party/libs_<platform>/`
* Python wheels in sysroot overlay: `/opt/jetson-sysroot-overlay/site-packages`
* Installed built deps in sysroot overlay: `/opt/jetson-sysroot-overlay/install`
* Closed-source runtime libs in sysroot overlay: `/opt/jetson-sysroot-overlay/lib`

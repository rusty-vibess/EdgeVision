### Third Party

External dependencies live here, grouped by sysroot version. `docker-compose.yml` mounts
`third_party/<SYSROOT_VERSION>-overlay` at `/opt/jetson-sysroot-overlay`.

---

#### Workflow (recommended order)

1. Create or update the sysroot (see root README, "Sysroot").
2. Ensure the matching overlay directory exists:
   `third_party/<SYSROOT_VERSION>-overlay/`.
3. Add Python wheels and/or build source deps into the overlay.
4. Use the overlay in builds by setting `CMAKE_PREFIX_PATH` to the overlay `install/`
   directory.

---

#### 1) Python wheels

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

#### 2) Source-built deps

Use `cmake/third_party/CMakeLists.txt` to fetch, build, and install source deps. See the
root README build section for the exact command. Add the resulting `install/` to
`CMAKE_PREFIX_PATH` so consumers can find these packages.

---

#### Intended layout

* Python wheels: `/opt/jetson-sysroot-overlay/site-packages`
* Installed deps: `/opt/jetson-sysroot-overlay/install`
* Closed source deps: `/opt/jetson-sysroot-overlay/lib`

 Some binaries are proprietary and just a nightmare to get ahold of so they're provided for easy under `third_party/lib`. However it's worth noting they may not be valid for all architectures. Provided binaries will be Ubuntu 20.04 aarch64 libs, consider acquiring your own libs if your target does not match. 

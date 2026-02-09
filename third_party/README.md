### Third Party

External dependencies live here, grouped by sysroot version. `docker-compose.yml` mounts
`third_party/<SYSROOT_VERSION>-overlay` at `/opt/jetson-sysroot/edgevision`.

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

* Python wheels: `/opt/jetson-sysroot/edgevision/site-packages`
* Installed deps: `/opt/jetson-sysroot/edgevision/install`

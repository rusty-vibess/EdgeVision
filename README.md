### Getting the Sysroot:
We need our Sysroot to develop against the Jetson Orin Nano
```bash
# Or equivalent rsync bin
/opt/homebrew/bin/rsync -aHAX --numeric-ids --info=progress2 \
  --exclude={"/dev/*","/proc/*","/sys/*","/run/*","/tmp/*","/home/*","/media/*","/mnt/*","/var/*","/lost+found"} <USER>@<JETSON_IP> ./sysroots/<VERSION>
```

### Compile Toolchain Tests
```bash
cmake -S /workspaces/repo/toolchain-tests \
      -B /workspaces/repo/toolchain-tests/build \
      -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE=/workspaces/repo/toolchains/jetson/jetson-aarch64.cmake \
      -DJETSON_CUDA_ROOT=/opt/jetson-sysroot/usr/local/cuda-12.6
```

### Exec Access via:
```bash
docker exec -it jetson-dev bash
```

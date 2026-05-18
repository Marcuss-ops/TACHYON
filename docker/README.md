# Tachyon Reproducible Linux Build Container

This directory provides a Docker build environment containing all development libraries and tools to compile and test the Tachyon Engine under a fully reproducible Ubuntu 24.04 setup.

## Quick Start

### 1. Build the Docker Image

Run the following command from the root repository directory:

```bash
docker build -f docker/Dockerfile.build -t tachyon-build .
```

### 2. Configure using System Dependencies

Configure the CMake build environment inside the container using the dedicated `linux-container` preset (which enables `TACHYON_USE_SYSTEM_DEPS=ON`):

```bash
docker run --rm -v "$PWD:/workspace" tachyon-build \
  cmake --preset linux-container
```

### 3. Build inside the Container

Compile Tachyon's core library, runtime pipeline, and test binaries:

```bash
docker run --rm -v "$PWD:/workspace" tachyon-build \
  cmake --build --preset linux-container -j$(nproc)
```

### 4. Run Tests

Execute the complete automated test suite:

```bash
docker run --rm -v "$PWD:/workspace" tachyon-build \
  ctest --test-dir build/linux-container --output-on-failure
```

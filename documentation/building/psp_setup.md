# PSP Build Setup

This fork targets the PSP in addition to the N64. Both toolchains live in the
same container image, so one environment builds the game assets, the N64
reference ROM, and the PSP `EBOOT.PBP`.

The N64 build is kept working on purpose: it is the visual reference the PSP
port is compared against.

## Prerequisites

Follow [Docker Build Setup](./docker_setup.md) first. The image now also
installs the [PSP toolchain](https://github.com/pspdev/pspdev), pinned to a
release so builds stay reproducible.

If you built the image before PSP support was added, rebuild it:

```sh
docker build -t portal64 .
```

## Game Files

The asset pipeline needs the Portal game files. Rather than copying ~6 GB into
the project, bind-mount them read-only at `vpk/Portal` when starting the
container:

```sh
docker run --rm \
    -v ./:/usr/src/app \
    -v <SteamLibrary>/steamapps/common/Portal:/usr/src/app/vpk/Portal:ro \
    -it portal64 bash
```

The symlink described in [`vpk/README.md`](../../vpk/README.md) is for native
builds only -- it points outside the container and will not resolve inside it.

## Verifying the Toolchains

Inside the container:

```sh
psp-gcc --version        # PSP
mips-n64-gcc --version   # N64
blender --version        # asset pipeline
vpk --help
```

`PSPDEV` must be set -- the PSP CMake toolchain fails loudly without it.

## Building

The N64 build is unchanged, see [Building the Game](./building.md):

```sh
cmake -G "Ninja" -B build/n64 -S . -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-N64.cmake
cmake --build build/n64
```

The PSP build uses its own toolchain file and its own build directory, so the
two never share a CMake cache:

```sh
cmake -G "Ninja" -B build/psp -S . -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchain-PSP.cmake
cmake --build build/psp
```

## Checking for N64 Regressions

Most PSP work touches shared `CMakeLists.txt` files, so every such change needs
a check that the N64 build is unaffected.

**Comparing ROM hashes does not work.** The build is not reproducible: building
the same tree twice produces two different ROMs. `skeletool64` emits its
declarations in a varying order, so roughly half the ROM's bytes change between
runs. Three builds of the same commit produced three distinct hashes.

Compare the generated build graph instead. It is deterministic, and it captures
exactly what a `CMakeLists.txt` change can affect -- which sources are compiled,
with which flags, into which targets:

```sh
# configure before and after the change into separate directories, then
sed 's|build/n64-after|BUILDDIR|g' build/n64-after/build.ninja > after.ninja
sed 's|build/n64-before|BUILDDIR|g' build/n64-before/build.ninja > before.ninja
diff before.ninja after.ninja
```

An empty diff means the N64 build is byte-for-byte unchanged. This only needs a
configure step, not a full build, so it takes seconds rather than minutes.

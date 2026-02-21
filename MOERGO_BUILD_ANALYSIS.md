# MoErgo-SC/ZMK Repository Analysis: Protoc/Nanopb & Build System

## Executive Summary

The moergo-sc/zmk repository has a sophisticated multi-layer build system using Nix for reproducible builds, with specific handling of the nanopb module and protoc binary. Unlike traditional Zephyr builds that rely on west and system-wide toolchains, MoErgo uses pinned nixpkgs to create isolated, reproducible build environments.

---

## 1. Nix Build Architecture Overview

### Key Files:
- `/default.nix` - Main entry point for Nix builds
- `/nix/pinned-nixpkgs.nix` - Frozen nixpkgs version
- `/nix/zmk.nix` - ZMK-specific build derivation
- `/nix/zephyr.nix` - Zephyr framework setup
- `/nix/manifest.json` - Frozen west dependencies with sha256 hashes
- `/release.nix` - Lambda container and CI image configuration

### Build Strategy:
- **Reproducible builds**: Uses pinned nixpkgs version (nixos-22.05 fork)
- **No west initialization**: Modules are pre-fetched and checksummed in Nix
- **Static environment**: All dependencies (gcc-arm-embedded, cmake, ninja, dtc, python) are pinned
- **No system dependencies**: Build environment is completely controlled by Nix

---

## 2. Protoc and Nanopb Handling

### Nanopb Module Configuration

**manifest.json (lines 11-16):**
```json
{
  "name": "nanopb",
  "url": "https://github.com/zmkfirmware/nanopb",
  "revision": "8c60555d6277a0360c876bd85d491fc4fb0cd74a",
  "path": "modules/lib/nanopb",
  "sha256": "1w77q47cvhg7xmfzbws4w2pn1zr74vh9lyzj0cf1p8gz0n2l3q1g"
}
```

**Key Points:**
- Uses a **custom ZMK fork** of nanopb (https://github.com/zmkfirmware/nanopb)
- Pinned to specific commit with Nix sha256 hash verification
- Installed to `modules/lib/nanopb`
- No special patchelf or binary manipulation needed

### How Nanopb Is Used in CMake

**app/CMakeLists.txt (lines ~112-125):**
```cmake
if (CONFIG_ZMK_STUDIO_RPC)
  # For some reason this is failing if run from a different sub-file.
  list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)

  include(nanopb)

  # Turn off the default nanopb behavior
  set(NANOPB_GENERATE_CPP_STANDALONE OFF)

  nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}
                        ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}/proto/zmk/studio.proto
                        ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}/proto/zmk/meta.proto
                        ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}/proto/zmk/core.proto
                        ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}/proto/zmk/behaviors.proto
                        ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}/proto/zmk/keymap.proto
  )

  target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
  target_sources(app PRIVATE ${proto_srcs} ${proto_hdrs})

  add_subdirectory(src/studio)
endif()
```

**Critical Configuration:**
- Uses Zephyr's built-in nanopb cmake module (`${ZEPHYR_BASE}/modules/nanopb`)
- Disables standalone behavior: `NANOPB_GENERATE_CPP_STANDALONE OFF`
- Proto files are in a separate `zmk-studio-messages` module (fetched separately)
- Generated sources added directly to compilation

### Protoc Binary Sourcing

**No explicit protoc handling in Nix files!**
- Protoc comes from the Zephyr module's `modules/nanopb` directory
- Zephyr's nanopb CMake module handles finding and using protoc
- In Nix environment: protoc is available through Zephyr's pinned dependencies
- **No patchelf or binary patching is applied**

---

## 3. Module Dependencies System

### Zephyr Module Setup (nix/zephyr.nix)

The system uses a two-tier approach:

**1. Module Definition (zephyr.nix):**
```nix
mkModule = { name, revision, url, sha256, ... }:
  stdenvNoCC.mkDerivation (finalAttrs: {
    name = "zmk-module-${name}";
    src = fetchgit {
      inherit name url sha256;
      rev = revision;
    };
    dontUnpack = true;
    dontBuild = true;
    installPhase = ''
      mkdir $out
      ln -s ${finalAttrs.src} $out/${name}
    '';
    passthru = {
      modulePath = "${finalAttrs.finalPackage}/${name}";
    };
  });
```

**Key characteristics:**
- Immutable git fetches with sha256 verification
- Creates symlink indirection for module paths
- No compilation, only structural preparation

**2. Required Zephyr Modules (zmk.nix, lines 49-56):**
```nix
requiredZephyrModules = [
  "cmsis" "hal_nordic" "tinycrypt" "lvgl" "picolibc" "segger" "cirque-input-module"
];

# Some Zephyr modules seemingly need a symlink indirection (modulePath),
# others don't (src).
# This is not the best way to fix it, but it works around the problem.
directZephyrModules = [ "cirque-input-module" ];

zephyrModuleDeps =
  let modules = lib.attrVals requiredZephyrModules zephyr.modules;
  in map (x: if builtins.elem x.src.name directZephyrModules then x.src else x.modulePath) modules;
```

**Note:** Nanopb is NOT explicitly listed in requiredZephyrModules - it comes through the Zephyr fork itself

---

## 4. CI/Workflow Configuration

### Primary CI Workflow: nix-build.yml

**Purpose:** Nix-native build pipeline (preferred method)

**Key Steps:**
```yaml
- uses: cachix/install-nix-action@v27
  with:
    nix_path: nixpkgs=channel:nixos-22.05
- uses: cachix/cachix-action@v15
  with:
    name: moergo-glove80-zmk-dev
    authToken: "${{ secrets.CACHIX_AUTH_TOKEN }}"
- name: Build ${{ matrix.board }} combined firmware
  run: nix-build -A ${{matrix.board}}_combined -o combined
```

**Important Details:**
- Uses `cachix` for caching compiled Nix derivations
- Builds for two boards: glove80, go60
- No interactive west needed
- Simple invocation: `nix-build -A <target>`

### Secondary Workflow: build-container.yml

**Purpose:** Build Lambda container with compilation service

**Key Components:**
```yaml
- uses: cachix/install-nix-action@v27
  with:
    nix_path: nixpkgs=channel:nixos-22.05
- name: Build lambda image
  run: nix-build release.nix --arg revision "\"${REVISION_TAG}\"" --arg firmwareVersion "\"${VERSION_NAME}\"" -A lambdaImage -o lambdaImage
```

**Outputs:**
- Builds OCI container image
- Pushes to Amazon ECR
- Uses `release.nix` to create reproducible build environment

### Third Workflow: build.yml (DISABLED)

**Status:** Marked with `if: ${{ false }}` - NOT ACTIVE

This was the original standard Zephyr build using west/cmake but has been **disabled in favor of Nix builds**.

---

## 5. Release Pipeline (release.nix)

### Lambda Build Environment

```nix
depsLayer = {
  name = "deps-layer";
  path = [ pkgs.ccache ];
  includes = zmk.buildInputs ++ zmk.nativeBuildInputs ++ zmk.zephyrModuleDeps;
};
```

**Includes all dependencies from zmk derivation:**
- cmake, ninja, dtc
- gcc-arm-embedded
- python with specific packages
- nanopb (via zephyr.modules)
- All Zephyr HAL modules

### Compilation Script Features

```nix
zmkCompileScript = let
  zmk' = zmk.override {
    gcc-arm-embedded = ccacheWrapper.override {
      unwrappedCC = pkgs.gcc-arm-embedded;
    };
  };
in pkgs.writeShellScriptBin "compileZmk" ''
  # ... compilation logic
  cmake -G Ninja -S ${zmk'.src}/app ${lib.escapeShellArgs zmk'.cmakeFlags} \
    "-DUSER_CACHE_DIR=/tmp/.cache" "-DBOARD=$board" \
    "-DBUILD_VERSION=${revision}" "-DDTS_EXTRA_CPPFLAGS=$firmwareFlags" \
    "''${cmakeExtraFlags[@]}"
  ninja
'';
```

**Optimization:**
- Wraps gcc-arm-embedded with ccache for faster rebuilds
- Pre-caches compilation results in Lambda layer
- Supports custom keymaps and kconfig files

---

## 6. Dependencies Overview

### Native Build Inputs (zmk.nix, line 110):
```nix
nativeBuildInputs = [ cmake ninja python dtc gcc-arm-embedded ];
```

### Runtime Build Inputs (zmk.nix, line 111):
```nix
buildInputs = [ zephyr ];
```

### Python Dependencies (zmk.nix, lines 36-47):
```nix
python = (buildPackages.python3.override { inherit packageOverrides; }).withPackages (ps: with ps; [
  pyelftools
  pyyaml
  canopen
  packaging
  progress
  anytree
  intelhex
  pykwalify
]);
```

**Note:** No explicit protoc binary in dependencies - it's part of the nanopb module from Zephyr

---

## 7. Key Differences from Standard ZMK Build

### Standard ZMK (west-based):
1. Initializes repo with `west init`
2. Fetches modules dynamically with `west update`
3. Relies on system-installed toolchain
4. Variable build environment between machines

### MoErgo-SC Build (Nix-based):
1. **Pre-computed manifest**: All modules frozen with sha256 hashes
2. **Immutable dependencies**: git fetches are checksummed and cached
3. **Reproducible environment**: Exact nixpkgs version pinned
4. **No toolchain installation**: Everything self-contained
5. **Faster CI**: Cachix caches compiled dependencies
6. **No patchelf needed**: Binary compatibility handled through Nix

### Nanopb/Protoc Specific Differences:
- **Standard**: Protoc installed system-wide or in west tools
- **MoErgo**: Protoc comes from Zephyr's pinned nanopb module, no installation step needed
- **Standard**: Config macro NANOPB_GENERATE_CPP might differ
- **MoErgo**: Explicitly sets `NANOPB_GENERATE_CPP_STANDALONE OFF`

---

## 8. West Manifest Patching

**nix/west-manifest.patch:**
Patches west's manifest freezing to work with Nix's prefetch workflow:

```patch
diff --git a/src/west/manifest.py b/src/west/manifest.py
@@ -1618,8 +1618,10 @@ class Manifest:
     def pdict(p):
         if not p.is_cloned():
-            raise RuntimeError(f'cannot freeze; project {p.name} '
-                               'is uncloned')
+            # For the purposes of exporting a frozen manifest for Nix, this
+            # is sufficient, as a package whose revision is not represented
+            # as a SHA will fail the prefetch.
+            return Project.as_dict(p)
```

**Purpose:** Allows exporting uncloned projects in manifest freeze, needed for Nix's `update-manifest` tool

---

## 9. Nanopb Integration Points

### Where Nanopb Modules Live:
1. **Zephyr fork**: https://github.com/zmkfirmware/nanopb
2. **Location in build**: `modules/lib/nanopb` (from manifest.json)
3. **CMake usage**: Loaded via `${ZEPHYR_BASE}/modules/nanopb`

### Proto File Source:
- Separate module: `zmk-studio-messages`
- Location: `modules/msgs/zmk-studio-messages`
- Contains: `proto/zmk/{studio,meta,core,behaviors,keymap}.proto`

### Conditional Compilation:
- Only compiled if `CONFIG_ZMK_STUDIO_RPC` is enabled
- Default: Disabled (keyboard firmware works without it)
- When enabled: Generates C++ code from proto definitions

---

## 10. Build Environment Configuration

### Pinned Nixpkgs Version:
```json
{
  "url": "https://releases.nixos.org/nixpkgs/nixpkgs-25.05pre716127.566e53c2ad75/nixexprs.tar.xz",
  "sha256": "182d5xq2w70znk61b8bn0cyq4jmp7vw239vmxbmsvv13zrjainbv"
}
```

**Details:**
- NixOS 25.05 pre-release (very recent)
- Ensures all package versions are consistent
- Can be updated with `nix/generate-pin.rb`

### CMake Configuration (zmk.nix, lines 89-108):
```nix
cmakeFlags = [
  "-DZEPHYR_BASE=${zephyr}/zephyr"
  "-DBOARD_ROOT=."
  "-DBOARD=${board}"
  "-DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb"
  "-DGNUARMEMB_TOOLCHAIN_PATH=${gcc-arm-embedded}"
  "-DCMAKE_C_COMPILER=${gcc-arm-embedded}/bin/arm-none-eabi-gcc"
  "-DCMAKE_CXX_COMPILER=${gcc-arm-embedded}/bin/arm-none-eabi-g++"
  "-DCMAKE_AR=${gcc-arm-embedded}/bin/arm-none-eabi-ar"
  "-DCMAKE_RANLIB=${gcc-arm-embedded}/bin/arm-none-eabi-ranlib"
  "-DZEPHYR_MODULES=${lib.concatStringsSep ";" zephyrModuleDeps}"
] ++ ...
```

---

## 11. Summary: How Protoc/Nanopb Works in MoErgo Build

### Complete Chain:

1. **Repository initialization (Nix)**:
   - `nix-build -A <target>` reads `default.nix`
   - Loads pinned nixpkgs version
   - Evaluates `zmk.nix` derivation

2. **Module fetching (Nix)**:
   - `zephyr.nix` reads `manifest.json`
   - Each module (including nanopb) is fetched via git with sha256 verification
   - Modules installed to `$out/<moduleName>`

3. **CMake configuration**:
   - ZMK's CMakeLists.txt detects `CONFIG_ZMK_STUDIO_RPC` flag
   - Adds `${ZEPHYR_BASE}/modules/nanopb` to CMAKE_MODULE_PATH
   - `include(nanopb)` loads Zephyr's nanopb CMake module

4. **Proto code generation**:
   - `nanopb_generate_cpp()` CMake function invoked
   - Protoc binary found within nanopb module
   - Proto files from `zmk-studio-messages` module processed
   - Generated C++ code compiled into firmware

5. **No special binary handling**:
   - Protoc executable is part of nanopb source
   - Works with Zephyr's build system directly
   - No patchelf or RPATH modifications needed
   - Permissions/execution maintained by Nix

### Advantages of MoErgo's Approach:

1. **Deterministic**: Same commit always builds identically
2. **No network dependencies**: All sources are checksummed
3. **Fast CI**: Cachix caches compiled dependencies
4. **Clear dependencies**: `manifest.json` is the source of truth
5. **No toolchain pollution**: Isolated from system state
6. **Easy updates**: `update-manifest` regenerates the manifest

---

## 12. Key Insights for Your Build System

### What MoErgo Does Differently:

1. **Manifest-driven**: Dependencies defined in `manifest.json` with sha256
2. **No west init**: Modules never cloned by west at build time
3. **Pre-computed**: All module paths/SHAs computed ahead of time
4. **Cacheable**: Each module is a separate Nix derivation
5. **Reproducible**: Same inputs always produce same outputs

### For Protoc Specifically:

- **MoErgo**: Uses Zephyr's pinned nanopb fork, no separate protoc binary
- **Standard**: Might pull protoc from system or separate binary package
- **Result**: MoErgo's approach is more reproducible and isolated

### Scaling Observation:

If you had protoc/nanopb issues in standard ZMK:
- Different system versions could have incompatible protoc
- Binary compatibility issues across architectures
- Dependency version mismatches with Zephyr's expectations

MoErgo avoids all of these by:
- Pinning everything to one Zephyr fork
- Using Nix's isolated environments
- Verifying all hashes at build time


# MoErgo-SC/ZMK: Critical File Locations & Relationships

## PROJECT STRUCTURE

```
moergo-sc/zmk/
├── default.nix                          # Main Nix entry point
├── release.nix                          # Lambda/container build config
├── README-NIX.md                        # Nix build documentation
│
├── nix/                                 # Nix build infrastructure
│   ├── pinned-nixpkgs.json             # Frozen nixpkgs version & hash
│   ├── pinned-nixpkgs.nix              # Loads pinned nixpkgs (JSON → Nix)
│   ├── zmk.nix                         # ZMK derivation definition
│   ├── zephyr.nix                      # Zephyr framework setup
│   ├── manifest.json                   # Frozen module list with sha256
│   ├── west-manifest.patch             # Patch for west.py to support Nix
│   ├── west-shell.nix                  # Development shell for west
│   ├── cmake-shell.nix                 # Development shell for cmake
│   ├── ccache.nix                      # ccache wrapper configuration
│   ├── semver.nix                      # Semantic versioning tool
│   ├── semver.patch.gz                 # Patch file for semver
│   └── update-manifest/                # Tool to regenerate manifest.json
│       ├── default.nix                 # Nix wrapper for update script
│       └── update-manifest.sh          # Bash script for updates
│
├── app/                                 # ZMK application source
│   ├── CMakeLists.txt                  # Main build config (nanopb @ line 112-125)
│   ├── Kconfig                         # Kernel configuration
│   ├── boards/                         # Board definitions
│   │   ├── arm/glove80/               # Glove80 keyboard
│   │   ├── arm/go60/                  # Go60 keyboard
│   │   └── ...
│   ├── src/                           # Application source code
│   │   ├── studio/                    # Studio RPC implementation
│   │   └── ...
│   └── include/                       # Application headers
│
├── .github/workflows/                 # GitHub Actions CI/CD
│   ├── nix-build.yml                  # Primary Nix build (ACTIVE)
│   ├── build-container.yml            # Lambda container build (ACTIVE)
│   ├── build.yml                      # West-based build (DISABLED)
│   ├── ble-test.yml
│   ├── build-user-config.yml
│   └── ...
│
├── docs/                              # Documentation
├── schema/                            # Hardware metadata schemas
├── lambda/                            # AWS Lambda compilation service
└── ...
```

---

## CRITICAL FILE RELATIONSHIPS

### Build Dependency Chain

```
1. nix-build default.nix
   ↓
2. default.nix imports ./nix/pinned-nixpkgs.nix
   ├─ Reads: nix/pinned-nixpkgs.json (URL + hash)
   └─ Returns: nixpkgs environment
   ↓
3. default.nix calls callPackage ./nix/zmk.nix
   ↓
4. zmk.nix specifies nativeBuildInputs and calls ./nix/zephyr.nix
   ↓
5. zephyr.nix reads ./nix/manifest.json
   ├─ For each module: fetchgit(url, rev, sha256)
   ├─ Creates: Nix derivation for module
   └─ Returns: Module paths + sources
   ↓
6. zmk.nix receives modules via zephyr.modules
   ├─ Pass to CMake: -DZEPHYR_MODULES=path1;path2;path3
   └─ Includes: nanopb module path
   ↓
7. CMake runs app/CMakeLists.txt
   ├─ Line 112: Appends nanopb path to CMAKE_MODULE_PATH
   ├─ Line 114: include(nanopb) loads CMake module
   ├─ Line 117: nanopb_generate_cpp() generates proto code
   └─ Protoc binary found in nanopb module
   ↓
8. Compilation of firmware
```

---

## KEY FILES & THEIR ROLES

### Nix Configuration Files

| File | Purpose | Key Content |
|------|---------|-------------|
| `default.nix` | Main entry point | Defines targets (glove80_left, go60_right, etc.) |
| `nix/pinned-nixpkgs.nix` | Loads frozen nixpkgs | Fetches tarball, returns pkgs |
| `nix/pinned-nixpkgs.json` | Nixpkgs version pin | URL + SHA256 hash |
| `nix/zmk.nix` | ZMK build rules | Dependencies, CMake flags, build steps |
| `nix/zephyr.nix` | Zephyr module builder | Creates derivations from manifest |
| `nix/manifest.json` | Frozen dependencies | Module list with sha256 hashes |
| `release.nix` | Release/container builds | Lambda image, compilation script |
| `nix/west-manifest.patch` | West patching | Allows frozen manifest export |

### Application Build Files

| File | Purpose | Protoc/Nanopb? |
|------|---------|---|
| `app/CMakeLists.txt` | Main build config | YES - nanopb setup @ lines 112-125 |
| `app/Kconfig` | Build options | CONFIG_ZMK_STUDIO_RPC enables proto |
| `app/west.yml` | Module manifest | Lists zephyr, nanopb, zmk-studio-messages |
| `app/boards/*/` | Board-specific config | Board definitions |

### CI/CD Workflows

| File | Status | Purpose |
|------|--------|---------|
| `.github/workflows/nix-build.yml` | ACTIVE | Primary Nix builds (glove80, go60) |
| `.github/workflows/build-container.yml` | ACTIVE | Lambda container builds |
| `.github/workflows/build.yml` | DISABLED | Original west-based build (reference) |
| `.github/workflows/build-user-config.yml` | ACTIVE | User firmware compilation |

---

## MANIFEST.JSON: THE DEPENDENCY SOURCE

### Structure (Excerpt)

```json
[
  {
    "name": "zephyr",
    "url": "https://github.com/zmkfirmware/zephyr",
    "revision": "f742ae36021571445b199a6b3c2cb3a6e3193833",
    "clone-depth": 1,
    "sha256": "0865rc77lvyvgipgpfcy7yfc7yv0y78ag2mlszli0rgxxp8fl54v"
  },
  {
    "name": "nanopb",
    "url": "https://github.com/zmkfirmware/nanopb",
    "revision": "8c60555d6277a0360c876bd85d491fc4fb0cd74a",
    "path": "modules/lib/nanopb",
    "sha256": "1w77q47cvhg7xmfzbws4w2pn1zr74vh9lyzj0cf1p8gz0n2l3q1g"
  },
  {
    "name": "zmk-studio-messages",
    "url": "https://github.com/zmkfirmware/zmk-studio-messages",
    "revision": "6cb4c283e76209d59c45fbcb218800cd19e9339d",
    "path": "modules/msgs/zmk-studio-messages",
    "sha256": "0yspr4ia6hxa29dy0v86qc8j3nvvnx2xx5qzsmnidpx9s233bz09"
  },
  ...
]
```

### Key Modules for Proto Support

1. **nanopb** (modules/lib/nanopb)
   - Source: https://github.com/zmkfirmware/nanopb
   - Contains: nanopb.cmake, protoc binary/script
   - CMake finds it at: ${ZEPHYR_BASE}/modules/nanopb

2. **zmk-studio-messages** (modules/msgs/zmk-studio-messages)
   - Source: https://github.com/zmkfirmware/zmk-studio-messages
   - Contains: Proto definitions (*.proto files)
   - CMake finds it at: ${ZEPHYR_ZMK_STUDIO_MESSAGES_MODULE_DIR}

---

## NMK.NIX: THE BUILD RECIPE

### Section: Dependencies (Line 110-111)

```nix
nativeBuildInputs = [ cmake ninja python dtc gcc-arm-embedded ];
buildInputs = [ zephyr ];
```

**What this means:**
- Tools needed to run the build: cmake, ninja, etc.
- At build time: zephyr module is available

### Section: Required Modules (Lines 49-56)

```nix
requiredZephyrModules = [
  "cmsis" "hal_nordic" "tinycrypt" "lvgl" "picolibc" "segger" "cirque-input-module"
];

directZephyrModules = [ "cirque-input-module" ];

zephyrModuleDeps =
  let modules = lib.attrVals requiredZephyrModules zephyr.modules;
  in map (x: if builtins.elem x.src.name directZephyrModules then x.src else x.modulePath) modules;
```

**Note:** nanopb is NOT in requiredZephyrModules - it comes via zephyr.modules automatically

### Section: CMake Flags (Lines 89-108)

```nix
cmakeFlags = [
  "-DZEPHYR_BASE=${zephyr}/zephyr"
  "-DBOARD_ROOT=."
  "-DBOARD=${board}"
  "-DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb"
  "-DGNUARMEMB_TOOLCHAIN_PATH=${gcc-arm-embedded}"
  "-DZEPHYR_MODULES=${lib.concatStringsSep ";" zephyrModuleDeps}"
  # ... more flags
];
```

**Zephyr sees:** All module paths concatenated with semicolons, including nanopb path

---

## CMakeLists.txt: NANOPB CONFIGURATION

### Lines 112-125: Nanopb Setup

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

**Critical Details:**
- Line 114: CMAKE_MODULE_PATH gets nanopb path
- Line 116: Loads nanopb.cmake from nanopb module
- Line 119: NANOPB_GENERATE_CPP_STANDALONE = OFF
- Lines 121-127: Proto files from zmk-studio-messages
- Line 129: Generated code added to build

---

## NIT/BUILD.YML: PRIMARY CI WORKFLOW

### Key Steps

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        board:
          - glove80
          - go60
    steps:
      - uses: actions/checkout@v4
      - uses: cachix/install-nix-action@v27
        with:
          nix_path: nixpkgs=channel:nixos-22.05
      - uses: cachix/cachix-action@v15
        with:
          name: moergo-glove80-zmk-dev
      - name: Build ${{ matrix.board }} combined firmware
        run: nix-build -A ${{matrix.board}}_combined -o combined
      - name: Upload result
        uses: actions/upload-artifact@v4
        with:
          name: ${{matrix.board}}.uf2
          path: ${{matrix.board}}.uf2
```

**What happens:**
1. Checkout code
2. Install Nix
3. Configure cachix (caches Nix derivations)
4. Run `nix-build -A glove80_combined` (builds left + right + combines)
5. Run `nix-build -A go60_combined` (same for Go60)
6. Upload .uf2 files

**No protoc installation needed** - it's in the manifest dependencies

---

## RELEASE.NIX: LAMBDA CONTAINER BUILDS

### Lambda Build Environment

```nix
depsLayer = {
  name = "deps-layer";
  path = [ pkgs.ccache ];
  includes = zmk.buildInputs ++ zmk.nativeBuildInputs ++ zmk.zephyrModuleDeps;
};
```

**What's included:**
- All zmk.nativeBuildInputs (cmake, ninja, gcc-arm-embedded, python, dtc)
- All zmk.buildInputs (zephyr module with all its dependencies)
- All zmk.zephyrModuleDeps (modules from manifest.json, including nanopb)

### Compilation Script

```nix
zmkCompileScript = ...
  cmake -G Ninja -S ${zmk'.src}/app ${lib.escapeShellArgs zmk'.cmakeFlags} \
    "-DUSER_CACHE_DIR=/tmp/.cache" "-DBOARD=$board" ...
  ninja
```

Same CMake flags as regular zmk.nix, including ZEPHYR_MODULES

---

## FILE MODIFICATION GUIDE

### To Update Dependencies:
1. Edit: `nix/manifest.json`
   - Update revision, sha256, or add new modules
   
2. Or run tool: `nix/update-manifest`
   - Automatically fetches and generates sha256 hashes

### To Change Build Configuration:
1. Edit: `nix/zmk.nix`
   - Change CMAKE_MODULE_PATH, dependencies, etc.

2. Or edit: `app/CMakeLists.txt`
   - Change what gets compiled (for app-level changes)

### To Add Protoc/Nanopb Features:
1. Update: `nix/manifest.json` - new nanopb revision
2. Update: `app/CMakeLists.txt` - new proto files
3. Update: Proto definitions in zmk-studio-messages module

---

## SUMMARY: THE COMPLETE FLOW

```
User runs: nix-build default.nix -A glove80_combined
    ↓
default.nix defines glove80_combined as zmk.override + combine_uf2
    ↓
zmk.override loads nix/zmk.nix with board="glove80_lh" or "glove80_rh"
    ↓
zmk.nix includes nix/zephyr.nix
    ↓
zephyr.nix reads nix/manifest.json
    ├─ Fetches nanopb: github.com/zmkfirmware/nanopb @ 8c60555d...
    ├─ Fetches zmk-studio-messages: github.com/zmkfirmware/zmk-studio-messages
    └─ Returns module paths to zmk.nix
    ↓
zmk.nix receives module paths, passes to CMake
    ├─ -DZEPHYR_MODULES=/nix/store/xxx-zephyr/zephyr/modules/lib/nanopb;...
    └─ Runs cmake + ninja
    ↓
app/CMakeLists.txt runs:
    ├─ Appends /nix/store/xxx-nanopb to CMAKE_MODULE_PATH
    ├─ include(nanopb) - loads /nix/store/xxx-nanopb/nanopb.cmake
    ├─ nanopb_generate_cpp() finds protoc in nanopb module
    ├─ Generates C++ from /nix/store/xxx-zmk-studio-messages/proto/*.proto
    └─ Compiles everything
    ↓
Firmware output: build/zephyr/zmk.uf2
    ↓
combine_uf2 combines left + right halves
    ↓
Output: glove80.uf2 or go60.uf2
```

All paths are deterministic, all versions are pinned, all hashes are verified.


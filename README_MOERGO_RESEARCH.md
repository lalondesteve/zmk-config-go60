# MoErgo-SC/ZMK Repository Research - Documentation Index

This directory contains comprehensive research and analysis of the moergo-sc/zmk repository's build system, with particular focus on how they handle protoc/nanopb, CI/CD workflows, and Nix-based reproducible builds.

## Documentation Files

### 1. **SEARCH_FINDINGS_SUMMARY.md** (START HERE)
   - **Length:** 13 KB
   - **Purpose:** Executive summary of all findings
   - **Best for:** Quick overview, answers to original questions
   - **Contains:**
     - Direct answers to all 5 original research questions
     - Key findings on protoc/nanopb handling
     - CI/workflow configuration overview
     - Build environment setup details
     - Architectural differences from standard ZMK
     - Complete build chain diagram

### 2. **MOERGO_QUICK_REFERENCE.md** (QUICK LOOKUP)
   - **Length:** 7.9 KB
   - **Purpose:** Quick reference guide for the build system
   - **Best for:** Looking up specific details, troubleshooting
   - **Contains:**
     - One-page architecture summary with flow diagram
     - Manifest structure
     - Critical build files with line numbers
     - Module dependency resolution
     - Build customization options
     - Reproducibility guarantees
     - Troubleshooting logic
     - Comparison table: Standard vs MoErgo approach

### 3. **MOERGO_FILE_LOCATIONS.md** (NAVIGATION GUIDE)
   - **Length:** 13 KB
   - **Purpose:** Complete file structure and relationships
   - **Best for:** Understanding how files connect, finding specific code
   - **Contains:**
     - Full project structure tree
     - Build dependency chain (detailed)
     - Key files table with purposes
     - manifest.json structure and key modules
     - zmk.nix sections breakdown
     - CMakeLists.txt nanopb setup (lines 112-125)
     - CI workflow details
     - File modification guide
     - Complete build flow with all file interactions

### 4. **MOERGO_BUILD_ANALYSIS.md** (COMPREHENSIVE REFERENCE)
   - **Length:** 14 KB
   - **Purpose:** Deep technical analysis of every aspect
   - **Best for:** Understanding the complete architecture, learning how it works
   - **Contains:**
     - 12 comprehensive sections
     - Nix build architecture overview
     - Detailed protoc/nanopb handling explanation
     - Module dependencies system (2-tier approach)
     - CI/workflow file configuration (all 3 workflows)
     - Release pipeline with Lambda details
     - Dependencies overview
     - Key differences from standard ZMK
     - West manifest patching explanation
     - Nanopb integration points
     - Build environment configuration
     - Reproducibility approach
     - Key insights for your build system

## Quick Start: What to Read

### If you have 5 minutes:
1. Read **SEARCH_FINDINGS_SUMMARY.md** - "ANSWERS TO ORIGINAL QUESTIONS"
2. Look at the build chain diagram at the end

### If you have 15 minutes:
1. Read **SEARCH_FINDINGS_SUMMARY.md** (full)
2. Skim **MOERGO_QUICK_REFERENCE.md** - sections "ONE-PAGE ARCHITECTURE SUMMARY" and "HOW PROTOC IS PROVIDED"

### If you have 30 minutes:
1. Read **SEARCH_FINDINGS_SUMMARY.md** (full)
2. Read **MOERGO_QUICK_REFERENCE.md** (full)
3. Skim **MOERGO_FILE_LOCATIONS.md** - "CRITICAL FILE RELATIONSHIPS"

### If you want complete understanding:
1. Read all four documents in order listed above
2. Reference **MOERGO_FILE_LOCATIONS.md** when you need to find specific code
3. Reference **MOERGO_BUILD_ANALYSIS.md** for deep technical details

## Key Findings Summary

### Protoc/Nanopb Handling
- **Not a separate binary** - part of nanopb module source
- **Fetched like any module** - git + sha256 verification
- **No patchelf needed** - CMake handles integration
- **Uses ZMK fork** - https://github.com/zmkfirmware/nanopb

### CI/CD Workflows
- **Primary:** nix-build.yml (ACTIVE) - Nix-based builds
- **Secondary:** build-container.yml (ACTIVE) - Lambda container builds
- **Reference:** build.yml (DISABLED) - Original west-based build

### Build System Approach
- **Manifest-driven** - all modules in manifest.json with sha256
- **No west at build time** - only for updates/manifest generation
- **Reproducible by design** - same commit = identical binary
- **Nix-based isolation** - no system dependencies

### Reproducibility Guarantees
- Nixpkgs version pinned
- All module commits pinned
- All sha256 hashes verified
- CMake flags identical
- Toolchain versions pinned
- **Result:** Same git commit → identical firmware across machines

## Technical Highlights

### Build Chain (Simplified)
```
nix-build
  ↓
default.nix (entry point)
  ↓
zmk.nix (build rules)
  ↓
zephyr.nix (module builder)
  ↓
manifest.json (frozen dependencies)
  ├─ nanopb module
  ├─ zmk-studio-messages
  └─ 28+ other modules
  ↓
CMakeLists.txt
  ├─ Finds protoc in nanopb
  ├─ Runs nanopb_generate_cpp()
  └─ Compiles firmware
  ↓
zmk.uf2 (firmware output)
```

### Key Files to Understand
1. **manifest.json** - Source of truth for all dependencies
2. **nix/zmk.nix** - Build recipe and dependency declaration
3. **nix/zephyr.nix** - Module fetching and setup
4. **app/CMakeLists.txt** (lines 112-125) - Nanopb integration
5. **.github/workflows/nix-build.yml** - Primary CI workflow

## Differences from Standard ZMK

| Aspect | Standard | MoErgo |
|--------|----------|--------|
| Build Command | `west build` | `nix-build` |
| Dependencies | Dynamic (west update) | Pre-computed (manifest.json) |
| Toolchain | System-wide | Nix-isolated |
| Protoc | System/bundled | Via nanopb module |
| Reproducibility | Variable | Guaranteed |
| CI Caching | Minimal | Cachix (fast) |

## Important Notes

### What They DON'T Do
- No patchelf for protoc
- No special binary patching
- No RPATH modifications
- No flake.nix (uses pinned-nixpkgs.nix instead)

### What They DO Do
- Use manifest.json for complete dependency pinning
- Patch west.py to enable frozen manifest export
- Wrap cc-like binaries with ccache
- Cache build artifacts in Cachix
- Build Lambda/container images for deployment

### West in Their System
- **For building:** Never used (`nix-build` instead)
- **For updating:** `west update` generates manifest
- **For freezing:** `west manifest --freeze` then `nix-prefetch-git`
- **Result:** manifest.json with all sha256 hashes

## Research Methodology

This research was conducted by:
1. Cloning moergo-sc/zmk repository
2. Examining all files in nix/ directory
3. Reading CMakeLists.txt nanopb configuration
4. Analyzing CI workflow files (.github/workflows/)
5. Studying manifest.json structure
6. Reading release.nix for Lambda builds
7. Checking for patchelf/fixup references
8. Analyzing dependency resolution chain

**No source code was modified.** This is purely investigative documentation.

## Files in This Research

- `/tmp/moergo-zmk/` - Cloned repository (temporary)
- `SEARCH_FINDINGS_SUMMARY.md` - This research's summary
- `MOERGO_QUICK_REFERENCE.md` - Quick lookup guide
- `MOERGO_FILE_LOCATIONS.md` - File structure and relationships
- `MOERGO_BUILD_ANALYSIS.md` - Deep technical analysis

## Next Steps for Your Build System

Based on this research, consider:

1. **If implementing Nix builds:** Look at manifest.json as the model
2. **If handling protoc:** Keep it part of the source, not separate binary
3. **If setting up CI:** Use cachix for derivative caching
4. **If ensuring reproducibility:** Pin all versions (nixpkgs, modules, sha256)
5. **If using west:** Separate "update" from "build" steps

## Questions Answered

### Original Research Questions:
1. How they handle protoc/nanopb binary in Nix build?
   - Answer: Part of nanopb module, no special handling needed

2. CI/workflow files configuration?
   - Answer: Three workflows, primary is nix-build.yml

3. Special provisions/patches for nanopb?
   - Answer: No nanopb-specific patches, only west-manifest.patch

4. Setup files (setup.nix, shell.nix, flake.nix)?
   - Answer: default.nix, zmk.nix, zephyr.nix, west-shell.nix, cmake-shell.nix

5. Documentation about building with Nix?
   - Answer: README-NIX.md, plus this detailed research

## Contact & Attribution

This research was conducted as part of investigating build system architectures for keyboard firmware.

- **Repository:** https://github.com/moergo-sc/zmk
- **Research Date:** February 21, 2026
- **Focus:** Build system architecture, protoc/nanopb handling, reproducibility

---

**Total Documentation:** 48 KB across 4 comprehensive markdown files
**All Information:** Derived from public repository analysis only


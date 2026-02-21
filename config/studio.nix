{ pkgs ?  import <nixpkgs> {}
, firmware ? import ../src {}
}:

let
  inherit (pkgs.lib) optional concatStringsSep;
  config = ./.;
  
  zephyr = firmware.zephyr;
  gcc-arm-embedded = pkgs.gcc-arm-embedded;
  
  python = (pkgs.python3.withPackages (ps: with ps; [
    pyelftools pyyaml canopen packaging progress anytree intelhex pykwalify
  ]));

  # The zmkfirmware/nanopb fork bundles pre-built ELF binaries (protoc,
  # protoc-gen-nanopb) whose interpreter path does not exist in the pure Nix
  # sandbox.  We create a patched copy of the module where those binaries are
  # rewritten by patchelf to use the glibc/libstdc++ from our nixpkgs.
  nanopb-patched = pkgs.runCommandNoCC "zmk-module-nanopb-patched"
    {
      nativeBuildInputs = [ pkgs.patchelf ];
      src = zephyr.modules.nanopb.modulePath;
      interpreter = "${pkgs.glibc}/lib/ld-linux-x86-64.so.2";
      rpath = pkgs.lib.makeLibraryPath [
        pkgs.glibc
        pkgs.stdenv.cc.cc.lib
      ];
    }
    ''
      cp -r --no-preserve=mode "$src" "$out"
      for bin in protoc protoc-gen-nanopb; do
        bin_path="$out/generator/$bin"
        if [ -f "$bin_path" ]; then
          patchelf \
            --set-interpreter "$interpreter" \
            --set-rpath      "$rpath" \
            "$bin_path"
        fi
      done
    '';

  allModules = [
    "${zephyr.modules.cmsis.modulePath}"
    "${zephyr.modules.hal_nordic.modulePath}"
    "${zephyr.modules.tinycrypt.modulePath}"
    "${zephyr.modules.lvgl.modulePath}"
    "${zephyr.modules.picolibc.modulePath}"
    "${zephyr.modules.segger.modulePath}"
    "${zephyr.modules.cirque-input-module.src}"
    "${nanopb-patched}"
    "${zephyr.modules.zmk-studio-messages.modulePath}"
  ];

  buildZmk = { board, kconfig, snippets ? [] }:
    pkgs.stdenvNoCC.mkDerivation {
      name = "zmk_${board}";

      sourceRoot = "source/app";

      src = builtins.path {
        name = "source";
        path = ../src;
        filter = path: type:
          let relPath = pkgs.lib.removePrefix (toString ../src + "/") (toString path);
          in (pkgs.lib.cleanSourceFilter path type) && ! (
            relPath == "nix" || pkgs.lib.hasSuffix ".nix" path ||
            relPath == "build" || relPath == ".west" ||
            relPath == "modules" || relPath == "tools" || relPath == "zephyr" ||
            relPath == "lambda" || relPath == ".github"
          );
      };

      preConfigure = ''
        cmakeFlagsArray+=("-DUSER_CACHE_DIR=$TEMPDIR/.cache")
      '';

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
        "-DZEPHYR_MODULES=${concatStringsSep ";" allModules}"
        "-DKEYMAP_FILE=${config}/go60.keymap"
        "-DEXTRA_CONF_FILE=${kconfig}"
      ] ++ optional (snippets != []) "-DSNIPPET=${concatStringsSep ";" snippets}";

      nativeBuildInputs = [ pkgs.cmake pkgs.ninja python pkgs.dtc gcc-arm-embedded ];
      buildInputs = [ zephyr ];

      installPhase = ''
        mkdir $out
        cp zephyr/zmk.{uf2,hex,bin,elf} $out
        cp zephyr/.config $out/zmk.kconfig
        cp zephyr/zephyr.dts $out/zmk.dts
      '';
    };

  go60_left_studio = buildZmk {
    board = "go60_lh";
    kconfig = "${config}/go60-studio.conf";
    snippets = [ "studio-rpc-usb-uart" ];
  };

  go60_right = firmware.zmk.override { 
    board = "go60_rh"; 
    keymap = "${config}/go60.keymap"; 
    kconfig = "${config}/go60.conf";
  };

in firmware.combine_uf2 go60_left_studio go60_right "go60"

{
  description = "barli - a lightweight status bar";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };
  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { 
        inherit system;
      };
      mkBarli = stdenv: xorg: stdenv.mkDerivation {
        pname = "barli";
        version = "0.1.0";
        src = ./.;
        buildInputs = [ xorg.libX11 ];
        buildPhase = ''
          mkdir -p build
          $CC -Os -s -flto -fdata-sections -ffunction-sections \
            -fno-asynchronous-unwind-tables -fno-unwind-tables \
            -fno-stack-protector -Wl,--gc-sections \
            barli.c -o build/barli -lX11
          strip --strip-all build/barli
        '';
        installPhase = ''
          mkdir -p $out/bin
          cp build/barli $out/bin/barli
        '';
        hardeningDisable = [ "all" ];
        dontStrip = false;
      };
    in
    {
      packages.${system} = {
        default = mkBarli pkgs.stdenv pkgs.xorg;
        musl = mkBarli pkgs.pkgsMusl.stdenv pkgs.pkgsMusl.xorg;
      };
      
      devShells.${system}.default = pkgs.mkShell {
        buildInputs = [
          pkgs.gcc
          pkgs.xorg.libX11
        ];
      };
    };
}

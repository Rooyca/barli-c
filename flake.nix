{
  description = "barli - a lightweight status bar";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      
      buildBarli = { stdenv, xorg }:
        stdenv.mkDerivation {
          pname = "barli";
          version = "0.1.0";

          src = ./.;

          buildInputs = [ xorg.libX11 ];

          buildPhase = ''
            runHook preBuild

            mkdir -p build
            $CC -Os -flto -fdata-sections -ffunction-sections \
              -fno-asynchronous-unwind-tables -fno-unwind-tables \
              -Wl,--gc-sections -Wl,--as-needed \
              barli.c -o build/barli -lX11

            runHook postBuild
          '';

          installPhase = ''
            runHook preInstall

            mkdir -p $out/bin
            install -Dm755 build/barli $out/bin/barli

            runHook postInstall
          '';

          hardeningDisable = [ "fortify" ];

          meta = with pkgs.lib; {
            description = "A lightweight X11 status bar";
            license = licenses.mit;
            platforms = platforms.linux;
          };
        };
    in
    {
      packages.${system} = {
        default = buildBarli {
          inherit (pkgs) stdenv xorg;
        };

        musl = buildBarli {
          inherit (pkgs.pkgsMusl) stdenv xorg;
        };
      };

      devShells.${system}.default = pkgs.mkShell {
        buildInputs = with pkgs; [
          gcc
          xorg.libX11
        ];

        shellHook = ''
          echo "barli development environment"
          echo "Build with: gcc -Os barli.c -o barli -lX11"
        '';
      };

      apps.${system}.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/barli";
      };
    };
}

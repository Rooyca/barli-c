default:
    @just --list

build-nix:
    nix build

build-nix-musl:
    nix build .#musl

build:
    gcc -Os -s -flto -fdata-sections -ffunction-sections \
        -fno-asynchronous-unwind-tables -fno-unwind-tables \
        -fno-stack-protector -Wl,--gc-sections \
        barli.c -o barli -lX11
    strip --strip-all barli

install: build
    mkdir -p ~/.local/bin
    cp barli ~/.local/bin/barli
    chmod +x ~/.local/bin/barli
    echo "Installed to ~/.local/bin/barli"

uninstall:
    rm -f ~/.local/bin/barli
    @echo "Uninstalled from ~/.local/bin/barli"

clean:
    rm -rf result result-* barli
    @echo "Cleaned build artifacts"

run: build
    ./barli

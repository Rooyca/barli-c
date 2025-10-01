# barli

**barli** is a tiny, lightweight status bar for X11.  
It runs shell commands (or plain executables) at given intervals and updates the X11 root window name with their results — perfect for minimal WMs like `dwm` or `xmonad`.

---

## Features
- Simple text-based configuration file (`~/.config/barli.conf`).  
- Runs commands periodically and displays their output.  
- Supports both plain commands and full shell commands.  

---

## Configuration

Each line in the config defines a task:

```
prefix :: command :: suffix :: [shell]
````

- **prefix** → Text shown before command output  
- **command** → The command to run  
- **suffix** → Text shown after command output  
- **shell** → Optional, set to `shell` to run inside `/bin/sh -c`  

### Example (`~/.config/barli.conf`)
```txt
SLEEP_TIME: 3
Clock :: date :: :: 
Mem :: free -h | awk 'NR==2{print $3}' :: used :: shell
````

---

## Installation

Use [`just`](https://github.com/casey/just) to build, install, and manage **barli**:

```bash
# Install to ~/.local/bin (no sudo needed)
just install

# Uninstall from ~/.local/bin
just uninstall

# Build and run for testing
just run
```

---

## Usage

Just run:

```bash
barli &
```

The bar text is stored in the root window name, so it will automatically be picked up by your WM’s status bar.


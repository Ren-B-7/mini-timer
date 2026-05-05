# mini-timer

A small GTK-3 based C timer that functions as both a CLI tool and a graphical application.

## Features

- **Dual Interface:** Use it from the command line for quick timer settings or the GUI for desktop monitoring.
- **Minimalist Design:** Built to be lightweight and efficient.
- **Theming:** Easily customizable look and feel using standard CSS via GTK resources.
- **Robustness:** Built with strict safety standards to ensure reliability.

## Usage

### CLI Mode

You can trigger the timer directly from your terminal:

```bash
# Set a timer for 10 minutes
timer 10

# Note: Further CLI flags and functionality are being expanded.
```

### GUI Mode

Running the command without arguments launches the graphical interface:

```bash
timer
```

## Build & Security

The project emphasizes code quality and security. The `Makefile` includes extensive compiler flags (`-Wall`, `-Wextra`, `-pedantic`, etc.) and modern hardening techniques.

### Build Options

- `make all`: Standard clean build.
- `make install`: Installs the binary to `$(HOME)/.local/bin/timer`.
- `make uninstall`: Removes the binary from `$(HOME)/.local/bin/timer`.
- `make clean`: Cleans build artifacts.

## License

This project is licensed under the MIT License.

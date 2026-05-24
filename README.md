# tuner

Chromatic guitar tuner for macOS and Linux.

`tuner` utilizes PortAudio for real-time microphone input, processes the audio using a Fast Fourier Transform (FFT) combined with a second-order low-pass filter, and maps the peak frequency component to the nearest musical note, calculating the cents sharp/flat dynamically in the terminal.

## Requirements

- **OS**: macOS or Linux.
- **Dependencies**:
  - `portaudio` (audio input backend)
  - `pkg-config` (compilation helper)

### Installing Dependencies

- **macOS (Homebrew)**:
  ```bash
  brew install portaudio pkg-config
  ```
- **Linux (Ubuntu/Debian)**:
  ```bash
  sudo apt-get install portaudio19-dev pkg-config
  ```

## Getting Started

1. Build the application:
   ```bash
   make
   ```
2. Run the tuner:
   ```bash
   make run
   ```
   *(Or execute `./tuner` directly)*

## Makefile Targets

- `make`: Compile the chromatic tuner binary.
- `make run`: Compile (if needed) and execute the tuner.
- `make clean`: Remove object files and the tuner binary.
- `make install`: Install the binary globally to `/usr/local/bin`.
- `make uninstall`: Remove the binary from `/usr/local/bin`.
- `make help`: List build targets.

## License & Credits

- Licensed under MIT.
- Forked and modernized by Jake Garrison.
- Original project by Bjorn Roche.

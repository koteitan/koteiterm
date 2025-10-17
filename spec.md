# koteiterm - Linux Terminal Emulator Specification

## Overview
A modern terminal emulator for Linux (WSL) environments, similar to xterm, providing a graphical interface for shell interaction.

## Target Environment
- **Platform**: Linux (WSL2)
- **Display Server**: X11 or Wayland
- **Programming Language**: C/C++ (recommended for performance and compatibility)
- **Architecture**: Native Linux application

## Core Features

### 1. Terminal Emulation
- **VT100/VT102 compatibility**: Support basic ANSI escape sequences
- **xterm-256color support**: 256-color palette
- **UTF-8 encoding**: Full Unicode support for international characters
- **Terminal size**: Configurable rows and columns (default: 80x24)
- **Scrollback buffer**: Configurable history (default: 1000 lines)

### 2. Shell Integration
- **PTY (Pseudo-Terminal) support**: Fork/exec shell process with PTY
- **Default shell**: Read from $SHELL environment variable (fallback: /bin/bash)
- **Signal handling**: Proper SIGCHLD, SIGWINCH handling
- **Window resize**: Communicate terminal size changes to shell

### 3. User Interface
- **Window manager**: Use X11/Wayland for window creation
- **Font rendering**: Monospace font with configurable size
- **Text rendering**:
  - Anti-aliased text rendering
  - Proper character cell alignment
  - Support for bold, italic, underline
- **Color support**:
  - 16 basic ANSI colors
  - 256-color palette
  - True color (24-bit RGB) support
- **Cursor styles**: Block, underline, beam (configurable)
- **Cursor blinking**: Configurable on/off

### 4. Input Handling
- **Keyboard input**:
  - Standard ASCII characters
  - Special keys (arrows, function keys, etc.)
  - Modifier keys (Ctrl, Alt, Shift combinations)
  - IME support for Asian languages
- **Mouse input**:
  - Text selection with mouse
  - Copy to clipboard
  - Paste from clipboard
  - Optional mouse reporting to applications

### 5. Display Features
- **Text selection**:
  - Mouse-based selection
  - Copy on select (optional)
  - Rectangular selection (Alt+drag)
- **Clipboard integration**:
  - Copy/paste with Ctrl+Shift+C/V
  - Middle-click paste
- **URL detection**: Clickable URLs (Ctrl+click to open)
- **Scrolling**:
  - Scrollbar (optional)
  - Shift+PageUp/PageDown
  - Mouse wheel support

## Technical Architecture

### Component Structure
```
koteiterm/
├── src/
│   ├── main.c              # Entry point, initialization
│   ├── terminal.c/h        # Terminal emulator core (VT parsing)
│   ├── pty.c/h             # PTY handling, shell process management
│   ├── display.c/h         # X11/Wayland window and rendering
│   ├── font.c/h            # Font loading and text rendering
│   ├── input.c/h           # Keyboard and mouse input handling
│   ├── config.c/h          # Configuration management
│   └── utils.c/h           # Utility functions
├── include/
│   └── koteiterm.h         # Common headers and definitions
├── config/
│   └── koteiterm.conf      # Default configuration file
├── tests/
│   └── (unit tests)
├── docs/
│   └── README.md
├── Makefile
└── spec.md (this file)
```

### Dependencies
- **libX11**: X11 window management
- **libXft** or **libfreetype**: Font rendering
- **libfontconfig**: Font discovery
- **libutil**: PTY utilities (openpty, forkpty)
- **libpthread**: Threading support (optional, for rendering)

### Key Algorithms

#### VT100 Escape Sequence Parser
- State machine-based parser
- Handle CSI (Control Sequence Introducer) sequences
- Support for SGR (Select Graphic Rendition) color/style codes
- Cursor movement commands
- Screen manipulation (clear, scroll)

#### PTY Communication
```
Parent Process (koteiterm)  ←→  PTY Master
                                    ↕
                                PTY Slave  ←→  Child Process (shell)
```

#### Rendering Pipeline
1. Read data from PTY master
2. Parse escape sequences and update terminal state
3. Update internal screen buffer (2D character array)
4. Render dirty regions to X11 window
5. Handle vsync and refresh rate

## Configuration

### Configuration File Format
- Location: `~/.config/koteiterm/koteiterm.conf` or `/etc/koteiterm/koteiterm.conf`
- Format: INI-style or simple key-value pairs

### Configurable Parameters
```ini
[terminal]
rows = 24
columns = 80
scrollback = 1000
shell = /bin/bash

[appearance]
font = monospace
font_size = 12
cursor_style = block
cursor_blink = true

[colors]
foreground = #ffffff
background = #000000
color0 = #000000
color1 = #cc0000
; ... (16 colors)

[behavior]
copy_on_select = false
scrollbar = true
url_click = true
```

## Development Phases

### Phase 1: Minimal Viable Product (MVP)
- Basic PTY creation and shell execution
- Simple X11 window with basic text rendering
- VT100 escape sequence parsing (subset)
- Basic keyboard input
- Fixed 80x24 terminal size

### Phase 2: Enhanced Features
- Full VT100/xterm compatibility
- 256-color support
- Window resizing
- Scrollback buffer
- Text selection and clipboard

### Phase 3: Polish and Optimization
- Configuration file support
- Font selection and scaling
- Performance optimization
- IME support
- URL detection and clicking

### Phase 4: Advanced Features
- Tab support (multiple terminals)
- Split panes
- Transparency and compositing
- Custom key bindings
- Themes and appearance customization

## Testing Strategy
- **Unit tests**: Test individual components (parser, PTY handling)
- **Integration tests**: Test terminal output with known escape sequences
- **Manual testing**: Run common programs (vim, htop, less, etc.)
- **Compatibility testing**: Compare with xterm behavior

## Performance Goals
- **Startup time**: < 100ms
- **Input latency**: < 10ms
- **Rendering**: 60 FPS minimum
- **Memory usage**: < 50MB for single terminal instance

## Standards and References
- **ECMA-48**: Control Functions for Coded Character Sets
- **VT100 User Guide**: DEC VT100 terminal specifications
- **xterm control sequences**: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
- **POSIX PTY**: IEEE Std 1003.1 pseudo-terminal specifications

## Future Considerations
- Wayland native support (in addition to X11)
- GPU-accelerated rendering (OpenGL/Vulkan)
- Sixel graphics support
- Ligature support for programming fonts
- Extension API for plugins

## License
TBD (suggest MIT or GPL)

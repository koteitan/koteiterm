# winclip.exe - Windows Clipboard Utility

A simple Windows utility for accessing the clipboard from WSL/Linux with proper UTF-8 support.

## Building on Windows

### Using MinGW-w64 (WSL):
```bash
x86_64-w64-mingw32-gcc -o winclip.exe winclip.c -municode
```

### Using Visual Studio:
```cmd
cl /Fe:winclip.exe winclip.c
```

### Using MSVC from WSL:
```bash
cl.exe /Fe:winclip.exe winclip.c
```

## Usage

### Get clipboard content (UTF-8 output):
```bash
winclip.exe get
```

### Set clipboard content (UTF-8 input):
```bash
echo "テキスト" | winclip.exe set
```

## Features

- Direct Windows API calls (no PowerShell overhead)
- Proper UTF-8 ↔ UTF-16 conversion
- Binary-safe I/O
- Fast and secure (no shell injection risks)

## Installation

Copy `winclip.exe` to a directory in your PATH, or place it in the koteiterm directory.

# pipimWatcher
 A lightweight Windows desktop application that displays PPM images and automatically reloads them when the file changes on disk. Drag &amp; drop support with real-time updates.

## Features

- **Drag & Drop** - Simply drag a PPM file onto the window to display it
- **Auto-Reload** - Watches the file and updates the display when the file changes
- **Aspect Ratio** - Maintains correct aspect ratio while fitting the window
- **PPM Support** - Handles both ASCII (P3) and binary (P6) PPM formats
- **Native Windows GUI** - Built with Win32 API for minimal dependencies

## Missing
- **PPM 6** binary support

## Screenshots

# Building

### Using Make (MinGW)

```bash
make
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
<img width="491" height="550" alt="image" src="https://github.com/user-attachments/assets/5117f2c6-ddcd-4fb7-b751-4deffdaacce2" />
<img width="491" height="550" alt="image" src="https://github.com/user-attachments/assets/53d73573-26c4-4c40-aad5-b234f85640c8" />
<img width="600" height="300" alt="image" src="https://github.com/user-attachments/assets/1ae5b1b2-8dcc-479e-a36c-150a0487f696" />

# Building

### Using Make (MinGW)

```bash
make

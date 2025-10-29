```
===============================================================================================
                      ##     #####    #####    ######   ####    #     ##
                     #  #    #    #   #    #   #       #        #    #  #
                    #    #   #    #   #    #   #####    ####    #   #    #
                    ######   #####    #    #   #            #   #   ######
                    #    #   #   #    #    #   #       #    #   #   #    #
                    #    #   #    #   #####    ######   ####    #   #    #
===============================================================================================
```

# Ardesia


> **Free digital sketchpad for screen annotation and presentation**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)](https://github.com/pilollipietro/ardesia)

Ardesia is the free digital sketchpad software that helps you make colored free-hand annotations with digital ink everywhere, record it and share on the network. It is easy to use and impressively fast and reactive. You can draw upon the desktop or import an image and annotate it and redistribute your work to the world.

Thanks to Ardesia you are free to open any application and fix your ideas and comments as if you wrote on a classic chalkboard. **Tradition and innovation are living together in simplicity** thanks to a natural user interface that reproduces the natural feeling of free-hand painting.

---

## 🎯 Use Cases

- **Presentations** - Highlight and annotate during live demos
- **Teaching** - Enhance lessons with smartboards and interactive whiteboards and tablet
- **Tutorials & Demos** - Create annotated screencasts
- **Brainstorming** - Quick sketches and artwork on any application
- **Remote Collaboration** - Share annotated screen via network streaming

---

## ✨ Features

- 🎨 **Free-hand drawing** with digital ink
- 📐 **Shape recognizer** - automatically converts sketches to geometric shapes
- 🖊️ **Multiple tools** - pen, highlighter, eraser, text, arrow, filler
- 🎨 **Color palette** - quick access colors plus custom picker
- ↩️ **Undo/Redo** - full annotation history
- 📸 **Export** - screenshot to image or multi-page PDF
- 🎥 **Recording** - capture and stream your annotated screen
- 🖥️ **Multi-monitor** - configurable toolbar and workspace placement
- 🎯 **Universal compatibility** - works with mouse, touchscreen, drawing tablets, Wiimote whiteboards, interactive projectors
- ⚙️ **Customizable** - configurable appearance and toolbar position
- 📂 **Open formats** - imports/exports IWB (Interactive Whiteboard) files

**Built with:** GTK 3.x (Glade/GtkBuilder) and Cairo graphics library

---

## 📋 System Requirements

### Linux
- **Display server:** X11 with composite manager (compiz, kwin, xcompmgr, xfwm, metacity, mutter) *or* Wayland (via XWayland)
- **Libraries:** GTK 3.x, Cairo, GLib 2.0
- **Optional:** VLC (for recording), xdg-utils

### Windows (Legacy Support)
⚠️ *Note: Windows support is legacy and may need updates for modern versions*
- Windows 7 (Ultimate, Enterprise, Professional, Home Premium)
- Windows Vista (Business, Enterprise, Ultimate, Home Premium)

---

## 🚀 Installation

See the [INSTALL](INSTALL) file for detailed build instructions.

### Quick Setup

**VLC for Recording (Optional):**

Debian/Ubuntu:
```bash
sudo apt-get install vlc
```

Other distros: Use your package manager or download from [videolan.org/vlc](http://www.videolan.org/vlc)

**xdg-utils (Usually pre-installed):**

Debian/Ubuntu:
```bash
sudo apt-get install xdg-utils
```

### Running

After installation, start Ardesia from:
- Application menu: **Applications → Tools → Ardesia**
- Command line: `ardesia`

---

## 🎮 Usage Guide

### Toolbar Overview

The Ardesia toolbar appears at the screen edge (default: right side). Click buttons to select tools and options.

#### Drawing Tools
| Tool | Description |
|------|-------------|
| **Pen** | Draw free-hand lines with selected color |
| **Highlighter** | Semi-transparent highlighting |
| **Eraser** | Remove annotations |
| **Text** | Insert text annotations (click to place) |
| **Arrow** | Draw lines with arrow at the end |
| **Filler** | Fill contiguous area with selected color |

#### Appearance Controls
| Control | Description |
|---------|-------------|
| **Thickness** | Cycle through thin/medium/thick line widths |
| **Colors** | Quick select: Blue, Green, Yellow, Red, White |
| **Color Selector** | Open full color palette |

#### Drawing Modes

Change how Ardesia interprets your strokes:

- **Handwriting** - Draw freely without assistance
- **Rectifier** - Converts closed paths to polygons, open paths to straight lines
- **Rounder** - Converts closed paths to ellipses/smoothed shapes, open paths to spline curves

💡 *Tip: Rectifier is perfect for diagrams, Rounder for organic shapes*

#### Actions
| Button | Function |
|--------|----------|
| **Lock/Unlock** | Toggle annotation mode. When unlocked, interact with desktop normally |
| **Clear** | Erase all annotations from screen |
| **Undo** | Reverse to previous state |
| **Redo** | Advance to more recent state |
| **Screenshot** | Export current view to image file |
| **Export PDF** | Add current view as new page to PDF (creates or appends) |
| **Record** | Start/stop desktop recording (requires VLC) |
| **Preferences** | Configure background color/image |
| **Info** | Show program information |
| **Quit** | Exit Ardesia |

### Advanced Features

#### Background Customization

Set custom backgrounds via Preferences:
- Choose background color
- Select from preset backgrounds
- Load custom image file

#### IWB File Support

Reuse Interactive Whiteboard projects:
1. Navigate to Ardesia workspace folder
2. Right-click `.iwb` file
3. Select "Open with Ardesia"

#### Toolbar Customization

**Position:**
```bash
ardesia --gravity north    # Top
ardesia --gravity south    # Bottom
ardesia --gravity west     # Left
ardesia --gravity east     # Right (default)
```

**Appearance:**
- Copy `desktop/gtkrc` to `/etc/xdg/ardesia/`
- Edit to customize look and feel

---

## ⚙️ Command Line Options

```
Usage: ardesia [OPTIONS] [filename.iwb]

Options:
  -V, --verbose              Enable verbose logging
  -d, --decorate             Add window decorations (allows moving toolbar)
  -g, --gravity POSITION     Toolbar position: east|west|north|south
  -l, --leftmargin PIXELS    Text left margin after Enter
  -t, --tabsize PIXELS       Tab size in text mode
  -c, --coverage MODE        Fit to: monitor|area|full
  -m, --tools-monitor N      Monitor for toolbar (default: 1)
  -M, --workspace-monitor N  Monitor for main window (default: 1)
  -x X_POSITION              Main window X position (default: 0)
  -y Y_POSITION              Main window Y position (default: 0)
  --width WIDTH              Main window width (default: 200)
  --height HEIGHT            Main window height (default: 200)
  -o, --opaque               Force opaque window (no transparency)
  -h, --help                 Show help screen
  -v, --version              Show version information

Arguments:
  filename                   Interactive Whiteboard Common File (.iwb)

Examples:
  ardesia                         # Start with defaults
  ardesia -g south                # Horizontal toolbar at bottom
  ardesia -d                      # Movable toolbar window
  ardesia -m 2 -M 1              # Toolbar on monitor 2, workspace on monitor 1
  ardesia --verbose              # Debug mode
  ardesia presentation.iwb       # Open IWB file
```

---

## 🎥 Recording & Streaming

### Local Recording

1. Ensure VLC is installed
2. Click **Record** button in Ardesia
3. Choose output filename on first use
4. Recording starts automatically
5. Click **Record** again to stop

### Live Streaming (Advanced)

Stream to Icecast2 server for live presentations:

**Requirements:**
- Icecast2 server (public IP + domain for internet access)
- Local machine with Ardesia and network access to server

**Setup:**

Edit screencast configuration:
- Linux: `$PREFIX/share/ardesia/scripts/screencast.sh`
- Windows: `share\ardesia\scripts\screencast.bat`

Uncomment and configure:
```bash
ICECAST="TRUE"
ICECAST_PASSWORD="your-password"
ICECAST_ADDRESS="icecast.server.com"
ICECAST_PORT="8000"
ICECAST_MOUNTPOINT="presentation"
```

**Usage:**
1. Click **Record** in Ardesia
2. Enter filename when prompted
3. Stream becomes available at: `http://icecast.server.com:8000/presentation.ogg`
4. Share URL in webpage or announcement

For Icecast2 server setup, visit [icecast.org](http://www.icecast.org/)

---

## 🔧 Troubleshooting

### Ardesia won't start - Composite manager error

**Cause:** Transparency requires window compositing

**Solution:** Enable compositor for your desktop environment

**GNOME:**
- Usually enabled by default
- Settings → Appearance → Visual Effects → Normal

**KDE:**
- Usually enabled by default
- System Settings → Display → Compositor → Enable

**XFCE:**
- Settings → Window Manager Tweaks → Compositor → Enable

**Standalone X11:**
```bash
# Install compositor
sudo apt-get install xcompmgr   # or compton/picom

# Run compositor
xcompmgr &
```

**Check if active:**
```bash
xdpyinfo | grep Composite
```

**Graphics drivers:**
- Intel: Free drivers usually work
- NVIDIA: Install proprietary drivers
- AMD/ATI: Install proprietary drivers

### Toolbar is off-screen or too large

Try horizontal layout:
```bash
ardesia -g south    # or -g north
```

Or enable window decorations to move manually:
```bash
ardesia -d
```

### Cannot move toolbar

**By design:** Toolbar is fixed to screen edge (right by default) to avoid hiding important content and stay accessible.

**To move it:** Use `--decorate` option:
```bash
ardesia -d
```

### Recording doesn't work

**Check VLC installation:**
```bash
vlc --version
```

**Test audio drivers:**
- Record audio with another program first
- Ensure microphone/system audio works

**Windows users:**
- Add VLC installation folder to PATH environment variable

**Advanced troubleshooting:**
- Run with verbose: `ardesia --verbose`
- Manually edit screencast script (see paths in [Recording & Streaming](#recording--streaming))

### Wayland compatibility

Ardesia works on Wayland via **XWayland** compatibility layer with some limitations:
- May experience input lag
- Some features might not work optimally
- Native Wayland support is in development

**For best experience on Wayland systems:**
- Use X11 session if available (select "GNOME on Xorg" at login)
- Or help us develop native Wayland support! See [CONTRIBUTING.md](CONTRIBUTING.md)

---

## 📚 Documentation

- **[INSTALL](INSTALL)** - Build and installation instructions
- **[CONTRIBUTING.md](CONTRIBUTING.md)** - Developer guide and contribution guidelines
- **[NEWS](NEWS)** - New features and changes
- **[COPYING](COPYING)** - GPL v3 license text

### External Resources

- **Video Tutorial:** [YouTube Demo](http://www.youtube.com/watch?v=iTiV1qz-rEI)
- **VLC Media Player:** [videolan.org/vlc](http://www.videolan.org/vlc)
- **Icecast Streaming:** [icecast.org](http://www.icecast.org/)

---

## 🤝 Contributing

Ardesia is being actively revived and modernized! Contributions are very welcome.

See [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Development environment setup
- Build process and testing
- Code style guidelines
- Pull request process
- Project architecture

**Quick ways to help:**
- 🐛 Report bugs via [GitHub Issues](https://github.com/pilollipietro/ardesia/issues)
- 💡 Suggest features
- 📝 Improve documentation
- 🧪 Write tests
- 💻 Submit code improvements

---

## 📜 License

Copyright © 2024 Pietro Pilolli and contributors

This program is free software: you can redistribute it and/or modify it under the terms of the **GNU General Public License version 3** as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the [COPYING](COPYING) file for details.

---

## 📞 Contact & Support

- **Issues & Bugs:** [GitHub Issues](https://github.com/pilollipietro/ardesia/issues)
- **Email:** pilolli.pietro@gmail.com
- **Repository:** [github.com/pilollipietro/ardesia](https://github.com/pilollipietro/ardesia)

---

**Have fun! 🎨**

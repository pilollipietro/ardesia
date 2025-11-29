# Contributing to Ardesia

First off, thank you for considering contributing to Ardesia! 🎨

Ardesia is a screen annotation and presentation tool that's being revived after ten years. We're rebuilding the community and modernizing the codebase, and your help is invaluable.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Setup](#development-setup)
- [Building from Source](#building-from-source)
- [Running Tests](#running-tests)
- [Coding Guidelines](#coding-guidelines)
- [Submitting Changes](#submitting-changes)
- [Project Status](#project-status)

## Code of Conduct

This project aims to be welcoming and inclusive.
Please be respectful and constructive in all interactions.

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues. When creating a bug report, include:

- **OS and version** (Ubuntu 24.04, Fedora 40, etc.)
- **Desktop environment** (GNOME, KDE/X11, etc.)
- **Steps to reproduce**
- **Expected vs actual behavior**
- **Screenshots if relevant**
- **Logs** (run with `ardesia --verbose`)

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. Provide:

- **Use case**: What problem does it solve?
- **Proposed solution**: How would it work?
- **Alternatives considered**: Other approaches you thought about

### Pull Requests

- Fork the repo and create your branch from `master`
- If you've added code, add tests
- Ensure the test suite passes
- Update documentation as needed
- Follow the coding style of the project

## Development Setup

### Prerequisites

**Required:**
- GTK 3.x development files
- Cairo graphics library
- GLib 2.0
- Glade (for UI editing)
- Python 3.x (for test suite)
- pkg-config
- autotools (autoconf, automake, libtool)

**Optional:**
- VLC (for recording features)
- xdg-utils (for integration)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install \
    build-essential \
    autoconf \
    automake \
    libtool \
    pkg-config \
    libgtk-3-dev \
    libcairo2-dev \
    libglib2.0-dev \
    glade \
    python3 \
    python3-gi \
    python3-gi-cairo \
    xdg-utils \
    vlc
```

**Fedora:**
```bash
sudo dnf install \
    gcc \
    autoconf \
    automake \
    libtool \
    pkgconfig \
    gtk3-devel \
    cairo-devel \
    glib2-devel \
    glade \
    python3 \
    python3-gobject \
    xdg-utils \
    vlc
```

**Arch Linux:**
```bash
sudo pacman -S \
    base-devel \
    autoconf \
    automake \
    libtool \
    pkgconfig \
    gtk3 \
    cairo \
    glib2 \
    glade \
    python \
    python-gobject \
    xdg-utils \
    vlc
```

### Compositor Requirement

Ardesia requires a composite manager for transparency:

**X11:**
- GNOME (mutter) - enable in Settings → Appearance
- KDE (kwin) - usually enabled by default
- XFCE (xfwm4) - enable in Settings → Window Manager Tweaks
- Standalone: `xcompmgr` or `compton`/`picom`

**Wayland:**
- basic features

To check if compositing is active on X11:
```bash
xdpyinfo | grep Composite
```

## Building from Source

### Quick Start

```bash
# Clone the repository
git clone https://github.com/pilollipietro/ardesia.git
cd ardesia

# Generate configure script
./autogen.sh

# Configure (default prefix is /usr/local)
./configure

# Build
make

# Run without installing
./src/ardesia

# Optional: Install system-wide
sudo make install
```

### Configure Options

```bash
# Install to custom location
./configure --prefix=$HOME/.local

# Enable debug symbols
./configure --enable-debug

# See all options
./configure --help
```

### Common Build Issues

**Problem:** `configure: error: GTK+ 3.x not found`
- **Solution:** Install `libgtk-3-dev` (Debian/Ubuntu) or `gtk3-devel` (Fedora)

**Problem:** `No composite manager detected`
- **Solution:** This is a runtime error, not build error. See [Compositor Requirement](#compositor-requirement)

**Problem:** `autogen.sh: command not found`
- **Solution:** Make it executable: `chmod +x autogen.sh`

## Running Tests

### GUI Test Suite

We have a Python-based GUI test suite that simulates user interactions:

```bash
# Run all tests
python3 src/test_gui.py

# Run with verbose output
python3 src/test_gui.py --verbose

# Run specific test
python3 src/test_gui.py TestClassName.test_method_name
```

### Test Coverage

Current test coverage focuses on:
- Application startup/shutdown
- Tool selection and switching
- Basic drawing operations
- Color changes
- Undo/redo functionality

**Help wanted:** We need more test coverage! See [Testing Priorities](#testing-priorities) below.

### Testing Priorities

High priority areas needing tests:
1. Shape recognizer (rectangle, circle, line detection)
2. Export functionality (screenshot, PDF)
3. Text annotation tool
4. Eraser behavior
5. Multi-monitor support
6. Edge cases (no compositor, file permissions, etc.)

## Coding Guidelines

### C Code Style

This project follows the **GIMP core coding style**, not a generic K&R variant.  
The formatting is automatically enforced by the `.clang-format` file in the repository root.

Please make sure to format all C source files using:

```bash
clang-format -i your_file.c
```

See the GIMP core coding style: [https://testing.developer.gimp.org/core/coding_style/](https://testing.developer.gimp.org/core/coding_style/)

```c
if (condition)
  {
    do_something ();
  }
else
  {
	do_something_else ();
  }

/* Space between function name and parentheses */
my_function (arg);

/* Pointer asterisk aligned with the variable name */
char          *string;
GtkWidget     *widget;

/* Align variable declarations vertically for readability */
GtkWidget     *dialog;
GtkBuilder    *builder;
GError        *error = NULL;

/* Function braces on a new line */
void
some_function (void)
{
  do_stuff ();
}
```

> **Note:** Do not manually adjust indentation or spacing.  
> Always run `clang-format` before committing to ensure consistency with GIMP’s official style.


### Python Code Style

For test code, follow PEP 8:

```python
# Use 4 spaces for indentation
# Use snake_case for functions and variables
# Use descriptive names

def test_pen_tool_selection():
    """Test that pen tool can be selected and activated."""
    # Test implementation
    pass
```

### Commit Messages

Write clear, descriptive commit messages:

```
Short summary (50 chars or less)

More detailed explanation if needed. Wrap at 72 characters.
Explain what and why, not how.

- Bullet points are fine
- Use present tense: "Add feature" not "Added feature"

Fixes #123
```

### GTK/Glade Guidelines

- Keep UI definitions in `.glade` files
- Separate business logic from UI callbacks
- Use meaningful widget IDs
- Document custom signals and callbacks

## Submitting Changes

### Pull Request Process

1. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**
   - Write clear, focused commits
   - Add tests if applicable
   - Update documentation

3. **Test thoroughly**
   ```bash
   make clean && make
   python3 src/test_gui.py
   ./src/ardesia  # Manual testing
   ```

4. **Push and create PR**
   ```bash
   git push origin feature/your-feature-name
   ```
   - Go to GitHub and create Pull Request
   - Fill in the PR template
   - Link related issues

5. **Respond to feedback**
   - Address review comments
   - Update your branch as needed
   - Don't force-push after review starts (unless asked)

### PR Checklist

- [ ] Code builds without warnings
- [ ] Tests pass
- [ ] New code has tests (if applicable)
- [ ] Documentation updated (if applicable)
- [ ] Commit messages are clear
- [ ] No unrelated changes included

## Project Status

### Current Phase: Revival & Modernization

Ardesia was inactive for ~15 years and is now being revived. We're focusing on:

**Phase 1: Foundation** (Current)
- ✅ Port to GTK3 (completed)
- ✅ Basic test suite (in progress)
- 🔄 CI/CD setup (planned)
- 🔄 Modern documentation (you're reading it!)

**Phase 2: Distribution**
- Flatpak packaging
- Modern build system (Meson)

**Phase 3: Modernization**
- GSettings migration (from GConf)
- Enhanced features

### Technology Stack

- **Language:** C
- **GUI:** GTK 3.x with Glade
- **Graphics:** Cairo
- **Build:** Autotools
- **Tests:** Python 3 + PyGObject

### Architecture Overview

```
ardesia/
├── src/                       # Main C source tree
│   ├── ardesia.c              # Application entry point; init & main loop
│   ├── bar.c / bar.h          # Toolbar and main bar helpers
│   ├── bar_callbacks.c        # UI callbacks for toolbar and preferences
│   ├── annotation_window.c    # Annotation overlay window + drawing loop
│   ├── annotation_window.h
│   ├── background_config.c    # Read/write user background config (colors/images)
│   ├── background_window.c    # Background selection UI helpers
│   ├── broken.c               # Stroke simplification / shape recognizer
│   ├── broken.h
│   ├── cairo_functions.c      # Reusable Cairo drawing helpers
│   ├── color_selector.c       # Color selection dialog
│   ├── recorder.c             # Recording integration / process control
│   ├── recordingstudio.c      # Recording UI glue
│   ├── iwb_saver.c            # IWB (internal) save logic
│   ├── pdf_saver.c            # PDF export logic
│   ├── utils.c                # Small utilities (file, math, helpers)
│   ├── user_config.c          # Loading and locating user config files
│   ├── preference_dialog.c    # Preferences dialogs
│   ├── font_selector.c        # Font chooser UI + callbacks
│   └── ...                    # many additional modules (loaders, savers)
├── desktop/                   # Runtime desktop resources
│   ├── ui/                    # Glade/UI files
│   ├── icons/                 # Icon assets (toolbar, tools)
│   ├── backgrounds/           # Built-in backgrounds and images
│   └── scripts/               # Helper scripts (screencast, etc.)
└── docs/                      # Design notes and developer docs
```

## Key Components (What They Do and Where)

* **Annotation Layer (Transparent Overlay)**
    * **Files:** `src/annotation_window.c`, `src/annotation_window.h`
    * **Role:** Manages the transparent drawing surface, the Cairo contexts, per-device coordinate lists (`AnnotateDeviceData`), and dispatches input events to tools. This is the runtime canvas.

* **Stroke Simplification & Shape Recognizer**
    * **Files:** `src/broken.c`, `src/broken.h`, `src/spline.c`
    * **Role:** Reduces noisy input to "meaningful points" (`build_meaningful_point_list()`), detects simple geometric shapes (ellipse, rectangle, regular polygon), and optionally "rectifies" / straightens or "roundifies" strokes. The simplified output is what gets exported or rendered as cleaned strokes.

* **Tool System**
    * **Files:** `src/bar_callbacks.c`, `src/bar.c`, plus many small modules for tools (`color_selector.c`, `font_selector.c`, filler/eraser logic).
    * **Role:** The toolbar (`bar`) provides tool selection. Each tool is a small behavior that reacts to press/move/release and updates the `AnnotateDeviceData` coordinate lists and the annotation surface.

* **Background Selection & Persistence**
    * **Files:** `src/background_config.c`, `src/background_window.c`, `src/bar_callbacks.c` (e.g., `create_bar_preference_window()`, `add_background_button()`).
    * **Role:** Presents background choices (colors/images), lets the user preview choices, and persists the selected background in the user config file. Uses `background_config_*` helpers to read/write and to map filename ↔ label.

* **Export System**
    * **Files:** `src/iwb_saver.c`, `src/pdf_saver.c`, `src/saver.c`, `src/iwb_loader.c`
    * **Role:** Writes the current annotation/canvas to disk in different formats. Export code typically receives a Cairo surface or the annotated data and serializes it to the target format.

* **Recording**
    * **Files:** `src/recorder.c`, `src/recordingstudio.c`
    * **Role:** Integrates with an external recorder (VLC or system tools) and provides a small UI for starting/stopping captures.

## Important Data Structures (Where Defined)

* **`AnnotatePoint`**
    * **Definition:** In a header near the annotation code.
    * **Role:** Represents a single sampled point: `x`, `y`, `width`, `pressure`, `timestamp`. Used in `GSList`s for strokes.

* **`AnnotateDeviceData`**
    * **Definition:** Per-input-device structure.
    * **Role:** Contains the coordinate list (`coord_list`), current tool state, and temporary buffers. Managed in a hash table keyed by `GdkDevice`.

* **`AnnotationData`**
    * **Definition:** Global state container.
    * **Role:** Holds global state (windows, current selection, background selection window references). Many modules read/write a global pointer to this.

## Common Flows (Typical Sequences)

* **Drawing and Simplification**
    1.  Input arrives at `annotation_window` -> appended to `AnnotateDeviceData`.
    2.  On stroke end: `broken()` is called, which internally calls `build_meaningful_point_list(list_inp, rectify, pixel_tolerance)`.
    3.  Optionally, `build_rectified_list()` is called if `rectify` is requested.
    4.  The result is used to draw a cleaned path or to detect a shape (rectangle/ellipse/polygon) and to export/save.

* **Background Preview and Confirm Flow**
    1.  User opens "Backgrounds" (via `create_bar_preference_window()`).
    2.  User toggles preview buttons — the UI shows the background immediately (temporary).
    3.  Only when the user clicks "OK" is the selection persisted with `background_config_set_current_background()`.
    4.  If the user cancels or closes without "OK", the original background is restored (see `background_config_restore_last_background()`).

## Where to Look for Specific Logic

* **For `build_meaningful_point_list()` / `build_rectified_list()`:**
    * Start reading in `src/broken.c`.

* **For Background Config Helpers:**
    * Look in `src/background_config.c` for functions like `get_color_keys()`, `add_color()`, `add_image()`, `set_current_background()`, and `restore_last_background()`.

## Getting Help

- **Issues:** Check [existing issues](https://github.com/pilollipietro/ardesia/issues)
- **Discussions:** Use GitHub Discussions for questions
- **Email:** pilolli.pietro@gmail.com

## Recognition

Contributors are recognized in:
- Git commit history
- Release notes
- AUTHORS file (for significant contributions)

Thank you for contributing to Ardesia! 🚀

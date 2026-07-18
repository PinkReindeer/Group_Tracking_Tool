# Group Tracking Tool

A desktop application for group tracking, built with C++20, OpenGL, and Dear ImGui. It features a layer-based architecture with PostgreSQL-backed data persistence and a modern immediate-mode GUI.

## Screenshots

<!-- Add app screenshot later -->

---

## Architecture

The project is split into two CMake targets:

| Target | Type | Description |
|--------|------|-------------|
| **Core** | Static library | Application framework — windowing (GLFW), rendering (OpenGL/GLAD), GUI (Dear ImGui), database access (libpqxx), and password hashing (libbcrypt). |
| **App** | Executable | The end-user application. Pushes one or more `Layer` subclasses onto the Core application loop. |

---

## Prerequisites

### Required Software

| Tool | Minimum Version | Notes |
|------|----------------|-------|
| **CMake** | 3.14+ | Build system generator |
| **C++ Compiler** | C++20 support | MSVC 2022 recommended on Windows |
| **PostgreSQL** | 14+ (tested with 18) | Server **and** development libraries |
| **OpenGL** | 4.1+ | Typically bundled with your GPU driver |
| **Git** | Any recent | For FetchContent dependency downloads |

### Installing PostgreSQL (Windows)

1. Download the installer from [postgresql.org/download/windows](https://www.postgresql.org/download/windows/).
2. Run the installer — make sure to check **"PostgreSQL Server"** and **"Command Line Tools"** components.
3. Note the installation path (default: `C:\Program Files\PostgreSQL\18`).
4. The CMake build needs `PostgreSQL_ROOT` to point to this directory (see [Build Configuration](#build-configuration) below).

### Installing PostgreSQL (Linux)

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install postgresql postgresql-server-dev-all libpq-dev

# Fedora
sudo dnf install postgresql-server postgresql-devel

# Arch
sudo pacman -S postgresql postgresql-libs
```

### Installing PostgreSQL (macOS)

```bash
brew install postgresql@18
```

---

## Build Instructions

### Option 1 — Visual Studio (Recommended on Windows)

1. Open the project folder in Visual Studio 2022 (File → Open → CMake…).
2. Visual Studio will detect `CMakePresets.json` automatically.
3. If your PostgreSQL installation path differs from the default, edit `CMakePresets.json`:
   ```jsonc
        "PostgreSQL_ROOT": "C:/Program Files/PostgreSQL/18" // adjust this
   ```
4. Select the **x64-Debug** configuration and build (`Ctrl+Shift+B`).
5. Set **App** as the startup project, then run (`F5`).

### Option 2 — Command Line (Window)

```bash
# Configure
cmake -B build -DPostgreSQL_ROOT="C:/Program Files/PostgreSQL/18"

# Build
cmake --build build
```

### Option 3 — Command Line (Unix Makefiles / Linux & macOS)

```bash
cmake -B build
cmake --build build
```

> [NOTE]
> On Linux/macOS, `find_package(PostgreSQL)` usually finds PostgreSQL automatically if installed through a package manager. You only need to set `-DPostgreSQL_ROOT=...` if CMake cannot locate it.

---

## Build Configuration

Key CMake variables you can override:

| Variable | Default | Description |
|----------|---------|-------------|
| `PostgreSQL_ROOT` | *(system default)* | Path to your PostgreSQL installation |
| `CMAKE_BUILD_TYPE` | `Debug` | `Debug`, `Release`, `RelWithDebInfo`, or `MinSizeRel` |
| `CMAKE_CXX_STANDARD` | `20` | C++ standard (do not lower) |

---

## Dependencies

All dependencies except PostgreSQL and OpenGL are fetched automatically via CMake `FetchContent`:

| Library | Version | Purpose |
|---------|---------|---------|
| [GLFW](https://github.com/glfw/glfw) | 3.4 | Cross-platform windowing and input |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | OpenGL mathematics |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.92.3 | Immediate-mode GUI |
| [libpqxx](https://github.com/jtv/libpqxx) | 7.9.2 | C++ PostgreSQL client library |
| [libbcrypt](https://github.com/trusch/libbcrypt) | — | Bcrypt password hashing |
| [GLAD](https://github.com/Dav1dde/glad) | — | OpenGL loader (vendored in `Core/Vendor/glad/`) |
| [stb_image](https://github.com/nothings/stb) | — | Image loading (vendored in `Core/Vendor/stb/`) |
| [IconFontCppHeaders](https://github.com/juliettef/IconFontCppHeaders) | latest | FontAwesome 6 icon constants |
| **PostgreSQL** | 14+ | **Must be installed manually** — see [Prerequisites](#prerequisites) |
| **OpenGL** | 4.1+ | Provided by GPU driver |

---

## Environment Setup

The app loads its database connection string from a `.env` file at runtime (searched upward from the working directory).

1. Copy the example env file and rename it to `.env`:
   ```bash
   # Windows (PowerShell)
   Copy-Item .env.example .env

   # Linux / macOS
   cp .env.example .env
   ```
2. Edit `.env` and set the `DATABASE_URI` key (this value is also documented as a placeholder in `.env.example`):
   ```env
   DATABASE_URI=<database-uri>
   ```
3. **Ask the project owner for the project URI** (the real PostgreSQL / Supabase connection string). Do not invent or commit a real URI. Replace `<database-uri>` in your local `.env` only.

> [IMPORTANT]
> Never commit `.env` with real credentials. Keep secrets in your local `.env` only; `.env.example` should stay as a placeholder template.

---

## Running

After a successful build, run the executable:

```bash
# Windows (from repo root)
.\build\App\Debug\App.exe

# Linux / macOS
./build/App/App
```

> [IMPORTANT]
> The working directory must be set so that `Assets/` (fonts, icons) is reachable. When running from Visual Studio, this is configured automatically via `VS_DEBUGGER_WORKING_DIRECTORY`. When running from the command line, either `cd` into the `App/` directory first or ensure the `Assets/` folder is next to the executable (the CMake post-build step copies it automatically).

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `Could NOT find PostgreSQL` | Set `-DPostgreSQL_ROOT` to your PostgreSQL install path |
| `libpq.dll not found` at runtime (Windows) | Add `C:\Program Files\PostgreSQL\18\bin` to your `PATH` |
| `.env file not found` / `DATABASE_URI key not found` | Copy `.env.example` to `.env`, set `DATABASE_URI`, and **ask the project owner for the project URI** |
| Font warnings on startup | Ensure the working directory contains `Assets/Fonts/` |
| OpenGL errors | Update your GPU drivers; the app requires OpenGL 4.1+ |
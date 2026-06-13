# Contributing

Pull requests are welcome. This is a native Windows C++ app, built either from the
Visual Studio solution or with CMake.

## Prerequisites

- Visual Studio 2022 (17.x) or 2026 (18.x) with the **Desktop development with C++**
  workload.
- For the CMake path: CMake 3.20+ and Ninja (both ship with the VS workload).

Every dependency is vendored under `third_party/`, so there is no vcpkg, Conan, or git
submodule to set up.

## Build

### Visual Studio

Open `steam-tracer-cleaner.sln`, set `Release | x64`, and build (F7). The binary lands
at `build/Release/steam-tracer-cleaner.exe`.

### CMake / Ninja (headless or CI)

```powershell
cmake --preset release
cmake --build --preset release
```

The binary lands at `build/release/bin/steam-tracer-cleaner.exe`. The CMake build also
compiles `tests/`. Run the unit tests with:

```powershell
ctest --preset debug
```

Notes: the project is C++23 (`/std:c++latest`), links the static CRT so the binary runs
without the VC++ redistributable, and forbids in-source builds (configure into a
separate `build/` directory).

## Style and quality

- Formatting is governed by `.clang-format` and `.editorconfig` at the repo root.
- `.clang-tidy` enables `bugprone-*`, `cert-*`, `cppcoreguidelines-*`, and `modernize-*`.
- CI (`.github/workflows/ci.yml`) builds the solution in both Debug and Release x64 on
  every push and pull request.
- Keep comments minimal and factual; match the surrounding code.

## Adding files

The repo carries two build systems that must stay in sync: the Visual Studio solution
(`.vcxproj` + `.vcxproj.filters`, used by VS and CI) and CMake (the `CMakeLists.txt` in
each subdirectory). When you add, remove, or rename a source file, update **both** the
project files and the relevant `CMakeLists.txt`.

## Layout

```
app/          WinMain, D3D11 device, message loop
core/         cleaning logic, profiles, ignore/preserve list, VDF helpers, configs,
              update check
platform/     Win32 wrappers (registry, filesystem, process)
ui/           ImGui screens (cleaner, configs, audit, backups, settings) and widgets
tests/        doctest unit tests (CMake build only)
third_party/  vendored deps: imgui, spdlog, nlohmann_json, sqlite, doctest
assets/       app icon, compiled into the .exe via app.rc
cmake/        shared compile options
```

The `core/` and `platform/` split keeps the cleaning rules unit-testable without a
Win32 message pump.

## Vendored libraries

| Library | License | Used for |
|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) (docking) | MIT | UI |
| [spdlog](https://github.com/gabime/spdlog) | MIT | Logging (header-only) |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT | Config files |
| [SQLite amalgamation](https://www.sqlite.org/) | Public domain | Browser cookie databases |
| [doctest](https://github.com/doctest/doctest) | MIT | Unit tests |

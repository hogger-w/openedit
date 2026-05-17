# Repository Guidelines

## Project Structure & Module Organization

This repository is a Visual Studio C++ Win32 editor application. The solution and project files are `openedit.sln` and `openedit.vcxproj`. Application source lives in `src/`: `openedit.cpp` contains the main window/editor logic, and `FindReplaceWindow.cpp` owns the find/replace dialog. Shared headers and resource IDs are also under `src/`.

Resources are defined in `openedit.rc`; the current app icon is `assets/oe.ico`. Vendored editor libraries live under `lib/scintilla` and `lib/lexilla`; treat them as third-party code. Build outputs go to `x64/`, intermediates to `openedit/x64/`, and release packages to `dist/`.

## Build, Test, and Development Commands

Use Visual Studio 2022 Developer PowerShell, or call MSBuild directly:

```powershell
msbuild openedit.sln /p:Configuration=Debug /p:Platform=x64 /m
```

Builds the app and Scintilla/Lexilla for local debugging.

```powershell
msbuild openedit.sln /p:Configuration=Release /p:Platform=x64 /m
```

Builds the optimized release executable at `x64/Release/openedit.exe`.

```powershell
.\x64\Debug\openedit.exe
```

Runs the debug build. If linking fails with `LNK1168`, close any running `openedit.exe`.

## Coding Style & Naming Conventions

Use C++17 and existing Win32 patterns. Keep indentation and brace style consistent with nearby code. Prefer `constexpr` for constants, `kName` for constants, `PascalCase` for functions/types, and descriptive local variable names. Keep resource IDs synchronized between `src/Resource.h` and `openedit.rc`.

## Testing Guidelines

There is no dedicated app test suite. Before handing off changes, build `Debug|x64`; for release work also build `Release|x64`. Manually smoke test launch, file/folder tree behavior, tab closing, menus, settings, theme switching, find/replace, and file open/save.

## Commit & Pull Request Guidelines

History currently only establishes an initial commit. Use short imperative subjects, for example `Fix folder tree double-click` or `Add release packaging`. PRs should describe user-visible changes, list build/manual tests performed, and note any resource or project-file updates.

## Agent-Specific Instructions

Do not edit generated outputs in `x64/`, `openedit/x64/`, or packaged artifacts in `dist/` unless explicitly requested. Avoid changing vendored `lib/` code. `openedit.rc` uses legacy encoding; if editing it, preserve its encoding.

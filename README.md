# OpenEdit

[中文](README.zh-CN.md) | English

OpenEdit is a lightweight Windows text editor built with Win32, Scintilla, and Lexilla. It focuses on fast startup, a clean desktop UI, and practical daily editing for code and plain text.

<p align="center">
  <img src="assets/oe.png" alt="OpenEdit icon" width="96" />
</p>

## Features

- Multi-tab editing with quick new, close, and previous-session restore.
- Folder tree sidebar with refresh, copy name, and open-in-Explorer actions.
- Scintilla-based editor with syntax highlighting, word wrap, whitespace display, and end-of-line display.
- Language modes for C/C++, C#, Java, JavaScript, TypeScript, Python, HTML, XML, CSS, JSON, SQL, Bash, PowerShell, Rust, Lua, Ruby, and Markdown.
- Find/replace window with normal and regex search, match case, reverse search, wrap search, count, replace, and replace all.
- Light/dark themes and Chinese/English UI.
- Status bar for document length, line count, caret position, encoding, and EOL format.

## Requirements

- Windows 10 or later.
- Visual Studio 2022 with the "Desktop development with C++" workload.
- Windows 10/11 SDK.
- Microsoft Visual C++ Redistributable 2015-2022 may be required on machines without the VC runtime.

## Build

Run from Visual Studio 2022 Developer PowerShell:

```powershell
msbuild openedit.sln /p:Configuration=Debug /p:Platform=x64 /m
```

Run the debug build:

```powershell
.\x64\Debug\openedit.exe
```

Build the release executable:

```powershell
msbuild openedit.sln /p:Configuration=Release /p:Platform=x64 /m
```

Release output:

```text
x64\Release\openedit.exe
```

If linking fails with `LNK1168`, close any running `openedit.exe` and rebuild.

## Release Packaging

OpenEdit currently ships as a portable executable. A typical release package contains:

- `openedit.exe`
- `LICENSE`

Example:

```powershell
New-Item -ItemType Directory -Force dist\openedit-release-x64
Copy-Item x64\Release\openedit.exe dist\openedit-release-x64\
Copy-Item LICENSE dist\openedit-release-x64\
Compress-Archive dist\openedit-release-x64 dist\openedit-release-x64.zip -Force
```

## Project Layout

```text
src/                  Application source and headers
assets/               Application icon assets
lib/scintilla/        Scintilla editor component
lib/lexilla/          Lexilla lexer component
openedit.rc           Win32 resource script
openedit.sln          Visual Studio solution
openedit.vcxproj      Visual Studio C++ project
dist/                 Local release package output
```

## Development Notes

- `openedit.rc` uses legacy resource-file encoding; preserve it when editing.
- `lib/` contains third-party code. Avoid changing it unless upgrading dependencies.
- UI changes should be checked in both light and dark themes.

## License

OpenEdit is licensed under the [Apache License 2.0](LICENSE).

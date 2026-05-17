# OpenEdit

中文 | [English](README.md)

OpenEdit 是一个基于 Win32、Scintilla 和 Lexilla 的轻量级 Windows 文本编辑器。项目目标是提供一个启动快、界面简洁、适合日常代码和文本编辑的桌面应用。

<p align="center">
  <img src="assets/oe.png" alt="OpenEdit 图标" width="96" />
</p>

## 功能特性

- 多标签页编辑，支持快速新建、关闭和恢复上次会话。
- 左侧文件夹树，支持刷新、复制名称和在文件管理器中打开。
- 基于 Scintilla 的编辑器，支持语法高亮、自动换行、空白符显示和行尾符显示。
- 支持 C/C++、C#、Java、JavaScript、TypeScript、Python、HTML、XML、CSS、JSON、SQL、Bash、PowerShell、Rust、Lua、Ruby、Markdown 等语言模式。
- 查找/替换窗口支持普通文本和正则表达式、匹配大小写、反向查找、循环查找、计数、替换和全部替换。
- 支持浅色/暗黑主题和中文/英文界面。
- 状态栏显示长度、行数、光标位置、编码和换行格式。

## 系统要求

- Windows 10 或更高版本。
- Visual Studio 2022，安装“使用 C++ 的桌面开发”工作负载。
- Windows 10/11 SDK。
- 在未安装 VC 运行库的机器上，可能需要 Microsoft Visual C++ Redistributable 2015-2022。

## 构建

在 Visual Studio 2022 Developer PowerShell 中执行：

```powershell
msbuild openedit.sln /p:Configuration=Debug /p:Platform=x64 /m
```

运行调试版本：

```powershell
.\x64\Debug\openedit.exe
```

构建发布版本：

```powershell
msbuild openedit.sln /p:Configuration=Release /p:Platform=x64 /m
```

发布版本输出：

```text
x64\Release\openedit.exe
```

如果构建时出现 `LNK1168`，通常是 `openedit.exe` 正在运行，关闭程序后重新构建即可。

## 打包 Release

OpenEdit 当前以便携式可执行文件发布。发布包通常包含：

- `openedit.exe`
- `LICENSE`

示例：

```powershell
New-Item -ItemType Directory -Force dist\openedit-release-x64
Copy-Item x64\Release\openedit.exe dist\openedit-release-x64\
Copy-Item LICENSE dist\openedit-release-x64\
Compress-Archive dist\openedit-release-x64 dist\openedit-release-x64.zip -Force
```

## 项目结构

```text
src/                  应用源码和头文件
assets/               应用图标资源
lib/scintilla/        Scintilla 编辑器组件
lib/lexilla/          Lexilla 语法解析组件
openedit.rc           Win32 资源脚本
openedit.sln          Visual Studio 解决方案
openedit.vcxproj      Visual Studio C++ 项目
dist/                 本地发布包输出目录
```

## 开发说明

- `openedit.rc` 使用传统资源文件编码，编辑时注意保留编码。
- `lib/` 下是第三方代码，除非升级依赖，否则尽量不要修改。
- UI 行为修改建议同时验证浅色/暗黑主题。

## 许可证

本项目使用 [Apache License 2.0](LICENSE)。

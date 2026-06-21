#include "openedit_internal.h"

void ShowErrorMessage(const wchar_t* message)
{
    MessageBoxW(hWnd, message, L"openedit", MB_OK | MB_ICONERROR);
}

std::wstring JoinPath(const std::wstring& directory, const std::wstring& fileName)
{
    if (directory.empty())
        return fileName;

    const wchar_t last = directory.back();
    if (last == L'\\' || last == L'/')
        return directory + fileName;
    return directory + L"\\" + fileName;
}

std::wstring GetEnvironmentDirectory(const wchar_t* variableName)
{
    DWORD required = GetEnvironmentVariableW(variableName, nullptr, 0);
    if (required == 0)
        return L"";

    std::wstring value(required, L'\0');
    DWORD written = GetEnvironmentVariableW(variableName, value.data(), required);
    if (written == 0 || written >= required)
        return L"";

    value.resize(written);
    return value;
}

std::wstring GetAppStateDirectory()
{
    std::wstring base = GetEnvironmentDirectory(L"LOCALAPPDATA");
    if (base.empty())
        base = GetEnvironmentDirectory(L"TEMP");
    if (base.empty())
        return L".";

    const std::wstring directory = JoinPath(base, L"openedit");
    CreateDirectoryW(directory.c_str(), nullptr);
    return directory;
}

std::wstring GetSessionFilePath()
{
    return JoinPath(GetAppStateDirectory(), L"session.txt");
}

std::wstring GetSettingsFilePath()
{
    return JoinPath(GetAppStateDirectory(), L"settings.txt");
}

std::wstring GetSessionTempDirectory()
{
    wchar_t tempPath[MAX_PATH]{};
    DWORD length = GetTempPathW(MAX_PATH, tempPath);
    std::wstring base = (length > 0 && length < MAX_PATH) ? std::wstring(tempPath, length) : GetEnvironmentDirectory(L"TEMP");
    if (base.empty())
        base = L".";

    const std::wstring directory = JoinPath(base, L"openedit");
    CreateDirectoryW(directory.c_str(), nullptr);
    return directory;
}

std::wstring CreateSessionTempFilePath()
{
    GUID guid{};
    wchar_t guidText[40]{};
    if (SUCCEEDED(CoCreateGuid(&guid)) && StringFromGUID2(guid, guidText, static_cast<int>(sizeof(guidText) / sizeof(guidText[0]))) > 0)
        return JoinPath(GetSessionTempDirectory(), std::wstring(L"untitled-") + guidText + L".tmp");

    return JoinPath(GetSessionTempDirectory(), L"untitled-" + std::to_wstring(GetTickCount64()) + L".tmp");
}

bool WriteBytesToFilePath(const std::wstring& path, const std::vector<char>& bytes, bool showErrors)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        if (showErrors)
            ShowErrorMessage(L"无法写入文件。");
        return false;
    }

    const char* cursor = bytes.data();
    size_t remaining = bytes.size();
    bool ok = true;
    while (remaining > 0)
    {
        const DWORD chunk = static_cast<DWORD>((std::min)(remaining, static_cast<size_t>((std::numeric_limits<DWORD>::max)())));
        DWORD written = 0;
        if (!WriteFile(file, cursor, chunk, &written, nullptr) || written != chunk)
        {
            ok = false;
            break;
        }
        cursor += written;
        remaining -= written;
    }

    CloseHandle(file);
    if (!ok && showErrors)
        ShowErrorMessage(L"保存文件时发生错误。");
    return ok;
}

bool ReadBytesFromFilePath(const std::wstring& path, std::vector<char>& content, bool showErrors)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        if (showErrors)
            ShowErrorMessage(L"无法打开文件。");
        return false;
    }

    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart > static_cast<LONGLONG>((std::numeric_limits<DWORD>::max)()))
    {
        CloseHandle(file);
        if (showErrors)
            ShowErrorMessage(L"文件太大，无法打开。");
        return false;
    }

    content.assign(static_cast<size_t>(fileSize.QuadPart) + 1, '\0');
    DWORD bytesRead = 0;
    const BOOL readOk = ReadFile(file, content.data(), static_cast<DWORD>(fileSize.QuadPart), &bytesRead, nullptr);
    CloseHandle(file);

    if (!readOk)
    {
        if (showErrors)
            ShowErrorMessage(L"读取文件时发生错误。");
        return false;
    }

    content[bytesRead] = '\0';
    content.resize(static_cast<size_t>(bytesRead) + 1);
    return true;
}

bool DirectoryExists(const std::wstring& path)
{
    if (path.empty())
        return false;

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsFileReadOnly(const std::wstring& path)
{
    if (path.empty())
        return false;

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_READONLY) &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool WriteDocumentTextToFile(const std::wstring& path, const std::string& utf8Text, DocumentEncoding encoding, bool showErrors)
{
    const std::vector<char> encodedText = EncodeUtf8ForFile(utf8Text, encoding);
    return WriteBytesToFilePath(path, encodedText, showErrors);
}

bool ReadDocumentTextFromFile(const std::wstring& path, std::string& utf8Text, DocumentEncoding& encoding, bool showErrors)
{
    std::vector<char> content;
    if (!ReadBytesFromFilePath(path, content, showErrors))
        return false;

    const size_t byteCount = content.empty() ? 0 : content.size() - 1;
    utf8Text = DecodeFileBytesToUtf8(content, byteCount, encoding);
    return true;
}

bool SaveFileFromEditor(const std::wstring& path)
{
    const sptr_t length = Sci(SCI_GETLENGTH);
    if (length < 0 || static_cast<unsigned long long>(length) > (std::numeric_limits<size_t>::max)() - 1)
    {
        ShowErrorMessage(L"文件太大，无法保存。");
        return false;
    }

    std::vector<char> text(static_cast<size_t>(length) + 1);
    Sci(SCI_GETTEXT, static_cast<uptr_t>(text.size()), reinterpret_cast<sptr_t>(text.data()));
    const std::string utf8Text(text.data(), static_cast<size_t>(length));
    const DocumentEncoding encoding = IsActiveTabValid() ? g_tabs[g_activeTabIndex].encoding : DocumentEncoding::Utf8;

    if (!WriteDocumentTextToFile(path, utf8Text, encoding, true))
        return false;

    Sci(SCI_SETSAVEPOINT);
    return true;
}

std::wstring PickSaveFilePath(HWND owner)
{
    wchar_t fileName[MAX_PATH] = L"";
    static const wchar_t filter[] =
        L"Text Files (*.txt)\0*.txt\0"
        L"Code Files\0*.cpp;*.h;*.cs;*.java;*.js;*.ts;*.py;*.html;*.css;*.json;*.sql;*.md;*.yaml;*.yml;*.toml;*.ini;*.properties;*.mk;*.diff;*.patch;*.bat;*.cmd;*.zig;*.nim;*.reg;*.iss\0"
        L"All Files (*.*)\0*.*\0\0";

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"txt";
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    return GetSaveFileNameW(&ofn) ? std::wstring(fileName) : std::wstring();
}

bool SaveCurrentFileAs()
{
    std::wstring path = PickSaveFilePath(hWnd);
    if (path.empty())
        return false;

    if (IsFileReadOnly(path))
    {
        MessageBoxW(
            hWnd,
            UiText(L"\u76EE\u6807\u6587\u4EF6\u662F\u53EA\u8BFB\u7684\uFF0C\u65E0\u6CD5\u8986\u76D6\u3002\u8BF7\u9009\u62E9\u5176\u4ED6\u4FDD\u5B58\u4F4D\u7F6E\uFF0C\u6216\u5148\u53D6\u6D88\u8BE5\u6587\u4EF6\u7684\u53EA\u8BFB\u5C5E\u6027\u3002",
                L"The target file is read-only and cannot be overwritten. Choose another save location or clear the file's read-only attribute first."),
            L"openedit",
            MB_OK | MB_ICONWARNING);
        return false;
    }

    if (!SaveFileFromEditor(path))
        return false;

    g_currentFilePath = path;
    ApplyLanguage(DetectLanguageFromPath(path));
    if (IsActiveTabValid())
    {
        DocumentTab& tab = g_tabs[g_activeTabIndex];
        tab.path = path;
        tab.title = FileNameFromPath(path);
        tab.text = GetEditorText();
        tab.savedText = tab.text;
        tab.languageCommand = g_currentLanguageCommand;
        tab.eolMode = static_cast<int>(Sci(SCI_GETEOLMODE));
        tab.modified = false;
        tab.untitled = false;
        tab.readOnly = IsFileReadOnly(path);
        Sci(SCI_SETREADONLY, tab.readOnly ? TRUE : FALSE, 0);
        InvalidateTabBar();
        RefreshOpenFilesTree();
        InvalidateStatusBar();
        if (!tab.openedFromFolder)
            SetFolderPaneVisible(true);
    }
    UpdateWindowTitle();
    return true;
}

bool PromptSaveReadOnlyFileAs(const std::wstring& path)
{
    std::wstring message = UiText(
        L"\u5F53\u524D\u6587\u4EF6\u662F\u53EA\u8BFB\u7684\uFF0C\u65E0\u6CD5\u76F4\u63A5\u4FDD\u5B58\u3002\u662F\u5426\u53E6\u5B58\u4E3A\u5176\u4ED6\u6587\u4EF6\uFF1F",
        L"The current file is read-only and cannot be saved directly. Save it as another file?");
    if (!path.empty())
        message = FileNameFromPath(path) + L"\n\n" + message;

    const int result = MessageBoxW(hWnd, message.c_str(), L"openedit", MB_YESNO | MB_ICONWARNING);
    return result == IDYES ? SaveCurrentFileAs() : false;
}

bool SaveCurrentFile()
{
    if (g_currentFilePath.empty())
        return SaveCurrentFileAs();

    const bool fileReadOnly = IsFileReadOnly(g_currentFilePath);
    if (IsActiveTabValid() && g_tabs[g_activeTabIndex].readOnly != fileReadOnly)
    {
        g_tabs[g_activeTabIndex].readOnly = fileReadOnly;
        Sci(SCI_SETREADONLY, fileReadOnly ? TRUE : FALSE, 0);
        InvalidateTabBar();
        RefreshOpenFilesTree();
        InvalidateStatusBar();
    }

    if (fileReadOnly)
    {
        return PromptSaveReadOnlyFileAs(g_currentFilePath);
    }

    if (!SaveFileFromEditor(g_currentFilePath))
        return false;

    if (IsActiveTabValid())
    {
        DocumentTab& tab = g_tabs[g_activeTabIndex];
        tab.path = g_currentFilePath;
        tab.title = FileNameFromPath(g_currentFilePath);
        tab.text = GetEditorText();
        tab.savedText = tab.text;
        tab.languageCommand = g_currentLanguageCommand;
        tab.eolMode = static_cast<int>(Sci(SCI_GETEOLMODE));
        tab.modified = false;
        tab.untitled = false;
        tab.readOnly = IsFileReadOnly(g_currentFilePath);
        Sci(SCI_SETREADONLY, tab.readOnly ? TRUE : FALSE, 0);
        InvalidateTabBar();
        RefreshOpenFilesTree();
        InvalidateStatusBar();
    }
    UpdateWindowTitle();
    return true;
}

bool PromptSaveIfModified()
{
    CaptureActiveTab();
    if (!(IsActiveTabValid() ? g_tabs[g_activeTabIndex].modified : Sci(SCI_GETMODIFY)))
        return true;

    std::wstring message = L"当前文件有未保存的修改。是否保存？";
    if (IsActiveTabValid())
        message = GetTabDisplayTitle(g_tabs[g_activeTabIndex]) + L" 有未保存的修改。是否保存？";

    const int result = MessageBoxW(
        hWnd,
        message.c_str(),
        L"openedit",
        MB_YESNOCANCEL | MB_ICONWARNING);

    if (result == IDYES)
        return SaveCurrentFile();
    if (result == IDNO)
        return true;
    return false;
}

void CloseTab(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return;

    if (tabIndex != g_activeTabIndex && g_tabs[tabIndex].modified)
        SwitchToTab(tabIndex);

    if (tabIndex == g_activeTabIndex && !PromptSaveIfModified())
        return;

    const int closingIndex = tabIndex;
    const bool closingActiveTab = closingIndex == g_activeTabIndex;
    g_tabs.erase(g_tabs.begin() + closingIndex);

    if (g_tabs.empty())
    {
        g_activeTabIndex = -1;
        AddDocumentTab(CreateDefaultUntitledTab());
        if (g_currentFolderPath.empty() && !HasOpenFilesTreeTabs())
            SetFolderPaneVisible(false);
        return;
    }

    if (closingActiveTab)
    {
        const int nextIndex = (std::min)(closingIndex, static_cast<int>(g_tabs.size()) - 1);
        g_activeTabIndex = -1;
        LoadTabIntoEditor(nextIndex);
    }
    else
    {
        if (closingIndex < g_activeTabIndex)
            --g_activeTabIndex;
        InvalidateTabBar();
        RefreshOpenFilesTree();
    }

    if (g_currentFolderPath.empty() && !HasOpenFilesTreeTabs())
        SetFolderPaneVisible(false);
}

bool PromptSaveAllTabs()
{
    CaptureActiveTab();
    for (int index = 0; index < static_cast<int>(g_tabs.size()); ++index)
    {
        if (g_tabs[index].untitled)
            continue;

        if (!g_tabs[index].modified)
            continue;

        SwitchToTab(index);
        if (!PromptSaveIfModified())
            return false;
    }
    return true;
}

void NewFile()
{
    AddDocumentTab(CreateUntitledTab());
}

bool LoadDocumentTabFromFile(const std::wstring& path, DocumentTab& tab, bool showErrors)
{
    std::string decodedText;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    if (!ReadDocumentTextFromFile(path, decodedText, encoding, showErrors))
        return false;

    tab.title = FileNameFromPath(path);
    tab.path = path;
    tab.sessionTempPath.clear();
    tab.text = decodedText;
    tab.savedText = tab.text;
    tab.languageCommand = DetectLanguageFromPath(path);
    tab.encoding = encoding;
    tab.eolMode = DetectEolModeFromText(tab.text);
    tab.modified = false;
    tab.untitled = false;
    tab.readOnly = IsFileReadOnly(path);
    return true;
}

bool LoadFileIntoEditor(const std::wstring& path, bool openedFromFolder)
{
    const int existingTab = FindOpenTabByPath(path);
    if (existingTab >= 0)
    {
        const bool readOnly = IsFileReadOnly(path);
        if (g_tabs[existingTab].readOnly != readOnly)
        {
            g_tabs[existingTab].readOnly = readOnly;
            if (existingTab == g_activeTabIndex)
                Sci(SCI_SETREADONLY, readOnly ? TRUE : FALSE, 0);
            InvalidateTabBar();
            RefreshOpenFilesTree();
            InvalidateStatusBar();
        }
        if (!openedFromFolder && g_tabs[existingTab].openedFromFolder)
        {
            g_tabs[existingTab].openedFromFolder = false;
            RefreshOpenFilesTree();
        }
        SwitchToTab(existingTab);
        if (HasFileTabs())
            RemoveEmptyUntitledTabs();
        if (!openedFromFolder)
            SetFolderPaneVisible(true);
        return true;
    }

    DocumentTab tab;
    if (!LoadDocumentTabFromFile(path, tab, true))
        return false;
    tab.openedFromFolder = openedFromFolder;

    AddDocumentTabReplacingDefaultBlank(std::move(tab));
    if (HasFileTabs())
        RemoveEmptyUntitledTabs();
    if (!openedFromFolder)
        SetFolderPaneVisible(true);
    return true;
}

bool HandleDroppedFiles(HDROP drop)
{
    if (!drop)
        return false;

    const UINT fileCount = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    bool openedAny = false;
    for (UINT index = 0; index < fileCount; ++index)
    {
        const UINT length = DragQueryFileW(drop, index, nullptr, 0);
        if (length == 0)
            continue;

        std::wstring path(length + 1, L'\0');
        const UINT copied = DragQueryFileW(drop, index, path.data(), length + 1);
        if (copied == 0)
            continue;

        path.resize(copied);
        const DWORD attributes = GetFileAttributesW(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        if (LoadFileIntoEditor(path))
            openedAny = true;
    }

    DragFinish(drop);
    if (openedAny)
        g_restoreFolderInSession = false;
    return openedAny;
}

std::wstring PickOpenFilePath(HWND owner)
{
    wchar_t fileName[MAX_PATH] = L"";
    static const wchar_t filter[] =
        L"All Supported Files\0*.txt;*.cpp;*.h;*.cs;*.java;*.js;*.ts;*.py;*.html;*.css;*.json;*.sql;*.md;*.xml;*.sh;*.ps1;*.rs;*.lua;*.rb;*.yaml;*.yml;*.toml;*.ini;*.properties;*.mk;*.diff;*.patch;*.bat;*.cmd;*.zig;*.nim;*.reg;*.iss\0"
        L"All Files (*.*)\0*.*\0\0";

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    return GetOpenFileNameW(&ofn) ? std::wstring(fileName) : std::wstring();
}

void OpenFileCommand()
{
    const std::wstring path = PickOpenFilePath(hWnd);
    if (path.empty())
        return;

    if (LoadFileIntoEditor(path))
        g_restoreFolderInSession = false;
}

int ParseSessionInt(const std::string& text, int fallback)
{
    if (text.empty())
        return fallback;

    int sign = 1;
    size_t index = 0;
    if (text[0] == '-')
    {
        sign = -1;
        index = 1;
    }

    int value = 0;
    for (; index < text.size(); ++index)
    {
        if (text[index] < '0' || text[index] > '9')
            return fallback;
        value = (value * 10) + (text[index] - '0');
    }
    return value * sign;
}

bool TryParseSettingsInt(const std::string& text, int& value)
{
    constexpr int kInvalidSettingsInt = (std::numeric_limits<int>::min)();
    value = ParseSessionInt(text, kInvalidSettingsInt);
    return value != kInvalidSettingsInt;
}

bool IsRestoredMainWindowRectSizeValid(const RECT& rect)
{
    const long long width = static_cast<long long>(rect.right) - rect.left;
    const long long height = static_cast<long long>(rect.bottom) - rect.top;
    return width >= kMinRestoredMainWindowWidth &&
        height >= kMinRestoredMainWindowHeight &&
        width <= (std::numeric_limits<int>::max)() &&
        height <= (std::numeric_limits<int>::max)();
}

bool ParseMainWindowPlacement(const std::string& text, MainWindowPlacement& placement)
{
    std::array<int, 5> values{};
    size_t start = 0;
    for (size_t index = 0; index < values.size(); ++index)
    {
        const bool lastField = index + 1 == values.size();
        const size_t end = lastField ? text.size() : text.find(',', start);
        if (end == std::string::npos || start > text.size())
            return false;

        if (!TryParseSettingsInt(text.substr(start, end - start), values[index]))
            return false;

        start = end + 1;
    }

    const long long right = static_cast<long long>(values[0]) + values[2];
    const long long bottom = static_cast<long long>(values[1]) + values[3];
    if (right > (std::numeric_limits<int>::max)() ||
        right < (std::numeric_limits<int>::min)() ||
        bottom > (std::numeric_limits<int>::max)() ||
        bottom < (std::numeric_limits<int>::min)())
    {
        return false;
    }

    RECT normalRect{
        values[0],
        values[1],
        static_cast<int>(right),
        static_cast<int>(bottom)
    };
    if (!IsRestoredMainWindowRectSizeValid(normalRect))
        return false;

    placement.hasPlacement = true;
    placement.normalRect = normalRect;
    placement.maximized = values[4] != 0;
    return true;
}

void CaptureMainWindowPlacement()
{
    if (!hWnd || !IsWindow(hWnd))
        return;

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    if (!GetWindowPlacement(hWnd, &placement))
        return;

    if (!IsRestoredMainWindowRectSizeValid(placement.rcNormalPosition))
        return;

    g_mainWindowPlacement.hasPlacement = true;
    g_mainWindowPlacement.normalRect = placement.rcNormalPosition;
    g_mainWindowPlacement.maximized = placement.showCmd == SW_SHOWMAXIMIZED ||
        (placement.flags & WPF_RESTORETOMAXIMIZED) != 0;
}

bool AdjustMainWindowPlacementToMonitor(RECT& rect)
{
    if (!IsRestoredMainWindowRectSizeValid(rect))
        return false;

    HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    if (!monitor)
        return false;

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo))
        return false;

    const RECT workArea = monitorInfo.rcWork;
    const int workWidth = workArea.right - workArea.left;
    const int workHeight = workArea.bottom - workArea.top;
    if (workWidth <= 0 || workHeight <= 0)
        return false;

    int width = static_cast<int>(static_cast<long long>(rect.right) - rect.left);
    int height = static_cast<int>(static_cast<long long>(rect.bottom) - rect.top);
    width = (std::min)(width, workWidth);
    height = (std::min)(height, workHeight);

    if (rect.left < workArea.left)
        rect.left = workArea.left;
    if (rect.top < workArea.top)
        rect.top = workArea.top;
    if (rect.left + width > workArea.right)
        rect.left = workArea.right - width;
    if (rect.top + height > workArea.bottom)
        rect.top = workArea.bottom - height;

    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    return true;
}

bool ApplyMainWindowPlacement(HWND window, int nCmdShow)
{
    if (!window || !g_mainWindowPlacement.hasPlacement)
        return false;

    RECT normalRect = g_mainWindowPlacement.normalRect;
    if (!AdjustMainWindowPlacementToMonitor(normalRect))
        return false;

    WINDOWPLACEMENT placement{};
    placement.length = sizeof(placement);
    placement.rcNormalPosition = normalRect;
    placement.showCmd = g_mainWindowPlacement.maximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
    if (nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE || nCmdShow == SW_SHOWMAXIMIZED)
        placement.showCmd = nCmdShow;

    return SetWindowPlacement(window, &placement) != FALSE;
}

char HexDigit(unsigned char value)
{
    return value < 10 ? static_cast<char>('0' + value) : static_cast<char>('A' + (value - 10));
}

int HexValue(char ch)
{
    if (ch >= '0' && ch <= '9')
        return ch - '0';
    if (ch >= 'A' && ch <= 'F')
        return 10 + (ch - 'A');
    if (ch >= 'a' && ch <= 'f')
        return 10 + (ch - 'a');
    return -1;
}

std::string EncodeSessionField(const std::string& text)
{
    std::string encoded;
    encoded.reserve(text.size());
    for (unsigned char ch : text)
    {
        if (ch == '%' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            encoded.push_back('%');
            encoded.push_back(HexDigit((ch >> 4) & 0x0f));
            encoded.push_back(HexDigit(ch & 0x0f));
        }
        else
        {
            encoded.push_back(static_cast<char>(ch));
        }
    }
    return encoded;
}

std::string DecodeSessionField(const std::string& text)
{
    std::string decoded;
    decoded.reserve(text.size());
    for (size_t index = 0; index < text.size(); ++index)
    {
        if (text[index] == '%' && index + 2 < text.size())
        {
            const int high = HexValue(text[index + 1]);
            const int low = HexValue(text[index + 2]);
            if (high >= 0 && low >= 0)
            {
                decoded.push_back(static_cast<char>((high << 4) | low));
                index += 2;
                continue;
            }
        }
        decoded.push_back(text[index]);
    }
    return decoded;
}

std::string EncodeSessionWideField(const std::wstring& text)
{
    return EncodeSessionField(WideToUtf8(text));
}

std::wstring DecodeSessionWideField(const std::string& text)
{
    return Utf8ToWide(DecodeSessionField(text));
}

std::vector<std::string> SplitSessionLine(const std::string& line)
{
    std::vector<std::string> fields;
    size_t start = 0;
    while (start <= line.size())
    {
        const size_t tab = line.find('\t', start);
        if (tab == std::string::npos)
        {
            fields.push_back(line.substr(start));
            break;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return fields;
}

DocumentEncoding SessionEncodingFromInt(int value)
{
    switch (value)
    {
    case 1:
        return DocumentEncoding::Utf8Bom;
    case 2:
        return DocumentEncoding::Ansi;
    case 3:
        return DocumentEncoding::Utf16LE;
    case 4:
        return DocumentEncoding::Utf16BE;
    case 0:
    default:
        return DocumentEncoding::Utf8;
    }
}

int SessionEncodingToInt(DocumentEncoding encoding)
{
    switch (encoding)
    {
    case DocumentEncoding::Utf8Bom:
        return 1;
    case DocumentEncoding::Ansi:
        return 2;
    case DocumentEncoding::Utf16LE:
        return 3;
    case DocumentEncoding::Utf16BE:
        return 4;
    case DocumentEncoding::Utf8:
    default:
        return 0;
    }
}

void LoadAppSettings()
{
    std::vector<char> content;
    if (!ReadBytesFromFilePath(GetSettingsFilePath(), content, false))
        return;

    const std::string settings(content.data(), content.empty() ? 0 : content.size() - 1);
    size_t start = 0;
    while (start < settings.size())
    {
        size_t end = settings.find('\n', start);
        if (end == std::string::npos)
            end = settings.size();

        std::string line = settings.substr(start, end - start);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.rfind("restorePreviousFiles=", 0) == 0)
            g_restorePreviousFilesOnStartup = ParseSessionInt(line.substr(21), 1) != 0;
        else if (line.rfind("windowPlacement=", 0) == 0)
            ParseMainWindowPlacement(line.substr(16), g_mainWindowPlacement);
        else if (line.rfind("shortcut.", 0) == 0)
        {
            const size_t equals = line.find('=');
            const size_t comma = line.find(',', equals == std::string::npos ? 0 : equals + 1);
            if (equals != std::string::npos && comma != std::string::npos)
            {
                const int commandId = ParseSessionInt(line.substr(9, equals - 9), 0);
                ShortcutBinding* shortcut = FindShortcutBinding(commandId);
                if (shortcut)
                {
                    shortcut->modifiers = static_cast<BYTE>(ParseSessionInt(line.substr(equals + 1, comma - equals - 1), shortcut->defaultModifiers));
                    shortcut->key = static_cast<WORD>(ParseSessionInt(line.substr(comma + 1), shortcut->defaultKey));
                }
            }
        }

        start = end + 1;
    }
}

void SaveAppSettings()
{
    CaptureMainWindowPlacement();

    std::string settings = std::string("restorePreviousFiles=") +
        (g_restorePreviousFilesOnStartup ? "1\n" : "0\n");
    if (g_mainWindowPlacement.hasPlacement)
    {
        const RECT& normalRect = g_mainWindowPlacement.normalRect;
        settings += "windowPlacement=";
        settings += std::to_string(normalRect.left);
        settings += ",";
        settings += std::to_string(normalRect.top);
        settings += ",";
        settings += std::to_string(normalRect.right - normalRect.left);
        settings += ",";
        settings += std::to_string(normalRect.bottom - normalRect.top);
        settings += ",";
        settings += (g_mainWindowPlacement.maximized ? "1\n" : "0\n");
    }
    for (const ShortcutBinding& shortcut : g_shortcutBindings)
    {
        settings += "shortcut.";
        settings += std::to_string(shortcut.commandId);
        settings += "=";
        settings += std::to_string(shortcut.modifiers);
        settings += ",";
        settings += std::to_string(shortcut.key);
        settings += "\n";
    }
    std::vector<char> bytes(settings.begin(), settings.end());
    WriteBytesToFilePath(GetSettingsFilePath(), bytes, false);
}

bool SaveSessionState()
{
    CaptureActiveTab();

    std::string session;
    session += "version=1\n";
    session += "active=" + std::to_string(g_activeTabIndex) + "\n";
    session += "nextUntitled=" + std::to_string(g_nextUntitledIndex) + "\n";
    if (g_restoreFolderInSession && DirectoryExists(g_currentFolderPath))
        session += "folder=" + EncodeSessionWideField(g_currentFolderPath) + "\n";

    for (DocumentTab& tab : g_tabs)
    {
        if (tab.untitled)
        {
            if (tab.sessionTempPath.empty())
                tab.sessionTempPath = CreateSessionTempFilePath();

            if (!WriteDocumentTextToFile(tab.sessionTempPath, tab.text, DocumentEncoding::Utf8, false))
                continue;

            session += "tab\tT\t";
            session += EncodeSessionWideField(GetTabDisplayTitle(tab)) + "\t";
            session += EncodeSessionWideField(tab.sessionTempPath) + "\t";
            session += std::to_string(tab.languageCommand) + "\t";
            session += std::to_string(SessionEncodingToInt(DocumentEncoding::Utf8)) + "\t";
            session += std::to_string(tab.eolMode) + "\t";
            session += tab.openedFromFolder ? "1\n" : "0\n";
        }
        else if (!tab.path.empty())
        {
            session += "tab\tF\t";
            session += EncodeSessionWideField(GetTabDisplayTitle(tab)) + "\t";
            session += EncodeSessionWideField(tab.path) + "\t";
            session += std::to_string(tab.languageCommand) + "\t";
            session += std::to_string(SessionEncodingToInt(tab.encoding)) + "\t";
            session += std::to_string(tab.eolMode) + "\t";
            session += tab.openedFromFolder ? "1\n" : "0\n";
        }
    }

    std::vector<char> bytes(session.begin(), session.end());
    return WriteBytesToFilePath(GetSessionFilePath(), bytes, false);
}

bool LoadStartupTabs()
{
    if (!g_restorePreviousFilesOnStartup)
        return false;

    std::vector<char> content;
    if (!ReadBytesFromFilePath(GetSessionFilePath(), content, false))
        return false;

    const std::string session(content.data(), content.empty() ? 0 : content.size() - 1);
    std::vector<DocumentTab> restoredTabs;
    int activeIndex = 0;
    int nextUntitledIndex = g_nextUntitledIndex;
    std::wstring restoredFolderPath;

    size_t start = 0;
    while (start < session.size())
    {
        size_t end = session.find('\n', start);
        if (end == std::string::npos)
            end = session.size();

        std::string line = session.substr(start, end - start);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line.rfind("active=", 0) == 0)
        {
            activeIndex = ParseSessionInt(line.substr(7), 0);
        }
        else if (line.rfind("nextUntitled=", 0) == 0)
        {
            nextUntitledIndex = ParseSessionInt(line.substr(13), nextUntitledIndex);
        }
        else if (line.rfind("folder=", 0) == 0)
        {
            restoredFolderPath = DecodeSessionWideField(line.substr(7));
        }
        else if (line.rfind("tab\t", 0) == 0)
        {
            const std::vector<std::string> fields = SplitSessionLine(line);
            if (fields.size() >= 7)
            {
                const bool temporaryTab = fields[1] == "T";
                const std::wstring title = DecodeSessionWideField(fields[2]);
                const std::wstring path = DecodeSessionWideField(fields[3]);
                const int languageCommand = ParseSessionInt(fields[4], IDM_LANG_TEXT);
                const DocumentEncoding encoding = SessionEncodingFromInt(ParseSessionInt(fields[5], 0));
                const int eolMode = ParseSessionInt(fields[6], SC_EOL_CRLF);
                const bool openedFromFolder = fields.size() >= 8 && ParseSessionInt(fields[7], 0) != 0;

                DocumentTab tab;
                if (temporaryTab)
                {
                    std::string text;
                    DocumentEncoding ignoredEncoding = DocumentEncoding::Utf8;
                    if (!ReadDocumentTextFromFile(path, text, ignoredEncoding, false))
                    {
                        start = end + 1;
                        continue;
                    }

                    tab.title = title.empty() ? (L"Untitled " + std::to_wstring(nextUntitledIndex++)) : title;
                    tab.path.clear();
                    tab.sessionTempPath = path;
                    tab.text = text;
                    tab.savedText = tab.text;
                    tab.languageCommand = languageCommand;
                    tab.encoding = DocumentEncoding::Utf8;
                    tab.eolMode = eolMode;
                    tab.modified = false;
                    tab.untitled = true;
                    tab.openedFromFolder = false;
                }
                else
                {
                    if (!LoadDocumentTabFromFile(path, tab, false))
                    {
                        start = end + 1;
                        continue;
                    }
                    tab.languageCommand = languageCommand;
                    tab.encoding = encoding;
                    tab.eolMode = eolMode;
                    tab.modified = false;
                    tab.untitled = false;
                    tab.openedFromFolder = openedFromFolder;
                }

                restoredTabs.push_back(std::move(tab));
            }
        }

        start = end + 1;
    }

    if (restoredTabs.empty())
        return false;

    g_tabs = std::move(restoredTabs);
    g_activeTabIndex = -1;
    g_nextUntitledIndex = (std::max)(g_nextUntitledIndex, nextUntitledIndex);
    activeIndex = (std::min)((std::max)(activeIndex, 0), static_cast<int>(g_tabs.size()) - 1);
    LoadTabIntoEditor(activeIndex);
    if (DirectoryExists(restoredFolderPath))
    {
        PopulateFolderTree(restoredFolderPath);
        g_restoreFolderInSession = true;
    }
    else
    {
        g_currentFolderPath.clear();
        g_folderItems.clear();
        g_restoreFolderInSession = false;
        SetFolderPaneVisible(HasOpenFilesTreeTabs());
    }
    return true;
}

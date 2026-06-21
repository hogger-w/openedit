#include "openedit_internal.h"

DocumentEncoding EncodingFromCommand(int commandId)
{
    switch (commandId)
    {
    case IDM_ENCODING_UTF8_BOM:
        return DocumentEncoding::Utf8Bom;
    case IDM_ENCODING_ANSI:
        return DocumentEncoding::Ansi;
    case IDM_ENCODING_UTF16_LE:
        return DocumentEncoding::Utf16LE;
    case IDM_ENCODING_UTF16_BE:
        return DocumentEncoding::Utf16BE;
    case IDM_ENCODING_UTF8:
    default:
        return DocumentEncoding::Utf8;
    }
}

int CommandFromEncoding(DocumentEncoding encoding)
{
    switch (encoding)
    {
    case DocumentEncoding::Utf8Bom:
        return IDM_ENCODING_UTF8_BOM;
    case DocumentEncoding::Ansi:
        return IDM_ENCODING_ANSI;
    case DocumentEncoding::Utf16LE:
        return IDM_ENCODING_UTF16_LE;
    case DocumentEncoding::Utf16BE:
        return IDM_ENCODING_UTF16_BE;
    case DocumentEncoding::Utf8:
    default:
        return IDM_ENCODING_UTF8;
    }
}

int EolModeFromCommand(int commandId)
{
    switch (commandId)
    {
    case IDM_EOL_UNIX:
        return SC_EOL_LF;
    case IDM_EOL_MAC:
        return SC_EOL_CR;
    case IDM_EOL_WINDOWS:
    default:
        return SC_EOL_CRLF;
    }
}

int CommandFromEolMode(int eolMode)
{
    switch (eolMode)
    {
    case SC_EOL_LF:
        return IDM_EOL_UNIX;
    case SC_EOL_CR:
        return IDM_EOL_MAC;
    case SC_EOL_CRLF:
    default:
        return IDM_EOL_WINDOWS;
    }
}

void SetActiveDocumentEncoding(DocumentEncoding encoding)
{
    if (!IsActiveTabValid())
        return;

    g_tabs[g_activeTabIndex].encoding = encoding;
    InvalidateStatusBar();
}

void SetActiveDocumentEolMode(int eolMode)
{
    if (!g_hSci)
        return;

    const int oldEolMode = static_cast<int>(Sci(SCI_GETEOLMODE));
    if (oldEolMode != eolMode)
    {
        if (IsActiveDocumentReadOnly())
        {
            ShowReadOnlyWarning(hWnd);
            return;
        }

        Sci(SCI_CONVERTEOLS, eolMode, 0);
        Sci(SCI_SETEOLMODE, eolMode, 0);
    }

    if (IsActiveTabValid())
    {
        DocumentTab& tab = g_tabs[g_activeTabIndex];
        tab.eolMode = eolMode;
        tab.text = GetEditorText();
        tab.modified = tab.text != tab.savedText;
        InvalidateTabBar();
        RefreshOpenFilesTree();
    }
    InvalidateStatusBar();
}

void SetControlFont(HWND control)
{
    if (control)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

HBRUSH GetPopupBackBrush()
{
    if (!g_hPopupBackBrush)
        g_hPopupBackBrush = CreateSolidBrush(ThemePopupBack());
    return g_hPopupBackBrush;
}

HBRUSH GetPopupSurfaceBrush()
{
    if (!g_hPopupSurfaceBrush)
        g_hPopupSurfaceBrush = CreateSolidBrush(ThemePopupSurface());
    return g_hPopupSurfaceBrush;
}

HBRUSH GetPopupInputBrush()
{
    if (!g_hPopupInputBrush)
        g_hPopupInputBrush = CreateSolidBrush(ThemePopupInputBack());
    return g_hPopupInputBrush;
}

HBRUSH GetMenuBackBrush()
{
    if (!g_hMenuBackBrush)
        g_hMenuBackBrush = CreateSolidBrush(ThemeMenuBack());
    return g_hMenuBackBrush;
}

void InvalidateFolderTree()
{
    if (g_hFolderTree)
        RedrawWindow(g_hFolderTree, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
}

void InvalidateOpenFilesTree()
{
    if (g_hOpenFilesTree)
        RedrawWindow(g_hOpenFilesTree, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
}

void DrawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius)
{
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawOwnerButton(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
        return;

    const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
    const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;
    RECT rect = drawItem->rcItem;
    FillRect(drawItem->hDC, &rect, GetPopupBackBrush());
    DrawRoundedPanel(drawItem->hDC, rect, ThemePopupButtonBack(pressed), focused ? ThemeAccent() : ThemePopupBorder(), 8);

    wchar_t text[128]{};
    GetWindowTextW(drawItem->hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, ThemePopupText());
    DrawTextW(drawItem->hDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}

bool IsSettingsOptionControl(int controlId)
{
    switch (controlId)
    {
    case IDC_SETTINGS_THEME_LIGHT:
    case IDC_SETTINGS_THEME_DARK:
    case IDC_SETTINGS_LANGUAGE_CHINESE:
    case IDC_SETTINGS_LANGUAGE_ENGLISH:
    case IDC_SETTINGS_RESTORE_PREVIOUS_FILES:
        return true;
    default:
        return false;
    }
}

bool IsColumnEditorOptionControl(int controlId)
{
    return controlId == IDC_COLUMN_MODE_TEXT || controlId == IDC_COLUMN_MODE_NUMBER;
}

bool IsSettingsNavigationControl(int controlId)
{
    return controlId == IDC_SETTINGS_TAB_GENERAL || controlId == IDC_SETTINGS_TAB_SHORTCUTS;
}

bool IsSettingsShortcutHotKeyControl(int controlId)
{
    return controlId >= IDC_SETTINGS_SHORTCUT_HOTKEY_BASE &&
        controlId < IDC_SETTINGS_SHORTCUT_HOTKEY_BASE + static_cast<int>(kShortcutBindingCount);
}

int SettingsShortcutIndexFromControlId(int controlId)
{
    if (!IsSettingsShortcutHotKeyControl(controlId))
        return -1;
    return controlId - IDC_SETTINGS_SHORTCUT_HOTKEY_BASE;
}

bool IsShortcutModifierKey(WORD key)
{
    switch (key)
    {
    case VK_SHIFT:
    case VK_CONTROL:
    case VK_MENU:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_LMENU:
    case VK_RMENU:
        return true;
    default:
        return false;
    }
}

BYTE ShortcutModifiersFromKeyboardState()
{
    BYTE modifiers = 0;
    if (GetKeyState(VK_CONTROL) & 0x8000)
        modifiers |= kShortcutCtrl;
    if (GetKeyState(VK_SHIFT) & 0x8000)
        modifiers |= kShortcutShift;
    if (GetKeyState(VK_MENU) & 0x8000)
        modifiers |= kShortcutAlt;
    return modifiers;
}

bool IsSettingsRadioControl(int controlId)
{
    switch (controlId)
    {
    case IDC_SETTINGS_THEME_LIGHT:
    case IDC_SETTINGS_THEME_DARK:
    case IDC_SETTINGS_LANGUAGE_CHINESE:
    case IDC_SETTINGS_LANGUAGE_ENGLISH:
        return true;
    default:
        return false;
    }
}

bool IsColumnEditorRadioControl(int controlId)
{
    return IsColumnEditorOptionControl(controlId);
}

bool IsSettingsOptionChecked(int controlId)
{
    switch (controlId)
    {
    case IDC_SETTINGS_THEME_LIGHT:
        return g_settingsDraftTheme == AppTheme::Light;
    case IDC_SETTINGS_THEME_DARK:
        return g_settingsDraftTheme == AppTheme::Dark;
    case IDC_SETTINGS_LANGUAGE_CHINESE:
        return g_settingsDraftLanguage == AppLanguage::Chinese;
    case IDC_SETTINGS_LANGUAGE_ENGLISH:
        return g_settingsDraftLanguage == AppLanguage::English;
    case IDC_SETTINGS_RESTORE_PREVIOUS_FILES:
        return g_settingsDraftRestorePreviousFiles;
    default:
        return false;
    }
}

bool IsColumnEditorOptionChecked(int controlId)
{
    switch (controlId)
    {
    case IDC_COLUMN_MODE_TEXT:
        return g_columnEditorMode == ColumnEditorMode::Text;
    case IDC_COLUMN_MODE_NUMBER:
        return g_columnEditorMode == ColumnEditorMode::Number;
    default:
        return false;
    }
}

void RedrawSettingsOptions(HWND settingsWindow)
{
    RedrawWindow(settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
}

void DrawPopupOptionControl(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
        return;

    const int controlId = GetDlgCtrlID(drawItem->hwndItem);
    const bool radio = IsSettingsRadioControl(controlId) || IsColumnEditorRadioControl(controlId);
    const bool checked = IsSettingsOptionControl(controlId) ?
        IsSettingsOptionChecked(controlId) : IsColumnEditorOptionChecked(controlId);
    RECT rect = drawItem->rcItem;

    FillRect(drawItem->hDC, &rect, GetPopupSurfaceBrush());

    const int glyphSize = 14;
    RECT glyph{
        rect.left + 1,
        rect.top + ((rect.bottom - rect.top) - glyphSize) / 2,
        rect.left + 1 + glyphSize,
        rect.top + ((rect.bottom - rect.top) - glyphSize) / 2 + glyphSize
    };

    const COLORREF border = checked ? ThemeAccent() : ThemePopupBorder();
    const COLORREF fill = checked ? ThemeAccent() : ThemePopupSurface();
    HBRUSH glyphBrush = CreateSolidBrush(fill);
    HPEN glyphPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldBrush = SelectObject(drawItem->hDC, glyphBrush);
    HGDIOBJ oldPen = SelectObject(drawItem->hDC, glyphPen);
    if (radio)
        Ellipse(drawItem->hDC, glyph.left, glyph.top, glyph.right, glyph.bottom);
    else
        RoundRect(drawItem->hDC, glyph.left, glyph.top, glyph.right, glyph.bottom, 4, 4);
    SelectObject(drawItem->hDC, oldPen);
    SelectObject(drawItem->hDC, oldBrush);
    DeleteObject(glyphPen);
    DeleteObject(glyphBrush);

    if (checked)
    {
        if (radio)
        {
            RECT dot{ glyph.left + 4, glyph.top + 4, glyph.right - 4, glyph.bottom - 4 };
            HBRUSH dotBrush = CreateSolidBrush(RGB(255, 255, 255));
            HPEN dotPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            oldBrush = SelectObject(drawItem->hDC, dotBrush);
            oldPen = SelectObject(drawItem->hDC, dotPen);
            Ellipse(drawItem->hDC, dot.left, dot.top, dot.right, dot.bottom);
            SelectObject(drawItem->hDC, oldPen);
            SelectObject(drawItem->hDC, oldBrush);
            DeleteObject(dotPen);
            DeleteObject(dotBrush);
        }
        else
        {
            HPEN checkPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            oldPen = SelectObject(drawItem->hDC, checkPen);
            MoveToEx(drawItem->hDC, glyph.left + 3, glyph.top + 7, nullptr);
            LineTo(drawItem->hDC, glyph.left + 6, glyph.top + 10);
            LineTo(drawItem->hDC, glyph.right - 3, glyph.top + 4);
            SelectObject(drawItem->hDC, oldPen);
            DeleteObject(checkPen);
        }
    }

    wchar_t text[128]{};
    GetWindowTextW(drawItem->hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    RECT textRect{ glyph.right + 7, rect.top, rect.right, rect.bottom };
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, ThemePopupText());
    DrawTextW(drawItem->hDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    if (drawItem->itemState & ODS_FOCUS)
    {
        RECT focusRect = rect;
        focusRect.right -= 2;
        DrawFocusRect(drawItem->hDC, &focusRect);
    }
}

void DrawSettingsNavigationItem(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem)
        return;

    const int controlId = GetDlgCtrlID(drawItem->hwndItem);
    const bool active = (controlId == IDC_SETTINGS_TAB_GENERAL && g_settingsActiveTab == 0) ||
        (controlId == IDC_SETTINGS_TAB_SHORTCUTS && g_settingsActiveTab == 1);
    const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;
    const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
    RECT rect = drawItem->rcItem;

    FillRect(drawItem->hDC, &rect, GetPopupSurfaceBrush());
    const COLORREF fill = active ? ThemePopupButtonBack(pressed) : ThemePopupSurface();
    HBRUSH fillBrush = CreateSolidBrush(fill);
    FillRect(drawItem->hDC, &rect, fillBrush);
    DeleteObject(fillBrush);

    if (active)
    {
        RECT accentRect{ rect.left, rect.top + 5, rect.left + 3, rect.bottom - 5 };
        HBRUSH accentBrush = CreateSolidBrush(ThemeAccent());
        FillRect(drawItem->hDC, &accentRect, accentBrush);
        DeleteObject(accentBrush);
    }

    wchar_t text[128]{};
    GetWindowTextW(drawItem->hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
    RECT textRect = rect;
    textRect.left += 12;
    textRect.right -= 8;
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, active ? ThemeAccent() : ThemePopupText());
    DrawTextW(drawItem->hDC, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    if (focused)
    {
        RECT focusRect = rect;
        InflateRect(&focusRect, -2, -2);
        DrawFocusRect(drawItem->hDC, &focusRect);
    }
}

HWND CreateSettingsControl(HWND parent, const wchar_t* className, const wchar_t* text,
    DWORD style, int x, int y, int width, int height, int id)
{
    HWND control = CreateWindowExW(0, className, text, WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
    SetControlFont(control);
    if (className && lstrcmpW(className, HOTKEY_CLASSW) == 0)
        ApplyControlTheme(control);
    return control;
}

void SetSettingsControlVisible(HWND settingsWindow, int controlId, bool visible)
{
    HWND control = GetDlgItem(settingsWindow, controlId);
    if (control)
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
}

void SetHotKeyControlShortcut(HWND control, const ShortcutBinding& shortcut)
{
    if (!control)
        return;

    const std::wstring text = ShortcutText(shortcut.modifiers, shortcut.key);
    SetWindowTextW(control, text.c_str());
    SendMessageW(control, EM_SETSEL, 0, -1);
}

LRESULT CALLBACK SettingsShortcutEditProc(HWND editControl, UINT message, WPARAM wParam, LPARAM lParam,
    UINT_PTR subclassId, DWORD_PTR refData)
{
    UNREFERENCED_PARAMETER(subclassId);
    const size_t index = static_cast<size_t>(refData);

    switch (message)
    {
    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_SETFOCUS:
    {
        LRESULT result = DefSubclassProc(editControl, message, wParam, lParam);
        SendMessageW(editControl, EM_SETSEL, 0, -1);
        return result;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const WORD key = static_cast<WORD>(wParam);
        if (index < g_settingsDraftShortcuts.size() && !IsShortcutModifierKey(key))
        {
            g_settingsDraftShortcuts[index].key = key;
            g_settingsDraftShortcuts[index].modifiers = ShortcutModifiersFromKeyboardState();
            SetHotKeyControlShortcut(editControl, g_settingsDraftShortcuts[index]);
        }
        return 0;
    }

    case WM_CHAR:
    case WM_SYSCHAR:
        return 0;

    case WM_NCDESTROY:
        RemoveWindowSubclass(editControl, SettingsShortcutEditProc, subclassId);
        break;
    }

    return DefSubclassProc(editControl, message, wParam, lParam);
}

void ResetSettingsShortcutControls(HWND settingsWindow)
{
    g_settingsDraftShortcuts = g_shortcutBindings;
    for (ShortcutBinding& shortcut : g_settingsDraftShortcuts)
    {
        shortcut.key = shortcut.defaultKey;
        shortcut.modifiers = shortcut.defaultModifiers;
    }

    for (size_t index = 0; index < g_settingsDraftShortcuts.size(); ++index)
        SetHotKeyControlShortcut(GetDlgItem(settingsWindow, IDC_SETTINGS_SHORTCUT_HOTKEY_BASE + static_cast<int>(index)),
            g_settingsDraftShortcuts[index]);
}

bool ShortcutNeedsModifier(WORD key)
{
    return (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9');
}

bool ReadSettingsShortcutControls(HWND settingsWindow, std::array<ShortcutBinding, kShortcutBindingCount>& shortcuts)
{
    shortcuts = g_settingsDraftShortcuts;
    for (size_t index = 0; index < shortcuts.size(); ++index)
    {
        if (shortcuts[index].key == 0)
        {
            MessageBoxW(settingsWindow,
                UiText(L"\u5FEB\u6377\u952E\u4E0D\u80FD\u4E3A\u7A7A\u3002", L"Shortcuts cannot be empty."),
                L"openedit", MB_OK | MB_ICONWARNING);
            return false;
        }

        if (ShortcutNeedsModifier(shortcuts[index].key) && shortcuts[index].modifiers == 0)
        {
            MessageBoxW(settingsWindow,
                UiText(L"\u5B57\u6BCD\u548C\u6570\u5B57\u5FEB\u6377\u952E\u9700\u8981 Ctrl\u3001Shift \u6216 Alt \u7EC4\u5408\u3002",
                    L"Letter and number shortcuts need Ctrl, Shift, or Alt."),
                L"openedit", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    for (size_t left = 0; left < shortcuts.size(); ++left)
    {
        for (size_t right = left + 1; right < shortcuts.size(); ++right)
        {
            if (shortcuts[left].key == shortcuts[right].key &&
                shortcuts[left].modifiers == shortcuts[right].modifiers)
            {
                MessageBoxW(settingsWindow,
                    UiText(L"\u5FEB\u6377\u952E\u4E0D\u80FD\u91CD\u590D\u3002", L"Shortcuts must be unique."),
                    L"openedit", MB_OK | MB_ICONWARNING);
                return false;
            }
        }
    }

    return true;
}

bool ShortcutsEqual(const std::array<ShortcutBinding, kShortcutBindingCount>& left,
    const std::array<ShortcutBinding, kShortcutBindingCount>& right)
{
    for (size_t index = 0; index < left.size(); ++index)
    {
        if (left[index].commandId != right[index].commandId ||
            left[index].key != right[index].key ||
            left[index].modifiers != right[index].modifiers)
            return false;
    }
    return true;
}

void UpdateSettingsTabVisibility(HWND settingsWindow)
{
    const bool generalVisible = g_settingsActiveTab == 0;
    const int generalControls[] = {
        IDC_SETTINGS_GENERAL_THEME_LABEL,
        IDC_SETTINGS_THEME_LIGHT,
        IDC_SETTINGS_THEME_DARK,
        IDC_SETTINGS_GENERAL_LANGUAGE_LABEL,
        IDC_SETTINGS_LANGUAGE_CHINESE,
        IDC_SETTINGS_LANGUAGE_ENGLISH,
        IDC_SETTINGS_GENERAL_STARTUP_LABEL,
        IDC_SETTINGS_RESTORE_PREVIOUS_FILES
    };

    for (int controlId : generalControls)
        SetSettingsControlVisible(settingsWindow, controlId, generalVisible);

    const bool shortcutsVisible = g_settingsActiveTab == 1;
    for (size_t index = 0; index < g_shortcutBindings.size(); ++index)
    {
        SetSettingsControlVisible(settingsWindow, IDC_SETTINGS_SHORTCUT_LABEL_BASE + static_cast<int>(index), shortcutsVisible);
        SetSettingsControlVisible(settingsWindow, IDC_SETTINGS_SHORTCUT_HOTKEY_BASE + static_cast<int>(index), shortcutsVisible);
    }
    SetSettingsControlVisible(settingsWindow, IDC_SETTINGS_SHORTCUT_RESET, shortcutsVisible);
    RedrawSettingsOptions(settingsWindow);
}

void CreateSettingsShortcutControls(HWND settingsWindow)
{
    constexpr int firstY = 76;
    constexpr int rowHeight = 19;
    for (size_t index = 0; index < g_settingsDraftShortcuts.size(); ++index)
    {
        const int y = firstY + static_cast<int>(index) * rowHeight;
        CreateSettingsControl(settingsWindow, L"STATIC",
            ShortcutCommandName(g_settingsDraftShortcuts[index].commandId).c_str(),
            SS_LEFTNOWORDWRAP, 34, y + 2, 158, 18,
            IDC_SETTINGS_SHORTCUT_LABEL_BASE + static_cast<int>(index));
        HWND hotKey = CreateSettingsControl(settingsWindow, L"EDIT", L"",
            WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 204, y, 168, 22,
            IDC_SETTINGS_SHORTCUT_HOTKEY_BASE + static_cast<int>(index));
        if (hotKey)
            SetWindowSubclass(hotKey, SettingsShortcutEditProc, 1, static_cast<DWORD_PTR>(index));
        SetHotKeyControlShortcut(hotKey, g_settingsDraftShortcuts[index]);
    }

    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u6062\u590D\u9ED8\u8BA4", L"Restore Defaults"),
        BS_OWNERDRAW | WS_TABSTOP, 16, 438, 128, 28, IDC_SETTINGS_SHORTCUT_RESET);
}

void InitializeSettingsControls(HWND settingsWindow)
{
    g_settingsDraftTheme = g_appTheme;
    g_settingsDraftLanguage = g_appLanguage;
    g_settingsDraftRestorePreviousFiles = g_restorePreviousFilesOnStartup;
    g_settingsDraftShortcuts = g_shortcutBindings;
    g_settingsActiveTab = 0;

    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u5E38\u89C4", L"General"),
        BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 404, 66, 96, 30, IDC_SETTINGS_TAB_GENERAL);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u5FEB\u6377\u952E", L"Shortcuts"),
        BS_OWNERDRAW | WS_TABSTOP, 404, 98, 96, 30, IDC_SETTINGS_TAB_SHORTCUTS);

    CreateSettingsControl(settingsWindow, L"STATIC", UiText(L"\u4E3B\u9898", L"Theme"),
        SS_LEFTNOWORDWRAP, 30, 66, 280, 20, IDC_SETTINGS_GENERAL_THEME_LABEL);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u6D45\u8272\u6A21\u5F0F", L"Light"),
        BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 42, 96, 120, 22, IDC_SETTINGS_THEME_LIGHT);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u6697\u9ED1\u6A21\u5F0F", L"Dark"),
        BS_OWNERDRAW | WS_TABSTOP, 186, 96, 120, 22, IDC_SETTINGS_THEME_DARK);

    CreateSettingsControl(settingsWindow, L"STATIC", UiText(L"\u754C\u9762\u8BED\u8A00", L"Interface Language"),
        SS_LEFTNOWORDWRAP, 30, 150, 280, 20, IDC_SETTINGS_GENERAL_LANGUAGE_LABEL);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u4E2D\u6587", L"Chinese"),
        BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 42, 180, 120, 22, IDC_SETTINGS_LANGUAGE_CHINESE);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u82F1\u6587", L"English"),
        BS_OWNERDRAW | WS_TABSTOP, 186, 180, 120, 22, IDC_SETTINGS_LANGUAGE_ENGLISH);

    CreateSettingsControl(settingsWindow, L"STATIC", UiText(L"\u542F\u52A8", L"Startup"),
        SS_LEFTNOWORDWRAP, 30, 234, 280, 20, IDC_SETTINGS_GENERAL_STARTUP_LABEL);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u6253\u5F00\u4E0A\u6B21\u7684\u6587\u4EF6", L"Open previous files"),
        BS_OWNERDRAW | WS_TABSTOP, 42, 264, 220, 22, IDC_SETTINGS_RESTORE_PREVIOUS_FILES);

    CreateSettingsShortcutControls(settingsWindow);

    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u786E\u5B9A", L"OK"),
        BS_OWNERDRAW | WS_TABSTOP, 334, 438, 80, 28, IDOK);
    CreateSettingsControl(settingsWindow, L"BUTTON", UiText(L"\u53D6\u6D88", L"Cancel"),
        BS_OWNERDRAW | WS_TABSTOP, 424, 438, 80, 28, IDCANCEL);

    UpdateSettingsTabVisibility(settingsWindow);
}

bool ApplySettingsFromWindow(HWND settingsWindow)
{
    const AppTheme selectedTheme = g_settingsDraftTheme;
    const AppLanguage selectedLanguage = g_settingsDraftLanguage;
    const bool selectedRestorePreviousFiles = g_settingsDraftRestorePreviousFiles;
    std::array<ShortcutBinding, kShortcutBindingCount> selectedShortcuts{};
    if (!ReadSettingsShortcutControls(settingsWindow, selectedShortcuts))
        return false;

    const bool themeChanged = selectedTheme != g_appTheme;
    const bool languageChanged = selectedLanguage != g_appLanguage;
    const bool shortcutsChanged = !ShortcutsEqual(selectedShortcuts, g_shortcutBindings);
    g_appTheme = selectedTheme;
    g_appLanguage = selectedLanguage;
    g_restorePreviousFilesOnStartup = selectedRestorePreviousFiles;
    g_shortcutBindings = selectedShortcuts;
    SaveAppSettings();

    if (themeChanged)
        ApplyAppTheme();
    if (shortcutsChanged)
        RebuildAcceleratorTable();
    if (languageChanged || shortcutsChanged)
        UpdateMainMenuText();
    if (languageChanged)
    {
        if (g_findWindow)
            g_findWindow->UpdateThemeAndLanguage(IsDarkTheme(), g_appLanguage == AppLanguage::Chinese);
        InvalidateStatusBar();
    }
    return true;
}

void ShowSettingsWindow()
{
    if (g_hSettingsWindow)
    {
        ShowWindow(g_hSettingsWindow, SW_SHOW);
        SetForegroundWindow(g_hSettingsWindow);
        return;
    }

    RECT ownerRect{};
    GetWindowRect(hWnd, &ownerRect);
    RECT windowRect{ 0, 0, 520, 484 };
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

    g_hSettingsWindow = CreateWindowExW(exStyle, kSettingsWindowClassName,
        UiText(L"\u8BBE\u7F6E", L"Settings"), style,
        x, y, width, height, hWnd, nullptr, hInst, nullptr);

    if (!g_hSettingsWindow)
        return;

    ApplyWindowChromeTheme(g_hSettingsWindow);
    EnableWindow(hWnd, FALSE);
    ShowWindow(g_hSettingsWindow, SW_SHOW);
    UpdateWindow(g_hSettingsWindow);
}

void InitializeAboutControls(HWND aboutWindow)
{
    HWND icon = CreateSettingsControl(aboutWindow, L"STATIC", L"",
        SS_ICON | SS_CENTERIMAGE, 26, 34, 36, 36, IDC_STATIC);
    if (icon)
        SendMessageW(icon, STM_SETICON,
            reinterpret_cast<WPARAM>(LoadSharedAppIcon(hInst, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON))), 0);

    CreateSettingsControl(aboutWindow, L"STATIC", L"openedit",
        SS_LEFTNOWORDWRAP, 78, 30, 220, 24, IDC_STATIC);
    CreateSettingsControl(aboutWindow, L"STATIC", UiText(L"\u7248\u672C 1.0.12", L"Version 1.0.12"),
        SS_LEFTNOWORDWRAP, 78, 58, 220, 20, IDC_STATIC);
    SYSTEMTIME localTime{};
    GetLocalTime(&localTime);
    const std::wstring copyrightText = g_appLanguage == AppLanguage::Chinese ?
        (std::wstring(L"\u7248\u6743\u6240\u6709 (c) ") + std::to_wstring(localTime.wYear)) :
        (std::wstring(L"Copyright (c) ") + std::to_wstring(localTime.wYear));
    CreateSettingsControl(aboutWindow, L"STATIC", copyrightText.c_str(),
        SS_LEFTNOWORDWRAP, 78, 82, 220, 20, IDC_STATIC);
    CreateSettingsControl(aboutWindow, L"BUTTON", UiText(L"\u786E\u5B9A", L"OK"),
        BS_OWNERDRAW | WS_TABSTOP, 224, 126, 80, 28, IDOK);
}

void ShowAboutWindow()
{
    if (g_hAboutWindow)
    {
        ShowWindow(g_hAboutWindow, SW_SHOW);
        SetForegroundWindow(g_hAboutWindow);
        return;
    }

    RECT ownerRect{};
    GetWindowRect(hWnd, &ownerRect);
    RECT windowRect{ 0, 0, 330, 172 };
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

    g_hAboutWindow = CreateWindowExW(exStyle, kAboutWindowClassName,
        UiText(L"\u5173\u4E8E openedit", L"About openedit"), style,
        x, y, width, height, hWnd, nullptr, hInst, nullptr);

    if (!g_hAboutWindow)
        return;

    ApplyWindowChromeTheme(g_hAboutWindow);
    ApplyWindowAppIcons(g_hAboutWindow);
    EnableWindow(hWnd, FALSE);
    ShowWindow(g_hAboutWindow, SW_SHOW);
    UpdateWindow(g_hAboutWindow);
}

void UpdateColumnEditorModeControls(HWND columnWindow)
{
    const bool textMode = g_columnEditorMode == ColumnEditorMode::Text;
    EnableWindow(GetDlgItem(columnWindow, IDC_COLUMN_TEXT_VALUE), textMode ? TRUE : FALSE);

    const int numberControls[] = {
        IDC_COLUMN_INITIAL_VALUE,
        IDC_COLUMN_INCREMENT_VALUE,
        IDC_COLUMN_REPEAT_VALUE,
        IDC_COLUMN_PADDING_VALUE
    };
    for (int controlId : numberControls)
        EnableWindow(GetDlgItem(columnWindow, controlId), textMode ? FALSE : TRUE);

    RedrawWindow(columnWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
}

HWND CreateColumnEditorControl(HWND parent, const wchar_t* className, const wchar_t* text,
    DWORD style, int x, int y, int width, int height, int id)
{
    HWND control = CreateSettingsControl(parent, className, text, style, x, y, width, height, id);
    if (control && (lstrcmpW(className, L"EDIT") == 0 || lstrcmpW(className, L"COMBOBOX") == 0))
        ApplyControlTheme(control);
    return control;
}

void InitializeColumnEditorControls(HWND columnWindow)
{
    g_columnEditorMode = ColumnEditorMode::Text;

    CreateColumnEditorControl(columnWindow, L"BUTTON", UiText(L"\u63D2\u5165\u6587\u672C", L"Insert Text"),
        BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 30, 22, 120, 22, IDC_COLUMN_MODE_TEXT);
    CreateColumnEditorControl(columnWindow, L"BUTTON", UiText(L"\u63D2\u5165\u6570\u5B57", L"Insert Number"),
        BS_OWNERDRAW | WS_TABSTOP, 164, 22, 136, 22, IDC_COLUMN_MODE_NUMBER);

    CreateColumnEditorControl(columnWindow, L"STATIC", UiText(L"\u6587\u672C", L"Text"),
        SS_LEFTNOWORDWRAP, 30, 58, 72, 20, IDC_STATIC);
    CreateColumnEditorControl(columnWindow, L"EDIT", L"",
        WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 112, 54, 310, 24, IDC_COLUMN_TEXT_VALUE);

    CreateColumnEditorControl(columnWindow, L"STATIC", UiText(L"\u521D\u59CB\u503C", L"Initial"),
        SS_LEFTNOWORDWRAP, 30, 106, 72, 20, IDC_STATIC);
    CreateColumnEditorControl(columnWindow, L"EDIT", L"1",
        WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 112, 102, 120, 24, IDC_COLUMN_INITIAL_VALUE);

    CreateColumnEditorControl(columnWindow, L"STATIC", UiText(L"\u589E\u91CF", L"Increment"),
        SS_LEFTNOWORDWRAP, 256, 106, 72, 20, IDC_STATIC);
    CreateColumnEditorControl(columnWindow, L"EDIT", L"1",
        WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 330, 102, 92, 24, IDC_COLUMN_INCREMENT_VALUE);

    CreateColumnEditorControl(columnWindow, L"STATIC", UiText(L"\u91CD\u590D\u6B21\u6570", L"Repeat"),
        SS_LEFTNOWORDWRAP, 30, 144, 80, 20, IDC_STATIC);
    CreateColumnEditorControl(columnWindow, L"EDIT", L"",
        WS_TABSTOP | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL, 112, 140, 120, 24, IDC_COLUMN_REPEAT_VALUE);

    CreateColumnEditorControl(columnWindow, L"STATIC", UiText(L"\u5F00\u5934\u8865\u9F50", L"Leading"),
        SS_LEFTNOWORDWRAP, 256, 144, 72, 20, IDC_STATIC);
    HWND padding = CreateColumnEditorControl(columnWindow, L"COMBOBOX", L"",
        WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 330, 140, 92, 140, IDC_COLUMN_PADDING_VALUE);
    if (padding)
    {
        SendMessageW(padding, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(UiText(L"\u4E0D\u8865\u9F50", L"None")));
        SendMessageW(padding, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(UiText(L"\u8865\u96F6", L"Zeros")));
        SendMessageW(padding, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(UiText(L"\u8865\u7A7A\u683C", L"Spaces")));
        SendMessageW(padding, CB_SETCURSEL, 0, 0);
    }

    CreateColumnEditorControl(columnWindow, L"BUTTON", UiText(L"\u786E\u5B9A", L"OK"),
        BS_OWNERDRAW | WS_TABSTOP, 270, 226, 72, 28, IDOK);
    CreateColumnEditorControl(columnWindow, L"BUTTON", UiText(L"\u53D6\u6D88", L"Cancel"),
        BS_OWNERDRAW | WS_TABSTOP, 350, 226, 72, 28, IDCANCEL);

    UpdateColumnEditorModeControls(columnWindow);
    SetFocus(GetDlgItem(columnWindow, IDC_COLUMN_TEXT_VALUE));
}

ColumnPaddingMode ReadColumnPaddingMode(HWND columnWindow)
{
    const int selection = static_cast<int>(SendMessageW(GetDlgItem(columnWindow, IDC_COLUMN_PADDING_VALUE), CB_GETCURSEL, 0, 0));
    switch (selection)
    {
    case 1:
        return ColumnPaddingMode::Zero;
    case 2:
        return ColumnPaddingMode::Space;
    default:
        return ColumnPaddingMode::None;
    }
}

bool ReadColumnEditorRequest(HWND columnWindow, ColumnEditRequest& request)
{
    request = ColumnEditRequest{};
    request.mode = g_columnEditorMode;

    if (request.mode == ColumnEditorMode::Text)
    {
        request.text = GetControlText(columnWindow, IDC_COLUMN_TEXT_VALUE);
        if (request.text.empty())
        {
            ShowColumnEditorMessage(columnWindow, L"\u8BF7\u8F93\u5165\u8981\u63D2\u5165\u7684\u6587\u672C\u3002", L"Enter the text to insert.");
            return false;
        }
        if (request.text.find_first_of(L"\r\n") != std::wstring::npos)
        {
            ShowColumnEditorMessage(columnWindow, L"\u63D2\u5165\u6587\u672C\u4E0D\u80FD\u5305\u542B\u6362\u884C\u7B26\u3002",
                L"Inserted text cannot contain line breaks.");
            return false;
        }
        return true;
    }

    const std::wstring initialText = GetControlText(columnWindow, IDC_COLUMN_INITIAL_VALUE);
    const std::wstring incrementText = GetControlText(columnWindow, IDC_COLUMN_INCREMENT_VALUE);
    const std::wstring repeatText = GetControlText(columnWindow, IDC_COLUMN_REPEAT_VALUE);

    if (!TryParseInt64(initialText, request.initialValue))
    {
        ShowColumnEditorMessage(columnWindow, L"\u521D\u59CB\u503C\u5FC5\u987B\u662F\u6709\u6548\u6574\u6570\u3002", L"Initial value must be a valid integer.");
        return false;
    }
    if (!TryParseInt64(incrementText, request.increment))
    {
        ShowColumnEditorMessage(columnWindow, L"\u589E\u91CF\u5FC5\u987B\u662F\u6709\u6548\u6574\u6570\u3002", L"Increment must be a valid integer.");
        return false;
    }

    request.repeatCount = 1;
    if (!TrimWhitespace(repeatText).empty() && !TryParsePositiveInt(repeatText, request.repeatCount))
    {
        ShowColumnEditorMessage(columnWindow, L"\u91CD\u590D\u6B21\u6570\u5FC5\u987B\u662F\u5927\u4E8E 0 \u7684\u6574\u6570\uFF0C\u6216\u7559\u7A7A\u3002",
            L"Repeat count must be an integer greater than 0, or blank.");
        return false;
    }

    request.padding = ReadColumnPaddingMode(columnWindow);
    request.minimumNumberWidth = request.padding == ColumnPaddingMode::None ? 0 : MinimumNumberWidthFromInitialText(initialText);
    return true;
}

void ShowColumnEditorWindow()
{
    if (g_hColumnEditorWindow)
    {
        ShowWindow(g_hColumnEditorWindow, SW_SHOW);
        SetForegroundWindow(g_hColumnEditorWindow);
        return;
    }

    if (Sci(SCI_GETREADONLY))
    {
        ShowColumnEditorMessage(hWnd, L"\u5F53\u524D\u6587\u6863\u662F\u53EA\u8BFB\u7684\uFF0C\u65E0\u6CD5\u6267\u884C\u5217\u7F16\u8F91\u3002",
            L"The current document is read-only, so column editing cannot be applied.");
        return;
    }

    ColumnEditSelection selection{};
    std::wstring error;
    if (!GetActiveColumnSelection(selection, error))
    {
        ShowColumnEditorMessage(hWnd, error);
        return;
    }

    RECT ownerRect{};
    GetWindowRect(hWnd, &ownerRect);
    RECT windowRect{ 0, 0, 460, 276 };
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU;
    const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT;
    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

    g_hColumnEditorWindow = CreateWindowExW(exStyle, kColumnEditorWindowClassName,
        ColumnEditorTitle(), style, x, y, width, height, hWnd, nullptr, hInst, nullptr);

    if (!g_hColumnEditorWindow)
        return;

    ApplyWindowChromeTheme(g_hColumnEditorWindow);
    EnableWindow(hWnd, FALSE);
    ShowWindow(g_hColumnEditorWindow, SW_SHOW);
    UpdateWindow(g_hColumnEditorWindow);
}

LRESULT CALLBACK ColumnEditorWndProc(HWND columnWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        InitializeColumnEditorControls(columnWindow);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(columnWindow, &ps);
        RECT clientRect{};
        GetClientRect(columnWindow, &clientRect);
        FillRect(hdc, &clientRect, GetPopupBackBrush());
        DrawRoundedPanel(hdc, RECT{ 16, 16, 444, 88 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        DrawRoundedPanel(hdc, RECT{ 16, 98, 444, 186 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        EndPaint(columnWindow, &ps);
        return 0;
    }

    case WM_COMMAND:
    {
        const int commandId = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (notification == BN_CLICKED)
        {
            if (commandId == IDC_COLUMN_MODE_TEXT || commandId == IDC_COLUMN_MODE_NUMBER)
            {
                g_columnEditorMode = commandId == IDC_COLUMN_MODE_TEXT ? ColumnEditorMode::Text : ColumnEditorMode::Number;
                UpdateColumnEditorModeControls(columnWindow);
                SetFocus(GetDlgItem(columnWindow,
                    g_columnEditorMode == ColumnEditorMode::Text ? IDC_COLUMN_TEXT_VALUE : IDC_COLUMN_INITIAL_VALUE));
                return 0;
            }
        }

        if (commandId == IDOK)
        {
            ColumnEditRequest request{};
            if (ReadColumnEditorRequest(columnWindow, request) && ApplyColumnEditRequest(columnWindow, request))
                DestroyPopupWindowWithoutFlash(columnWindow);
            return 0;
        }
        if (commandId == IDCANCEL)
        {
            DestroyPopupWindowWithoutFlash(columnWindow);
            return 0;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawItem)
        {
            const int controlId = GetDlgCtrlID(drawItem->hwndItem);
            if (IsColumnEditorOptionControl(controlId))
                DrawPopupOptionControl(drawItem);
            else
                DrawOwnerButton(drawItem);
        }
        return TRUE;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, IsWindowEnabled(reinterpret_cast<HWND>(lParam)) ? ThemePopupText() : ThemeInvisible());
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, ThemePopupInputBack());
        return reinterpret_cast<LRESULT>(GetPopupInputBrush());
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetTextColor(hdc, IsWindowEnabled(control) ? ThemePopupText() : ThemeInvisible());
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, ThemePopupSurface());
        return reinterpret_cast<LRESULT>(GetPopupSurfaceBrush());
    }

    case WM_CLOSE:
        DestroyPopupWindowWithoutFlash(columnWindow);
        return 0;

    case WM_DESTROY:
        if (g_hColumnEditorWindow == columnWindow)
            g_hColumnEditorWindow = nullptr;
        RestoreMainWindowAfterPopupClose(true);
        return 0;
    }

    return DefWindowProc(columnWindow, message, wParam, lParam);
}

LRESULT CALLBACK AboutWndProc(HWND aboutWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        InitializeAboutControls(aboutWindow);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(aboutWindow, &ps);
        RECT clientRect{};
        GetClientRect(aboutWindow, &clientRect);
        FillRect(hdc, &clientRect, GetPopupBackBrush());
        DrawRoundedPanel(hdc, RECT{ 16, 18, 314, 112 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        EndPaint(aboutWindow, &ps);
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            DestroyPopupWindowWithoutFlash(aboutWindow);
            return 0;
        }
        break;

    case WM_DRAWITEM:
        DrawOwnerButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        return TRUE;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, ThemePopupText());
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, ThemePopupSurface());
        return reinterpret_cast<LRESULT>(GetPopupSurfaceBrush());
    }

    case WM_CLOSE:
        DestroyPopupWindowWithoutFlash(aboutWindow);
        return 0;

    case WM_DESTROY:
        if (g_hAboutWindow == aboutWindow)
            g_hAboutWindow = nullptr;
        RestoreMainWindowAfterPopupClose(true);
        return 0;
    }

    return DefWindowProc(aboutWindow, message, wParam, lParam);
}

LRESULT CALLBACK SettingsWndProc(HWND settingsWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        InitializeSettingsControls(settingsWindow);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(settingsWindow, &ps);
        RECT clientRect{};
        GetClientRect(settingsWindow, &clientRect);
        FillRect(hdc, &clientRect, GetPopupBackBrush());
        DrawRoundedPanel(hdc, RECT{ 400, 60, 504, 134 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        if (g_settingsActiveTab == 0)
        {
            DrawRoundedPanel(hdc, RECT{ 16, 60, 388, 134 }, ThemePopupSurface(), ThemePopupBorder(), 10);
            DrawRoundedPanel(hdc, RECT{ 16, 144, 388, 218 }, ThemePopupSurface(), ThemePopupBorder(), 10);
            DrawRoundedPanel(hdc, RECT{ 16, 228, 388, 302 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        }
        else
        {
            DrawRoundedPanel(hdc, RECT{ 16, 60, 388, 430 }, ThemePopupSurface(), ThemePopupBorder(), 10);
        }
        EndPaint(settingsWindow, &ps);
        return 0;
    }

    case WM_COMMAND:
    {
        const int commandId = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (notification == BN_CLICKED)
        {
            if (commandId == IDC_SETTINGS_TAB_GENERAL || commandId == IDC_SETTINGS_TAB_SHORTCUTS)
            {
                g_settingsActiveTab = commandId == IDC_SETTINGS_TAB_GENERAL ? 0 : 1;
                UpdateSettingsTabVisibility(settingsWindow);
                return 0;
            }
            if (commandId == IDC_SETTINGS_THEME_LIGHT || commandId == IDC_SETTINGS_THEME_DARK)
            {
                g_settingsDraftTheme = commandId == IDC_SETTINGS_THEME_LIGHT ? AppTheme::Light : AppTheme::Dark;
                RedrawSettingsOptions(settingsWindow);
                return 0;
            }
            if (commandId == IDC_SETTINGS_LANGUAGE_CHINESE || commandId == IDC_SETTINGS_LANGUAGE_ENGLISH)
            {
                g_settingsDraftLanguage = commandId == IDC_SETTINGS_LANGUAGE_CHINESE ? AppLanguage::Chinese : AppLanguage::English;
                RedrawSettingsOptions(settingsWindow);
                return 0;
            }
            if (commandId == IDC_SETTINGS_RESTORE_PREVIOUS_FILES)
            {
                g_settingsDraftRestorePreviousFiles = !g_settingsDraftRestorePreviousFiles;
                RedrawWindow(GetDlgItem(settingsWindow, commandId), nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
                return 0;
            }
            if (commandId == IDC_SETTINGS_SHORTCUT_RESET)
            {
                ResetSettingsShortcutControls(settingsWindow);
                return 0;
            }
        }

        if (commandId == IDOK)
        {
            if (ApplySettingsFromWindow(settingsWindow))
                DestroyPopupWindowWithoutFlash(settingsWindow);
            return 0;
        }
        if (commandId == IDCANCEL)
        {
            DestroyPopupWindowWithoutFlash(settingsWindow);
            return 0;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawItem)
        {
            const int controlId = GetDlgCtrlID(drawItem->hwndItem);
            if (IsSettingsNavigationControl(controlId))
                DrawSettingsNavigationItem(drawItem);
            else if (IsSettingsOptionControl(controlId))
                DrawPopupOptionControl(drawItem);
            else
                DrawOwnerButton(drawItem);
        }
        return TRUE;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, ThemePopupText());
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, ThemePopupInputBack());
        return reinterpret_cast<LRESULT>(GetPopupInputBrush());
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        const int controlId = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
        if (IsSettingsShortcutHotKeyControl(controlId))
        {
            SetTextColor(hdc, ThemePopupText());
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, ThemePopupInputBack());
            return reinterpret_cast<LRESULT>(GetPopupInputBrush());
        }

        SetTextColor(hdc, ThemePopupText());
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, ThemePopupSurface());
        return reinterpret_cast<LRESULT>(GetPopupSurfaceBrush());
    }

    case WM_CLOSE:
        DestroyPopupWindowWithoutFlash(settingsWindow);
        return 0;

    case WM_DESTROY:
        if (g_hSettingsWindow == settingsWindow)
            g_hSettingsWindow = nullptr;
        RestoreMainWindowAfterPopupClose(true);
        return 0;
    }

    return DefWindowProc(settingsWindow, message, wParam, lParam);
}

LRESULT CALLBACK FolderSplitterPreviewWndProc(HWND previewWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(previewWindow, &ps);

        RECT clientRect{};
        GetClientRect(previewWindow, &clientRect);
        HBRUSH background = CreateSolidBrush(kFolderSplitterPreviewBackColor);
        FillRect(hdc, &clientRect, background);
        DeleteObject(background);

        SetBkMode(hdc, TRANSPARENT);
        HPEN linePen = CreatePen(PS_DASH, 1, kFolderSplitterPreviewLineColor);
        HGDIOBJ oldPen = SelectObject(hdc, linePen);
        const int centerX = (clientRect.right - clientRect.left) / 2;
        MoveToEx(hdc, centerX, clientRect.top, nullptr);
        LineTo(hdc, centerX, clientRect.bottom);
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);

        EndPaint(previewWindow, &ps);
        return 0;
    }
    }

    return DefWindowProc(previewWindow, message, wParam, lParam);
}

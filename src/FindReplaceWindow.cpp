#include "FindReplaceWindow.h"
#include "FindReplaceWindowSupport.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

using namespace FindReplaceSupport;

OpenEditFindWindow::OpenEditFindWindow(HINSTANCE instance) : instance_(instance)
{
}

OpenEditFindWindow::~OpenEditFindWindow()
{
    Destroy();
}

bool OpenEditFindWindow::Show(HWND owner, bool replaceVisible, const std::wstring& findText,
    const std::wstring& replaceText, bool darkTheme, bool chineseLanguage,
    const Callbacks& callbacks)
{
    UNREFERENCED_PARAMETER(replaceVisible);

    owner_ = owner;
    callbacks_ = callbacks;
    darkTheme_ = darkTheme;
    chineseLanguage_ = chineseLanguage;

    if (!EnsureWindow(owner))
        return false;

    SetWindowTextW(GetDlgItem(window_, IDC_FIND_TEXT), findText.c_str());
    SetWindowTextW(GetDlgItem(window_, IDC_REPLACE_TEXT), replaceText.c_str());
    UpdateThemeAndLanguage(darkTheme_, chineseLanguage_);
    const bool wasVisible = IsVisible();
    if (!wasVisible)
        PositionWindow();
    else
        LayoutControls();
    ShowWindow(window_, SW_SHOWNOACTIVATE);
    SetForegroundWindow(window_);
    active_ = true;
    ApplyOpacity(active_);
    FocusFindText();
    SendMessageW(GetDlgItem(window_, IDC_FIND_TEXT), EM_SETSEL, 0, -1);
    return true;
}

void OpenEditFindWindow::Hide()
{
    if (!window_)
        return;

    SaveWindowPosition();
    if (owner_ && IsWindow(owner_))
        SetActiveWindow(owner_);

    SetWindowPos(window_, nullptr, 0, 0, 0, 0,
        SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
        SWP_NOOWNERZORDER | SWP_NOACTIVATE);

    if (owner_ && IsWindow(owner_))
    {
        SetActiveWindow(owner_);
    }
}

void OpenEditFindWindow::Destroy()
{
    if (window_)
    {
        DestroyWindow(window_);
        window_ = nullptr;
    }

    DeleteObject(backgroundBrush_);
    DeleteObject(surfaceBrush_);
    DeleteObject(editBrush_);
    backgroundBrush_ = nullptr;
    surfaceBrush_ = nullptr;
    editBrush_ = nullptr;
}

bool OpenEditFindWindow::TranslateDialogMessage(MSG& message)
{
    if (!window_ || !IsWindowVisible(window_))
        return false;
    if (message.hwnd != window_ && !IsChild(window_, message.hwnd))
        return false;

    if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)
    {
        Hide();
        return true;
    }
    if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
        (GetKeyState(VK_CONTROL) >= 0))
    {
        ExecuteFind((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        return true;
    }
    return IsDialogMessageW(window_, &message) != FALSE;
}

void OpenEditFindWindow::UpdateThemeAndLanguage(bool darkTheme, bool chineseLanguage)
{
    darkTheme_ = darkTheme;
    chineseLanguage_ = chineseLanguage;
    UpdateBrushes();
    UpdateTexts();
    ApplyWindowDarkMode(window_, darkTheme_);
    if (window_)
        RedrawThemedControls();
}

bool OpenEditFindWindow::IsVisible() const
{
    return window_ && IsWindowVisible(window_);
}

std::wstring OpenEditFindWindow::FindText() const
{
    return window_ ? GetControlText(window_, IDC_FIND_TEXT) : L"";
}

std::wstring OpenEditFindWindow::ReplaceText() const
{
    return window_ ? GetControlText(window_, IDC_REPLACE_TEXT) : L"";
}

bool OpenEditFindWindow::EnsureWindow(HWND owner)
{
    static bool registered = false;
    if (!registered)
    {
        if (!EnsureSliderClass(instance_))
            return false;

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = instance_;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.hbrBackground = nullptr;
        windowClass.lpszClassName = kClassName;
        if (!RegisterClassExW(&windowClass))
            return false;
        registered = true;
    }

    if (window_ && IsWindow(window_))
        return true;

    RECT clientRect{ 0, 0, kWindowWidth, kClientHeight };
    const DWORD style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
    const DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT | WS_EX_LAYERED;
    AdjustWindowRectEx(&clientRect, style, FALSE, exStyle);

    window_ = CreateWindowExW(exStyle, kClassName,
        Text(chineseLanguage_, L"\u67E5\u627E / \u66FF\u6362", L"Find / Replace"),
        style, CW_USEDEFAULT, CW_USEDEFAULT,
        clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        owner, nullptr, instance_, this);
    return window_ != nullptr;
}

void OpenEditFindWindow::CreateControls()
{
    CreateControl(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, 0, IDC_FIND_TEXT);
    CreateControl(L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, 0, IDC_REPLACE_TEXT);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_FIND_NEXT);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_FIND_PREVIOUS);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_COUNT);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_MARK);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_REPLACE);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_REPLACE_ALL);

    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_REVERSE);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_MATCH_CASE);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_WRAP);

    CreateControl(L"STATIC", L"", SS_LEFTNOWORDWRAP, 0, IDC_MODE_LABEL);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 0, IDC_MODE_NORMAL);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_MODE_REGEX);

    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_OPACITY_ENABLED);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_GROUP | WS_TABSTOP, 0, IDC_OPACITY_ON_BLUR);
    CreateControl(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, 0, IDC_OPACITY_ALWAYS);
    CreateControl(kSliderClassName, L"", WS_TABSTOP, 0, IDC_OPACITY_SLIDER);
    CreateControl(L"STATIC", L"", SS_RIGHT | SS_LEFTNOWORDWRAP, 0, IDC_OPACITY_VALUE);
    CreateControl(L"STATIC", L"", SS_LEFTNOWORDWRAP, 0, IDC_STATUS);

    SendMessageW(GetDlgItem(window_, IDC_FIND_TEXT), EM_LIMITTEXT, 255, 0);
    SendMessageW(GetDlgItem(window_, IDC_REPLACE_TEXT), EM_LIMITTEXT, 255, 0);
    SendMessageW(GetDlgItem(window_, IDC_FIND_TEXT), EM_SETCUEBANNER, TRUE,
        reinterpret_cast<LPARAM>(Text(chineseLanguage_, L"\u67E5\u627E", L"Find")));
    SendMessageW(GetDlgItem(window_, IDC_REPLACE_TEXT), EM_SETCUEBANNER, TRUE,
        reinterpret_cast<LPARAM>(Text(chineseLanguage_, L"\u66FF\u6362\u4E3A", L"Replace with")));

    CheckDlgButton(window_, IDC_WRAP, BST_CHECKED);
    SetOptionChecked(IDC_WRAP, true);
    CheckRadioButton(window_, IDC_MODE_NORMAL, IDC_MODE_REGEX, IDC_MODE_NORMAL);
    CheckRadioButton(window_, IDC_OPACITY_ON_BLUR, IDC_OPACITY_ALWAYS, IDC_OPACITY_ON_BLUR);
    SendMessageW(GetDlgItem(window_, IDC_OPACITY_SLIDER), TBM_SETRANGE, TRUE, MAKELPARAM(0, kOpacityMaxPercent));
    SendMessageW(GetDlgItem(window_, IDC_OPACITY_SLIDER), TBM_SETPOS, TRUE, 0);
    UpdateOpacityControls();
}

HWND OpenEditFindWindow::CreateControl(const wchar_t* className, const wchar_t* text,
    DWORD style, DWORD exStyle, int id)
{
    HWND control = CreateWindowExW(exStyle, className, text, WS_CHILD | WS_VISIBLE | style,
        0, 0, 0, 0, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SetDefaultFont(control);
    return control;
}

void OpenEditFindWindow::LayoutControls()
{
    if (!window_)
        return;

    const int margin = 18;
    const int gap = 8;
    const int rowHeight = 28;
    const int inputWidth = 342;
    const int smallButtonWidth = 66;
    const int countWidth = 58;
    const int markWidth = 58;
    const int replaceAllWidth = 92;
    const int findY = 18;
    const int replaceY = 58;
    const int panelY = 106;
    const int panelHeight = 104;
    const int statusY = 224;
    const int firstButtonX = margin + inputWidth + gap;
    const int secondButtonX = firstButtonX + smallButtonWidth + gap;
    const int thirdButtonX = secondButtonX + smallButtonWidth + gap;
    const int fourthButtonX = thirdButtonX + countWidth + gap;

    findEditFrame_ = RECT{ margin, findY, margin + inputWidth, findY + rowHeight };
    replaceEditFrame_ = RECT{ margin, replaceY, margin + inputWidth, replaceY + rowHeight };

    MoveWindow(GetDlgItem(window_, IDC_FIND_TEXT),
        findEditFrame_.left + kEditInsetX, findEditFrame_.top + kEditInsetY,
        inputWidth - (kEditInsetX * 2), rowHeight - (kEditInsetY * 2), TRUE);
    MoveWindow(GetDlgItem(window_, IDC_FIND_NEXT), firstButtonX, findY, smallButtonWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_FIND_PREVIOUS), secondButtonX, findY, smallButtonWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_COUNT), thirdButtonX, findY, countWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_MARK), fourthButtonX, findY, markWidth, rowHeight, TRUE);

    MoveWindow(GetDlgItem(window_, IDC_REPLACE_TEXT),
        replaceEditFrame_.left + kEditInsetX, replaceEditFrame_.top + kEditInsetY,
        inputWidth - (kEditInsetX * 2), rowHeight - (kEditInsetY * 2), TRUE);
    MoveWindow(GetDlgItem(window_, IDC_REPLACE), firstButtonX, replaceY, smallButtonWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_REPLACE_ALL), secondButtonX, replaceY, replaceAllWidth, rowHeight, TRUE);

    const int col1X = margin + 12;
    const int col2X = 226;
    const int col3X = 426;
    MoveWindow(GetDlgItem(window_, IDC_REVERSE), col1X, panelY + 14, 120, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_MATCH_CASE), col1X, panelY + 40, 136, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_WRAP), col1X, panelY + 66, 120, 22, TRUE);

    MoveWindow(GetDlgItem(window_, IDC_MODE_LABEL), col2X, panelY + 10, 150, 18, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_MODE_NORMAL), col2X, panelY + 36, 130, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_MODE_REGEX), col2X, panelY + 62, 150, 22, TRUE);

    MoveWindow(GetDlgItem(window_, IDC_OPACITY_ENABLED), col3X, panelY + 10, 116, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_OPACITY_ON_BLUR), col3X, panelY + 36, 92, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_OPACITY_ALWAYS), col3X + 92, panelY + 36, 70, 22, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_OPACITY_SLIDER), col3X, panelY + 66, 120, 28, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_OPACITY_VALUE), col3X + 126, panelY + 68, 42, 18, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_STATUS), margin, statusY, kWindowWidth - margin * 2, 20, TRUE);
}

void OpenEditFindWindow::PositionWindow()
{
    if (!window_ || !owner_)
        return;

    RECT ownerRect{};
    GetWindowRect(owner_, &ownerRect);

    RECT windowRect{};
    GetWindowRect(window_, &windowRect);
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    int x = hasLastPosition_ ? lastPosition_.x :
        ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
    int y = hasLastPosition_ ? lastPosition_.y :
        ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

    HMONITOR monitor = hasLastPosition_ ?
        MonitorFromPoint(lastPosition_, MONITOR_DEFAULTTONEAREST) :
        MonitorFromWindow(owner_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{ sizeof(monitorInfo) };
    if (monitor && GetMonitorInfoW(monitor, &monitorInfo))
    {
        const RECT& work = monitorInfo.rcWork;
        const int workLeft = static_cast<int>(work.left);
        const int workTop = static_cast<int>(work.top);
        const int maxX = (std::max)(workLeft, static_cast<int>(work.right) - width);
        const int maxY = (std::max)(workTop, static_cast<int>(work.bottom) - height);
        x = (std::min)((std::max)(x, workLeft), maxX);
        y = (std::min)((std::max)(y, workTop), maxY);
    }

    SetWindowPos(window_, HWND_TOP, x, y, width, height, SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    LayoutControls();
}

void OpenEditFindWindow::SaveWindowPosition()
{
    if (!window_)
        return;

    RECT windowRect{};
    if (!GetWindowRect(window_, &windowRect))
        return;

    lastPosition_ = POINT{ windowRect.left, windowRect.top };
    hasLastPosition_ = true;
}

void OpenEditFindWindow::UpdateTexts()
{
    if (!window_)
        return;

    SetWindowTextW(window_, Text(chineseLanguage_, L"\u67E5\u627E / \u66FF\u6362", L"Find / Replace"));
    SetWindowTextW(GetDlgItem(window_, IDC_FIND_NEXT), Text(chineseLanguage_, L"\u4E0B\u4E00\u4E2A", L"Next"));
    SetWindowTextW(GetDlgItem(window_, IDC_FIND_PREVIOUS), Text(chineseLanguage_, L"\u4E0A\u4E00\u4E2A", L"Prev"));
    SetWindowTextW(GetDlgItem(window_, IDC_COUNT), Text(chineseLanguage_, L"\u8BA1\u6570", L"Count"));
    SetWindowTextW(GetDlgItem(window_, IDC_MARK), Text(chineseLanguage_, L"\u6807\u8BB0", L"Mark"));
    SetWindowTextW(GetDlgItem(window_, IDC_REPLACE), Text(chineseLanguage_, L"\u66FF\u6362", L"Replace"));
    SetWindowTextW(GetDlgItem(window_, IDC_REPLACE_ALL), Text(chineseLanguage_, L"\u5168\u90E8\u66FF\u6362", L"All"));
    SetWindowTextW(GetDlgItem(window_, IDC_REVERSE), Text(chineseLanguage_, L"\u53CD\u5411", L"Reverse"));
    SetWindowTextW(GetDlgItem(window_, IDC_MATCH_CASE), Text(chineseLanguage_, L"\u5339\u914D\u5927\u5C0F\u5199", L"Match case"));
    SetWindowTextW(GetDlgItem(window_, IDC_WRAP), Text(chineseLanguage_, L"\u5FAA\u73AF\u67E5\u627E", L"Wrap search"));
    SetWindowTextW(GetDlgItem(window_, IDC_MODE_LABEL), Text(chineseLanguage_, L"\u67E5\u627E\u6A21\u5F0F", L"Find mode"));
    SetWindowTextW(GetDlgItem(window_, IDC_MODE_NORMAL), Text(chineseLanguage_, L"\u666E\u901A", L"Normal"));
    SetWindowTextW(GetDlgItem(window_, IDC_MODE_REGEX), Text(chineseLanguage_, L"\u6B63\u5219\u8868\u8FBE\u5F0F", L"Regex"));
    SetWindowTextW(GetDlgItem(window_, IDC_OPACITY_ENABLED), Text(chineseLanguage_, L"\u900F\u660E\u5EA6", L"Opacity"));
    SetWindowTextW(GetDlgItem(window_, IDC_OPACITY_ON_BLUR), Text(chineseLanguage_, L"\u5931\u53BB\u7126\u70B9\u540E", L"On blur"));
    SetWindowTextW(GetDlgItem(window_, IDC_OPACITY_ALWAYS), Text(chineseLanguage_, L"\u59CB\u7EC8", L"Always"));
    SendMessageW(GetDlgItem(window_, IDC_FIND_TEXT), EM_SETCUEBANNER, TRUE,
        reinterpret_cast<LPARAM>(Text(chineseLanguage_, L"\u67E5\u627E", L"Find")));
    SendMessageW(GetDlgItem(window_, IDC_REPLACE_TEXT), EM_SETCUEBANNER, TRUE,
        reinterpret_cast<LPARAM>(Text(chineseLanguage_, L"\u66FF\u6362\u4E3A", L"Replace with")));

    SetStatus(statusText_);
    UpdateOpacityControls();
    LayoutControls();
}

void OpenEditFindWindow::UpdateBrushes()
{
    DeleteObject(backgroundBrush_);
    DeleteObject(surfaceBrush_);
    DeleteObject(editBrush_);
    backgroundBrush_ = CreateSolidBrush(PanelBack(darkTheme_));
    surfaceBrush_ = CreateSolidBrush(PanelSurface(darkTheme_));
    editBrush_ = CreateSolidBrush(EditBack(darkTheme_));
}

void OpenEditFindWindow::RedrawThemedControls()
{
    if (!window_)
        return;

    RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void OpenEditFindWindow::RedrawControl(int id)
{
    HWND control = GetDlgItem(window_, id);
    if (control)
        RedrawWindow(control, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
}

bool OpenEditFindWindow::IsOptionChecked(int id) const
{
    switch (id)
    {
    case IDC_REVERSE:
        return reverse_;
    case IDC_MATCH_CASE:
        return matchCase_;
    case IDC_WRAP:
        return wrap_;
    case IDC_MODE_NORMAL:
        return !regex_;
    case IDC_MODE_REGEX:
        return regex_;
    case IDC_OPACITY_ENABLED:
        return opacityEnabled_;
    case IDC_OPACITY_ON_BLUR:
        return !opacityAlways_;
    case IDC_OPACITY_ALWAYS:
        return opacityAlways_;
    default:
        return false;
    }
}

void OpenEditFindWindow::SetOptionChecked(int id, bool checked)
{
    switch (id)
    {
    case IDC_REVERSE:
        reverse_ = checked;
        break;
    case IDC_MATCH_CASE:
        matchCase_ = checked;
        break;
    case IDC_WRAP:
        wrap_ = checked;
        break;
    case IDC_MODE_NORMAL:
        regex_ = !checked;
        break;
    case IDC_MODE_REGEX:
        regex_ = checked;
        break;
    case IDC_OPACITY_ENABLED:
        opacityEnabled_ = checked;
        break;
    case IDC_OPACITY_ON_BLUR:
        opacityAlways_ = !checked;
        break;
    case IDC_OPACITY_ALWAYS:
        opacityAlways_ = checked;
        break;
    default:
        return;
    }
    CheckDlgButton(window_, id, checked ? BST_CHECKED : BST_UNCHECKED);
}

void OpenEditFindWindow::ToggleCheckBox(int id)
{
    SetOptionChecked(id, !IsOptionChecked(id));
    RedrawControl(id);
}

void OpenEditFindWindow::SelectRadio(int firstId, int lastId, int selectedId)
{
    CheckRadioButton(window_, firstId, lastId, selectedId);
    if (firstId == IDC_MODE_NORMAL)
        regex_ = selectedId == IDC_MODE_REGEX;
    else if (firstId == IDC_OPACITY_ON_BLUR)
        opacityAlways_ = selectedId == IDC_OPACITY_ALWAYS;
    for (int id = firstId; id <= lastId; ++id)
        RedrawControl(id);
}

void OpenEditFindWindow::UpdateOpacityControls()
{
    if (!window_)
        return;

    const int opacity = static_cast<int>(SendMessageW(GetDlgItem(window_, IDC_OPACITY_SLIDER), TBM_GETPOS, 0, 0));
    SetWindowTextW(GetDlgItem(window_, IDC_OPACITY_VALUE), (std::to_wstring(opacity) + L"%").c_str());
    ApplyOpacity(active_);
    RedrawWindow(GetDlgItem(window_, IDC_OPACITY_SLIDER), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    RedrawWindow(GetDlgItem(window_, IDC_OPACITY_VALUE), nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
}

void OpenEditFindWindow::ApplyOpacity(bool active)
{
    if (!window_)
        return;

    BYTE alpha = 255;
    const int opacity = static_cast<int>(SendMessageW(GetDlgItem(window_, IDC_OPACITY_SLIDER), TBM_GETPOS, 0, 0));
    if (opacityEnabled_ && (opacityAlways_ || !active))
    {
        const int clamped = (std::min)((std::max)(opacity, 0), kOpacityMaxPercent);
        alpha = static_cast<BYTE>((std::max)(60, 255 - static_cast<int>(std::round(255.0 * clamped / 100.0))));
    }
    SetLayeredWindowAttributes(window_, 0, alpha, LWA_ALPHA);
}

void OpenEditFindWindow::SetStatus(const std::wstring& text)
{
    statusText_ = text;
    if (window_)
        SetWindowTextW(GetDlgItem(window_, IDC_STATUS), statusText_.c_str());
}

OpenEditFindRequest OpenEditFindWindow::BuildRequest() const
{
    OpenEditFindRequest request;
    if (!window_)
        return request;

    request.findText = GetControlText(window_, IDC_FIND_TEXT);
    request.replaceText = GetControlText(window_, IDC_REPLACE_TEXT);
    request.reverse = reverse_;
    request.matchCase = matchCase_;
    request.wrap = wrap_;
    request.regex = regex_;
    return request;
}

void OpenEditFindWindow::FocusFindText()
{
    HWND findText = GetDlgItem(window_, IDC_FIND_TEXT);
    if (findText)
        SetFocus(findText);
}

void OpenEditFindWindow::ExecuteFind(bool previous)
{
    const OpenEditFindRequest request = BuildRequest();
    const bool found = callbacks_.find && callbacks_.find(callbacks_.context, request, previous);
    SetStatus(found ? Text(chineseLanguage_, L"\u5DF2\u627E\u5230", L"Found") :
        Text(chineseLanguage_, L"\u672A\u627E\u5230", L"Not found"));
    FocusFindText();
}

void OpenEditFindWindow::ExecuteCount()
{
    const OpenEditFindRequest request = BuildRequest();
    const int count = callbacks_.count ? callbacks_.count(callbacks_.context, request) : 0;
    SetStatus(std::wstring(chineseLanguage_ ? L"\u8BA1\u6570: " : L"Count: ") + std::to_wstring(count));
    FocusFindText();
}

void OpenEditFindWindow::ExecuteMark()
{
    const OpenEditFindRequest request = BuildRequest();
    const int count = callbacks_.mark ? callbacks_.mark(callbacks_.context, request) : 0;
    SetStatus(std::wstring(chineseLanguage_ ? L"\u5DF2\u6807\u8BB0: " : L"Marked: ") + std::to_wstring(count));
    FocusFindText();
}

void OpenEditFindWindow::ExecuteReplace()
{
    const OpenEditFindRequest request = BuildRequest();
    const bool replaced = callbacks_.replace && callbacks_.replace(callbacks_.context, request);
    SetStatus(replaced ? Text(chineseLanguage_, L"\u5DF2\u66FF\u6362", L"Replaced") :
        Text(chineseLanguage_, L"\u672A\u627E\u5230", L"Not found"));
    FocusFindText();
}

void OpenEditFindWindow::ExecuteReplaceAll()
{
    const OpenEditFindRequest request = BuildRequest();
    const int count = callbacks_.replaceAll ? callbacks_.replaceAll(callbacks_.context, request) : 0;
    SetStatus(std::wstring(chineseLanguage_ ? L"\u5DF2\u66FF\u6362: " : L"Replaced: ") + std::to_wstring(count));
    FocusFindText();
}

LRESULT CALLBACK OpenEditFindWindow::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    OpenEditFindWindow* self = reinterpret_cast<OpenEditFindWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE)
    {
        CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = reinterpret_cast<OpenEditFindWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->window_ = window;
    }

    return self ? self->HandleMessage(window, message, wParam, lParam) :
        DefWindowProcW(window, message, wParam, lParam);
}

LRESULT OpenEditFindWindow::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case kFindWindowGetDarkThemeMessage:
        return darkTheme_ ? TRUE : FALSE;

    case WM_CREATE:
        UpdateBrushes();
        CreateControls();
        UpdateTexts();
        return 0;

    case WM_COMMAND:
    {
        const int commandId = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        const bool clicked = notification == BN_CLICKED;
        if (clicked && commandId == IDC_FIND_NEXT)
            ExecuteFind(false);
        else if (clicked && commandId == IDC_FIND_PREVIOUS)
            ExecuteFind(true);
        else if (clicked && commandId == IDC_COUNT)
            ExecuteCount();
        else if (clicked && commandId == IDC_MARK)
            ExecuteMark();
        else if (clicked && commandId == IDC_REPLACE)
            ExecuteReplace();
        else if (clicked && commandId == IDC_REPLACE_ALL)
            ExecuteReplaceAll();
        else if (clicked && (commandId == IDC_REVERSE || commandId == IDC_MATCH_CASE || commandId == IDC_WRAP))
            ToggleCheckBox(commandId);
        else if (clicked && (commandId == IDC_MODE_NORMAL || commandId == IDC_MODE_REGEX))
            SelectRadio(IDC_MODE_NORMAL, IDC_MODE_REGEX, commandId);
        else if (clicked && commandId == IDC_OPACITY_ENABLED)
        {
            ToggleCheckBox(commandId);
            UpdateOpacityControls();
        }
        else if (clicked && (commandId == IDC_OPACITY_ON_BLUR || commandId == IDC_OPACITY_ALWAYS))
        {
            SelectRadio(IDC_OPACITY_ON_BLUR, IDC_OPACITY_ALWAYS, commandId);
            UpdateOpacityControls();
        }
        else if ((commandId == IDC_FIND_TEXT || commandId == IDC_REPLACE_TEXT) && notification == EN_CHANGE)
            SetStatus(L"");
        return 0;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (!drawItem)
            return 0;

        const int controlId = GetDlgCtrlID(drawItem->hwndItem);
        if (IsCheckBoxControl(controlId) || IsRadioControl(controlId))
        {
            DrawOptionControl(drawItem, darkTheme_, IsRadioControl(controlId), IsOptionChecked(controlId));
            return TRUE;
        }

        const bool pressed = (drawItem->itemState & ODS_SELECTED) != 0;
        const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;
        RECT rect = drawItem->rcItem;
        FillRect(drawItem->hDC, &rect, backgroundBrush_);
        DrawRoundedRect(drawItem->hDC, rect, ButtonBack(darkTheme_, pressed), focused ? Accent() : ButtonBorder(darkTheme_), 8);

        wchar_t text[128]{};
        GetWindowTextW(drawItem->hwndItem, text, static_cast<int>(sizeof(text) / sizeof(text[0])));
        DrawTextInRect(drawItem->hDC, rect, text, PanelText(darkTheme_), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == GetDlgItem(window_, IDC_OPACITY_SLIDER))
            UpdateOpacityControls();
        return 0;

    case WM_ACTIVATE:
        active_ = LOWORD(wParam) != WA_INACTIVE;
        ApplyOpacity(active_);
        return 0;

    case WM_CLOSE:
        Hide();
        return 0;

    case WM_SIZE:
        LayoutControls();
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(window, &ps);
        RECT clientRect{};
        GetClientRect(window, &clientRect);
        FillRect(hdc, &clientRect, backgroundBrush_);

        DrawRoundedRect(hdc, findEditFrame_, EditBack(darkTheme_), Border(darkTheme_), 10);
        DrawRoundedRect(hdc, replaceEditFrame_, EditBack(darkTheme_), Border(darkTheme_), 10);

        RECT column1{ 18, 100, 194, 214 };
        RECT column2{ 214, 100, 392, 214 };
        RECT column3{ 412, 100, 642, 214 };
        DrawRoundedRect(hdc, column1, PanelSurface(darkTheme_), Border(darkTheme_), 10);
        DrawRoundedRect(hdc, column2, PanelSurface(darkTheme_), Border(darkTheme_), 10);
        DrawRoundedRect(hdc, column3, PanelSurface(darkTheme_), Border(darkTheme_), 10);

        EndPaint(window, &ps);
        return 0;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, PanelText(darkTheme_));
        SetBkColor(hdc, EditBack(darkTheme_));
        return reinterpret_cast<LRESULT>(editBrush_);
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        const int controlId = GetDlgCtrlID(control);
        const bool panelControl = IsPanelControl(controlId);
        SetTextColor(hdc, controlId == IDC_STATUS || controlId == IDC_OPACITY_VALUE ?
            MutedText(darkTheme_) : PanelText(darkTheme_));
        SetBkMode(hdc, TRANSPARENT);
        SetBkColor(hdc, panelControl ? PanelSurface(darkTheme_) : PanelBack(darkTheme_));
        return reinterpret_cast<LRESULT>(panelControl ? surfaceBrush_ : backgroundBrush_);
    }

    case WM_NCDESTROY:
        if (window_ == window)
            window_ = nullptr;
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

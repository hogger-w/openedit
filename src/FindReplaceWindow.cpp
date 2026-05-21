#include "FindReplaceWindow.h"

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

namespace
{
constexpr wchar_t kClassName[] = L"OpenEditFindReplaceWindow";
constexpr wchar_t kSliderClassName[] = L"OpenEditOpacitySlider";
constexpr UINT kFindWindowGetDarkThemeMessage = WM_APP + 91;
constexpr int kWindowWidth = 660;
constexpr int kClientHeight = 254;
constexpr int kEditInsetX = 10;
constexpr int kEditInsetY = 5;
constexpr int kOpacityMaxPercent = 70;

constexpr int IDC_FIND_TEXT = 2102;
constexpr int IDC_REPLACE_TEXT = 2103;
constexpr int IDC_FIND_NEXT = 2104;
constexpr int IDC_FIND_PREVIOUS = 2105;
constexpr int IDC_COUNT = 2106;
constexpr int IDC_REPLACE = 2107;
constexpr int IDC_REPLACE_ALL = 2108;
constexpr int IDC_REVERSE = 2109;
constexpr int IDC_MATCH_CASE = 2110;
constexpr int IDC_STATUS = 2112;
constexpr int IDC_WRAP = 2116;
constexpr int IDC_MODE_LABEL = 2117;
constexpr int IDC_MODE_NORMAL = 2118;
constexpr int IDC_MODE_REGEX = 2119;
constexpr int IDC_OPACITY_ENABLED = 2120;
constexpr int IDC_OPACITY_ON_BLUR = 2121;
constexpr int IDC_OPACITY_ALWAYS = 2122;
constexpr int IDC_OPACITY_SLIDER = 2123;
constexpr int IDC_OPACITY_VALUE = 2124;

COLORREF PanelBack(bool dark) { return dark ? RGB(45, 45, 48) : RGB(248, 250, 252); }
COLORREF PanelSurface(bool dark) { return dark ? RGB(52, 52, 56) : RGB(255, 255, 255); }
COLORREF PanelText(bool dark) { return dark ? RGB(232, 232, 232) : RGB(31, 41, 55); }
COLORREF MutedText(bool dark) { return dark ? RGB(170, 170, 170) : RGB(91, 101, 113); }
COLORREF EditBack(bool dark) { return dark ? RGB(30, 30, 30) : RGB(255, 255, 255); }
COLORREF Border(bool dark) { return dark ? RGB(82, 82, 88) : RGB(200, 210, 222); }
COLORREF Accent() { return RGB(0, 120, 215); }
COLORREF ButtonBack(bool dark, bool pressed) { return pressed ? (dark ? RGB(70, 70, 76) : RGB(221, 232, 246)) : (dark ? RGB(58, 58, 63) : RGB(242, 246, 251)); }
COLORREF ButtonBorder(bool dark) { return dark ? RGB(94, 94, 102) : RGB(183, 195, 209); }
COLORREF SliderTrack(bool dark) { return dark ? RGB(78, 78, 84) : RGB(213, 221, 232); }
COLORREF SliderThumb(bool dark) { return dark ? RGB(230, 230, 230) : RGB(255, 255, 255); }

bool IsPanelControl(int controlId)
{
    switch (controlId)
    {
    case IDC_REVERSE:
    case IDC_MATCH_CASE:
    case IDC_WRAP:
    case IDC_MODE_LABEL:
    case IDC_MODE_NORMAL:
    case IDC_MODE_REGEX:
    case IDC_OPACITY_ENABLED:
    case IDC_OPACITY_ON_BLUR:
    case IDC_OPACITY_ALWAYS:
    case IDC_OPACITY_SLIDER:
    case IDC_OPACITY_VALUE:
        return true;
    default:
        return false;
    }
}

bool IsCheckBoxControl(int controlId)
{
    switch (controlId)
    {
    case IDC_REVERSE:
    case IDC_MATCH_CASE:
    case IDC_WRAP:
    case IDC_OPACITY_ENABLED:
        return true;
    default:
        return false;
    }
}

bool IsRadioControl(int controlId)
{
    switch (controlId)
    {
    case IDC_MODE_NORMAL:
    case IDC_MODE_REGEX:
    case IDC_OPACITY_ON_BLUR:
    case IDC_OPACITY_ALWAYS:
        return true;
    default:
        return false;
    }
}

const wchar_t* Text(bool chinese, const wchar_t* zh, const wchar_t* en)
{
    return chinese ? zh : en;
}

void SetDefaultFont(HWND control)
{
    if (control)
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
}

std::wstring GetControlText(HWND parent, int id)
{
    HWND control = GetDlgItem(parent, id);
    if (!control)
        return L"";

    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    if (length > 0)
        GetWindowTextW(control, &text[0], length + 1);
    text.resize(static_cast<size_t>(length));
    return text;
}

void DrawRoundedRect(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius)
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

void DrawTextInRect(HDC hdc, const RECT& rect, const std::wstring& text, COLORREF color, UINT format)
{
    RECT textRect = rect;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    DrawTextW(hdc, text.c_str(), -1, &textRect, format | DT_NOPREFIX);
}

void DrawOptionControl(const DRAWITEMSTRUCT* drawItem, bool dark, bool radio, bool checked)
{
    RECT rect = drawItem->rcItem;
    HBRUSH background = CreateSolidBrush(PanelSurface(dark));
    FillRect(drawItem->hDC, &rect, background);
    DeleteObject(background);

    const int glyphSize = 14;
    RECT glyph{
        rect.left + 1,
        rect.top + ((rect.bottom - rect.top) - glyphSize) / 2,
        rect.left + 1 + glyphSize,
        rect.top + ((rect.bottom - rect.top) - glyphSize) / 2 + glyphSize
    };

    const COLORREF border = checked ? Accent() : Border(dark);
    const COLORREF fill = checked ? Accent() : PanelSurface(dark);
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
            oldBrush = SelectObject(drawItem->hDC, dotBrush);
            HPEN dotPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
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
    DrawTextInRect(drawItem->hDC, textRect, text, PanelText(dark), DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    if (drawItem->itemState & ODS_FOCUS)
    {
        RECT focusRect = rect;
        focusRect.left = glyph.left;
        focusRect.right -= 2;
        DrawFocusRect(drawItem->hDC, &focusRect);
    }
}

struct SliderState
{
    int minimum = 0;
    int maximum = kOpacityMaxPercent;
    int position = 0;
    bool dragging = false;
};

int ClampSliderPosition(const SliderState& state, int position)
{
    return (std::min)((std::max)(position, state.minimum), state.maximum);
}

int SliderPositionFromPoint(HWND window, const SliderState& state, int x)
{
    RECT rect{};
    GetClientRect(window, &rect);
    const int thumbRadius = 7;
    const int start = rect.left + thumbRadius;
    const int end = rect.right - thumbRadius;
    if (end <= start || state.maximum <= state.minimum)
        return state.minimum;

    const int clampedX = (std::min)((std::max)(x, start), end);
    const double ratio = static_cast<double>(clampedX - start) / static_cast<double>(end - start);
    return ClampSliderPosition(state, state.minimum + static_cast<int>(std::round(ratio * (state.maximum - state.minimum))));
}

void NotifySliderChanged(HWND window, int code)
{
    SliderState* state = reinterpret_cast<SliderState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!state)
        return;
    SendMessageW(GetParent(window), WM_HSCROLL, MAKEWPARAM(code, state->position), reinterpret_cast<LPARAM>(window));
}

void SetSliderPosition(HWND window, SliderState& state, int position, bool notify)
{
    const int clamped = ClampSliderPosition(state, position);
    if (clamped == state.position)
        return;

    state.position = clamped;
    InvalidateRect(window, nullptr, TRUE);
    if (notify)
        NotifySliderChanged(window, TB_THUMBTRACK);
}

LRESULT CALLBACK SliderWndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    SliderState* state = reinterpret_cast<SliderState*>(GetWindowLongPtrW(window, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        state = new SliderState();
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        return 0;

    case TBM_SETRANGE:
        if (state)
        {
            state->minimum = LOWORD(lParam);
            state->maximum = HIWORD(lParam);
            state->position = ClampSliderPosition(*state, state->position);
            if (wParam)
                InvalidateRect(window, nullptr, TRUE);
        }
        return 0;

    case TBM_SETPOS:
        if (state)
        {
            state->position = ClampSliderPosition(*state, static_cast<int>(lParam));
            if (wParam)
                InvalidateRect(window, nullptr, TRUE);
        }
        return 0;

    case TBM_GETPOS:
        return state ? state->position : 0;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_LBUTTONDOWN:
        if (state)
        {
            SetFocus(window);
            SetCapture(window);
            state->dragging = true;
            SetSliderPosition(window, *state, SliderPositionFromPoint(window, *state, GET_X_LPARAM(lParam)), true);
        }
        return 0;

    case WM_MOUSEMOVE:
        if (state && state->dragging)
            SetSliderPosition(window, *state, SliderPositionFromPoint(window, *state, GET_X_LPARAM(lParam)), true);
        return 0;

    case WM_LBUTTONUP:
        if (state && state->dragging)
        {
            state->dragging = false;
            ReleaseCapture();
            SetSliderPosition(window, *state, SliderPositionFromPoint(window, *state, GET_X_LPARAM(lParam)), true);
            NotifySliderChanged(window, TB_ENDTRACK);
        }
        return 0;

    case WM_KEYDOWN:
        if (state)
        {
            int next = state->position;
            if (wParam == VK_LEFT)
                --next;
            else if (wParam == VK_RIGHT)
                ++next;
            else if (wParam == VK_HOME)
                next = state->minimum;
            else if (wParam == VK_END)
                next = state->maximum;
            else
                break;
            SetSliderPosition(window, *state, next, true);
        }
        return 0;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(window, nullptr, TRUE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(window, &ps);
        RECT rect{};
        GetClientRect(window, &rect);
        const bool dark = SendMessageW(GetParent(window), kFindWindowGetDarkThemeMessage, 0, 0) != 0;

        HBRUSH backBrush = CreateSolidBrush(PanelSurface(dark));
        FillRect(hdc, &rect, backBrush);
        DeleteObject(backBrush);

        const int thumbRadius = 7;
        const int y = (rect.top + rect.bottom) / 2;
        const int start = rect.left + thumbRadius;
        const int end = rect.right - thumbRadius;
        const double ratio = state && state->maximum > state->minimum ?
            static_cast<double>(state->position - state->minimum) / static_cast<double>(state->maximum - state->minimum) : 0.0;
        const int thumbX = start + static_cast<int>(std::round((end - start) * ratio));

        RECT track{ start, y - 2, end, y + 2 };
        DrawRoundedRect(hdc, track, SliderTrack(dark), SliderTrack(dark), 4);
        RECT activeTrack{ start, y - 2, thumbX, y + 2 };
        if (activeTrack.right > activeTrack.left)
            DrawRoundedRect(hdc, activeTrack, Accent(), Accent(), 4);

        HBRUSH thumbBrush = CreateSolidBrush(SliderThumb(dark));
        HPEN thumbPen = CreatePen(PS_SOLID, 1, Accent());
        HGDIOBJ oldBrush = SelectObject(hdc, thumbBrush);
        HGDIOBJ oldPen = SelectObject(hdc, thumbPen);
        Ellipse(hdc, thumbX - thumbRadius, y - thumbRadius, thumbX + thumbRadius, y + thumbRadius);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(thumbPen);
        DeleteObject(thumbBrush);

        if (GetFocus() == window)
        {
            RECT focusRect = rect;
            InflateRect(&focusRect, -1, -1);
            DrawFocusRect(hdc, &focusRect);
        }

        EndPaint(window, &ps);
        return 0;
    }

    case WM_NCDESTROY:
        delete state;
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

bool EnsureSliderClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered)
        return true;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = SliderWndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kSliderClassName;
    registered = RegisterClassExW(&windowClass) != 0;
    return registered;
}

void ApplyWindowDarkMode(HWND window, bool darkTheme)
{
    if (!window)
        return;

    const BOOL dark = darkTheme ? TRUE : FALSE;
    DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}
}

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
    const int inputWidth = 372;
    const int smallButtonWidth = 66;
    const int countWidth = 58;
    const int replaceAllWidth = 92;
    const int findY = 18;
    const int replaceY = 58;
    const int panelY = 106;
    const int panelHeight = 104;
    const int statusY = 224;
    const int firstButtonX = margin + inputWidth + gap;
    const int secondButtonX = firstButtonX + smallButtonWidth + gap;
    const int thirdButtonX = secondButtonX + smallButtonWidth + gap;

    findEditFrame_ = RECT{ margin, findY, margin + inputWidth, findY + rowHeight };
    replaceEditFrame_ = RECT{ margin, replaceY, margin + inputWidth, replaceY + rowHeight };

    MoveWindow(GetDlgItem(window_, IDC_FIND_TEXT),
        findEditFrame_.left + kEditInsetX, findEditFrame_.top + kEditInsetY,
        inputWidth - (kEditInsetX * 2), rowHeight - (kEditInsetY * 2), TRUE);
    MoveWindow(GetDlgItem(window_, IDC_FIND_NEXT), firstButtonX, findY, smallButtonWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_FIND_PREVIOUS), secondButtonX, findY, smallButtonWidth, rowHeight, TRUE);
    MoveWindow(GetDlgItem(window_, IDC_COUNT), thirdButtonX, findY, countWidth, rowHeight, TRUE);

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

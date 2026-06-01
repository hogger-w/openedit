#include "FindReplaceWindowSupport.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <algorithm>
#include <cmath>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace FindReplaceSupport
{
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

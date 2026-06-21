#include "openedit_internal.h"

namespace
{
inline constexpr UINT_PTR kTabTooltipHoverTimerId = 1;
inline constexpr UINT_PTR kTabScrollAnimationTimerId = 2;
inline constexpr UINT kTabTooltipHoverDelayMs = 500;
inline constexpr UINT kTabScrollAnimationFrameMs = 16;
inline constexpr DWORD kTabScrollAnimationDurationMs = 160;
inline constexpr int kTabScrollButtonWidth = 24;
inline constexpr int kTabScrollButtonCount = 2;
inline constexpr int kTabWheelScrollPixels = 72;

int g_pendingTabTooltipIndex = -1;
POINT g_pendingTabTooltipPoint{};
bool g_tabTooltipVisible = false;
int g_visibleTabTooltipIndex = -1;
int g_tabScrollOffset = 0;
int g_hoveredTabScrollButton = 0;
bool g_tabScrollAnimationActive = false;
int g_tabScrollAnimationStartOffset = 0;
int g_tabScrollAnimationTargetOffset = 0;
DWORD g_tabScrollAnimationStartTick = 0;

void CancelPendingTabTooltip(HWND tabBar)
{
    HWND timerWindow = tabBar ? tabBar : g_hTabBar;
    if (timerWindow && IsWindow(timerWindow))
        KillTimer(timerWindow, kTabTooltipHoverTimerId);

    g_pendingTabTooltipIndex = -1;
    g_pendingTabTooltipPoint = POINT{};
}

void StopTabScrollAnimation(HWND tabBar)
{
    HWND timerWindow = tabBar ? tabBar : g_hTabBar;
    if (timerWindow && IsWindow(timerWindow))
        KillTimer(timerWindow, kTabScrollAnimationTimerId);

    g_tabScrollAnimationActive = false;
}

int TabScrollButtonAreaWidth()
{
    return kTabScrollButtonWidth * kTabScrollButtonCount;
}

int TabContentWidth()
{
    return static_cast<int>(g_tabs.size()) * kTabWidth;
}

bool HasTabOverflow(HWND tabBar)
{
    RECT clientRect{};
    GetClientRect(tabBar, &clientRect);
    return TabContentWidth() > (clientRect.right - clientRect.left);
}

RECT GetTabViewportRect(HWND tabBar)
{
    RECT rect{};
    GetClientRect(tabBar, &rect);
    if (HasTabOverflow(tabBar))
        rect.right = (std::max)(rect.left, rect.right - TabScrollButtonAreaWidth());
    return rect;
}

int MaxTabScrollOffset(HWND tabBar)
{
    const RECT viewportRect = GetTabViewportRect(tabBar);
    const int viewportWidth = (std::max)(0, static_cast<int>(viewportRect.right - viewportRect.left));
    return (std::max)(0, TabContentWidth() - viewportWidth);
}

void ClampTabScrollOffset(HWND tabBar)
{
    if (!HasTabOverflow(tabBar))
    {
        g_tabScrollOffset = 0;
        return;
    }

    g_tabScrollOffset = (std::min)((std::max)(g_tabScrollOffset, 0), MaxTabScrollOffset(tabBar));
}

RECT GetTabScrollButtonRect(HWND tabBar, int direction)
{
    RECT clientRect{};
    GetClientRect(tabBar, &clientRect);
    RECT rect{
        clientRect.right - TabScrollButtonAreaWidth(),
        clientRect.top,
        clientRect.right - kTabScrollButtonWidth,
        clientRect.bottom
    };
    if (direction > 0)
    {
        rect.left += kTabScrollButtonWidth;
        rect.right += kTabScrollButtonWidth;
    }
    return rect;
}

int HitTestTabScrollButton(HWND tabBar, POINT point)
{
    if (!HasTabOverflow(tabBar))
        return 0;

    const RECT leftRect = GetTabScrollButtonRect(tabBar, -1);
    if (PtInRect(&leftRect, point))
        return -1;

    const RECT rightRect = GetTabScrollButtonRect(tabBar, 1);
    if (PtInRect(&rightRect, point))
        return 1;

    return 0;
}

bool CanScrollTabs(HWND tabBar, int direction)
{
    ClampTabScrollOffset(tabBar);
    return direction < 0 ? g_tabScrollOffset > 0 : g_tabScrollOffset < MaxTabScrollOffset(tabBar);
}

bool ScrollTabsBy(HWND tabBar, int delta)
{
    StopTabScrollAnimation(tabBar);

    if (!HasTabOverflow(tabBar) || delta == 0)
    {
        ClampTabScrollOffset(tabBar);
        return false;
    }

    const int oldOffset = g_tabScrollOffset;
    g_tabScrollOffset += delta;
    ClampTabScrollOffset(tabBar);
    if (g_tabScrollOffset == oldOffset)
        return false;

    CancelPendingTabTooltip(tabBar);
    g_tabTooltipVisible = false;
    g_visibleTabTooltipIndex = -1;
    if (g_hTabTooltip && IsWindow(g_hTabTooltip))
    {
        TOOLINFOW toolInfo{};
        toolInfo.cbSize = sizeof(toolInfo);
        toolInfo.hwnd = tabBar;
        toolInfo.uId = 1;
        SendMessageW(g_hTabTooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&toolInfo));
    }
    InvalidateRect(tabBar, nullptr, TRUE);
    return true;
}

bool AnimateTabsBy(HWND tabBar, int delta)
{
    if (!HasTabOverflow(tabBar) || delta == 0)
    {
        ClampTabScrollOffset(tabBar);
        return false;
    }

    ClampTabScrollOffset(tabBar);
    const int oldOffset = g_tabScrollOffset;
    const int targetOffset = (std::min)((std::max)(oldOffset + delta, 0), MaxTabScrollOffset(tabBar));
    if (targetOffset == oldOffset)
        return false;

    CancelPendingTabTooltip(tabBar);
    g_tabTooltipVisible = false;
    g_visibleTabTooltipIndex = -1;
    g_tabScrollAnimationActive = true;
    g_tabScrollAnimationStartOffset = oldOffset;
    g_tabScrollAnimationTargetOffset = targetOffset;
    g_tabScrollAnimationStartTick = GetTickCount();
    SetTimer(tabBar, kTabScrollAnimationTimerId, kTabScrollAnimationFrameMs, nullptr);
    return true;
}

void UpdateTabScrollAnimation(HWND tabBar)
{
    if (!g_tabScrollAnimationActive)
        return;

    if (!HasTabOverflow(tabBar))
    {
        StopTabScrollAnimation(tabBar);
        g_tabScrollOffset = 0;
        InvalidateRect(tabBar, nullptr, TRUE);
        return;
    }

    const DWORD elapsed = GetTickCount() - g_tabScrollAnimationStartTick;
    if (elapsed >= kTabScrollAnimationDurationMs)
    {
        g_tabScrollOffset = g_tabScrollAnimationTargetOffset;
        ClampTabScrollOffset(tabBar);
        StopTabScrollAnimation(tabBar);
        InvalidateRect(tabBar, nullptr, TRUE);
        return;
    }

    const double t = static_cast<double>(elapsed) / static_cast<double>(kTabScrollAnimationDurationMs);
    const double remaining = 1.0 - t;
    const double eased = 1.0 - (remaining * remaining * remaining);
    const int delta = g_tabScrollAnimationTargetOffset - g_tabScrollAnimationStartOffset;
    g_tabScrollOffset = g_tabScrollAnimationStartOffset +
        static_cast<int>((delta * eased) + (delta >= 0 ? 0.5 : -0.5));
    ClampTabScrollOffset(tabBar);
    InvalidateRect(tabBar, nullptr, TRUE);
}
}

void EnsureActiveTabVisible()
{
    StopTabScrollAnimation(g_hTabBar);

    if (!g_hTabBar || !IsWindow(g_hTabBar) || g_activeTabIndex < 0)
        return;

    if (!HasTabOverflow(g_hTabBar))
    {
        g_tabScrollOffset = 0;
        return;
    }

    const RECT viewportRect = GetTabViewportRect(g_hTabBar);
    const int viewportWidth = (std::max)(0, static_cast<int>(viewportRect.right - viewportRect.left));
    const int tabLeft = g_activeTabIndex * kTabWidth;
    const int tabRight = tabLeft + kTabWidth;
    if (tabLeft < g_tabScrollOffset)
        g_tabScrollOffset = tabLeft;
    else if (tabRight > g_tabScrollOffset + viewportWidth)
        g_tabScrollOffset = tabRight - viewportWidth;

    ClampTabScrollOffset(g_hTabBar);
}

int GetTabBarTabWidth(HWND tabBar)
{
    UNREFERENCED_PARAMETER(tabBar);
    return kTabWidth;
}

RECT GetTabBarTabRect(HWND tabBar, int tabIndex)
{
    ClampTabScrollOffset(tabBar);

    const RECT viewportRect = GetTabViewportRect(tabBar);
    const int tabWidth = GetTabBarTabWidth(tabBar);
    const int left = viewportRect.left + (tabIndex * tabWidth) - g_tabScrollOffset;
    return RECT{ left, viewportRect.top, left + tabWidth, viewportRect.bottom };
}

RECT GetTabCloseButtonRect(const RECT& tabRect)
{
    const int top = tabRect.top + ((tabRect.bottom - tabRect.top - kTabCloseSize) / 2);
    return RECT{
        tabRect.right - kTabCloseSize - 6,
        top,
        tabRect.right - 6,
        top + kTabCloseSize
    };
}

RECT GetTabSaveIconRect(const RECT& tabRect)
{
    const int top = tabRect.top + ((tabRect.bottom - tabRect.top - kTabSaveIconSize) / 2);
    return RECT{
        tabRect.left + 8,
        top,
        tabRect.left + 8 + kTabSaveIconSize,
        top + kTabSaveIconSize
    };
}

int HitTestTabBar(HWND tabBar, POINT point, bool* closeButton)
{
    if (closeButton)
        *closeButton = false;

    ClampTabScrollOffset(tabBar);
    const RECT viewportRect = GetTabViewportRect(tabBar);
    if (!PtInRect(&viewportRect, point))
        return -1;

    const int count = static_cast<int>(g_tabs.size());
    for (int index = 0; index < count; ++index)
    {
        const RECT tabRect = GetTabBarTabRect(tabBar, index);
        if (!PtInRect(&tabRect, point))
            continue;

        const RECT closeRect = GetTabCloseButtonRect(tabRect);
        if (closeButton)
            *closeButton = PtInRect(&closeRect, point);
        return index;
    }

    return -1;
}

HWND EnsureTabTooltip(HWND tabBar)
{
    if (g_hTabTooltip && IsWindow(g_hTabTooltip))
        return g_hTabTooltip;

    g_hTabTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        tabBar, nullptr, hInst, nullptr);
    if (!g_hTabTooltip)
        return nullptr;

    SetWindowPos(g_hTabTooltip, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    SendMessageW(g_hTabTooltip, TTM_SETMAXTIPWIDTH, 0, 480);
    SendMessageW(g_hTabTooltip, TTM_SETDELAYTIME, TTDT_INITIAL, 250);

    TOOLINFOW toolInfo{};
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.uFlags = TTF_TRACK | TTF_ABSOLUTE;
    toolInfo.hwnd = tabBar;
    toolInfo.uId = 1;
    toolInfo.lpszText = const_cast<LPWSTR>(L"");
    GetClientRect(tabBar, &toolInfo.rect);
    SendMessageW(g_hTabTooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&toolInfo));
    return g_hTabTooltip;
}

void HideTabTooltip()
{
    CancelPendingTabTooltip(g_hTabBar);
    g_tabTooltipVisible = false;
    g_visibleTabTooltipIndex = -1;

    if (!g_hTabTooltip || !IsWindow(g_hTabTooltip))
        return;

    TOOLINFOW toolInfo{};
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = g_hTabBar;
    toolInfo.uId = 1;
    SendMessageW(g_hTabTooltip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&toolInfo));
}

void UpdateTabTooltip(HWND tabBar, int tabIndex, POINT point)
{
    UNREFERENCED_PARAMETER(point);

    if (tabIndex < 0)
    {
        HideTabTooltip();
        return;
    }

    HWND tooltip = EnsureTabTooltip(tabBar);
    if (!tooltip)
        return;

    g_tabTooltipText = GetTabTooltipText(tabIndex);
    if (g_tabTooltipText.empty())
    {
        HideTabTooltip();
        return;
    }

    TOOLINFOW toolInfo{};
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = tabBar;
    toolInfo.uId = 1;
    toolInfo.lpszText = const_cast<LPWSTR>(g_tabTooltipText.c_str());
    SendMessageW(tooltip, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&toolInfo));

    const RECT tabRect = GetTabBarTabRect(tabBar, tabIndex);
    const RECT viewportRect = GetTabViewportRect(tabBar);
    POINT screenPoint{ (std::max)(tabRect.left, viewportRect.left) + 12, tabRect.bottom + 4 };
    ClientToScreen(tabBar, &screenPoint);
    SendMessageW(tooltip, TTM_TRACKPOSITION, 0, MAKELPARAM(screenPoint.x, screenPoint.y));
    SendMessageW(tooltip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&toolInfo));
    g_tabTooltipVisible = true;
    g_visibleTabTooltipIndex = tabIndex;
}

void ScheduleTabTooltip(HWND tabBar, int tabIndex, POINT point)
{
    if (tabIndex < 0)
    {
        HideTabTooltip();
        return;
    }

    if (g_tabTooltipVisible && tabIndex == g_visibleTabTooltipIndex)
    {
        return;
    }

    if (g_pendingTabTooltipIndex == tabIndex)
    {
        g_pendingTabTooltipPoint = point;
        return;
    }

    HideTabTooltip();
    g_pendingTabTooltipIndex = tabIndex;
    g_pendingTabTooltipPoint = point;
    SetTimer(tabBar, kTabTooltipHoverTimerId, kTabTooltipHoverDelayMs, nullptr);
}

void ShowPendingTabTooltip(HWND tabBar)
{
    const int pendingTabIndex = g_pendingTabTooltipIndex;
    CancelPendingTabTooltip(tabBar);
    if (pendingTabIndex < 0)
        return;

    POINT point{};
    if (!GetCursorPos(&point))
        point = g_pendingTabTooltipPoint;
    else
        ScreenToClient(tabBar, &point);

    bool closeButton = false;
    const int currentTabIndex = HitTestTabBar(tabBar, point, &closeButton);
    if (currentTabIndex != pendingTabIndex || currentTabIndex != g_hoveredTabIndex)
        return;

    UpdateTabTooltip(tabBar, currentTabIndex, point);
}

void UpdateTabHoverState(HWND tabBar, POINT point)
{
    const int scrollButton = HitTestTabScrollButton(tabBar, point);
    bool closeButton = false;
    const int tabIndex = scrollButton == 0 ? HitTestTabBar(tabBar, point, &closeButton) : -1;
    const int closeIndex = closeButton ? tabIndex : -1;
    const bool changed = tabIndex != g_hoveredTabIndex ||
        closeIndex != g_hoveredTabCloseIndex ||
        scrollButton != g_hoveredTabScrollButton;

    if (changed)
    {
        g_hoveredTabIndex = tabIndex;
        g_hoveredTabCloseIndex = closeIndex;
        g_hoveredTabScrollButton = scrollButton;
        InvalidateRect(tabBar, nullptr, TRUE);
    }

    ScheduleTabTooltip(tabBar, tabIndex, point);

    if (!g_trackingTabMouse)
    {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = tabBar;
        if (TrackMouseEvent(&track))
            g_trackingTabMouse = true;
    }
}

bool CloseTabAtPoint(HWND tabBar, POINT point)
{
    bool closeButton = false;
    const int tabIndex = HitTestTabBar(tabBar, point, &closeButton);
    if (tabIndex < 0 || !closeButton)
        return false;

    HideTabTooltip();
    CloseTab(tabIndex);
    UpdateTabHoverState(tabBar, point);
    return true;
}

void DrawTabSaveIcon(HDC hdc, RECT iconRect, bool modified)
{
    const COLORREF iconColor = modified ? kTabSaveRed : kTabSaveBlue;
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(26, 55, 88));
    HBRUSH bodyBrush = CreateSolidBrush(iconColor);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, bodyBrush);

    Rectangle(hdc, iconRect.left, iconRect.top, iconRect.right, iconRect.bottom);

    HBRUSH detailBrush = CreateSolidBrush(RGB(255, 255, 255));
    RECT slotRect{ iconRect.left + 3, iconRect.top + 2, iconRect.right - 3, iconRect.top + 5 };
    RECT labelRect{ iconRect.left + 4, iconRect.bottom - 5, iconRect.right - 4, iconRect.bottom - 2 };
    FillRect(hdc, &slotRect, detailBrush);
    FillRect(hdc, &labelRect, detailBrush);
    DeleteObject(detailBrush);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bodyBrush);
    DeleteObject(borderPen);
}

void DrawTabCloseButton(HDC hdc, RECT closeRect, bool hovered)
{
    if (hovered)
    {
        HBRUSH hoverBrush = CreateSolidBrush(ThemeTabCloseHoverBack());
        HPEN hoverPen = CreatePen(PS_SOLID, 1, ThemeTabCloseHoverBack());
        HGDIOBJ oldBrush = SelectObject(hdc, hoverBrush);
        HGDIOBJ oldPen = SelectObject(hdc, hoverPen);
        RoundRect(hdc, closeRect.left, closeRect.top, closeRect.right, closeRect.bottom, 4, 4);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(hoverPen);
        DeleteObject(hoverBrush);
    }

    HPEN closePen = CreatePen(PS_SOLID, hovered ? 2 : 1, hovered ? ThemeTabCloseHover() : ThemeTabClose());
    HGDIOBJ oldPen = SelectObject(hdc, closePen);

    MoveToEx(hdc, closeRect.left + 4, closeRect.top + 4, nullptr);
    LineTo(hdc, closeRect.right - 4, closeRect.bottom - 4);
    MoveToEx(hdc, closeRect.right - 5, closeRect.top + 4, nullptr);
    LineTo(hdc, closeRect.left + 3, closeRect.bottom - 4);

    SelectObject(hdc, oldPen);
    DeleteObject(closePen);
}

void FillTopRoundedRect(HDC hdc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, kTabCornerRadius * 2, kTabCornerRadius * 2);

    RECT bottomRect = rect;
    bottomRect.top = (std::max)(rect.top, rect.bottom - kTabCornerRadius);
    FillRect(hdc, &bottomRect, brush);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawTabScrollButton(HWND tabBar, HDC hdc, int direction)
{
    const RECT buttonRect = GetTabScrollButtonRect(tabBar, direction);
    const bool enabled = CanScrollTabs(tabBar, direction);
    const bool hovered = enabled && g_hoveredTabScrollButton == direction;
    const COLORREF back = hovered ? ThemeTabInactiveBack() : ThemeTabBarBack();
    const COLORREF border = ThemeTabBorder();
    const COLORREF icon = enabled ? ThemeTabText() : (IsDarkTheme() ? RGB(96, 96, 100) : RGB(166, 174, 184));

    HBRUSH backBrush = CreateSolidBrush(back);
    FillRect(hdc, &buttonRect, backBrush);
    DeleteObject(backBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    MoveToEx(hdc, buttonRect.left, buttonRect.top + 4, nullptr);
    LineTo(hdc, buttonRect.left, buttonRect.bottom - 4);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);

    HPEN iconPen = CreatePen(PS_SOLID, enabled ? 2 : 1, icon);
    oldPen = SelectObject(hdc, iconPen);
    const int centerX = (buttonRect.left + buttonRect.right) / 2;
    const int centerY = (buttonRect.top + buttonRect.bottom) / 2;
    const int half = 4;
    if (direction < 0)
    {
        MoveToEx(hdc, centerX + 2, centerY - half, nullptr);
        LineTo(hdc, centerX - 2, centerY);
        LineTo(hdc, centerX + 2, centerY + half);
    }
    else
    {
        MoveToEx(hdc, centerX - 2, centerY - half, nullptr);
        LineTo(hdc, centerX + 2, centerY);
        LineTo(hdc, centerX - 2, centerY + half);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(iconPen);
}

void DrawTabBar(HWND tabBar, HDC hdc)
{
    RECT clientRect{};
    GetClientRect(tabBar, &clientRect);
    ClampTabScrollOffset(tabBar);
    const bool overflow = HasTabOverflow(tabBar);
    const RECT viewportRect = GetTabViewportRect(tabBar);

    HBRUSH barBrush = CreateSolidBrush(ThemeTabBarBack());
    FillRect(hdc, &clientRect, barBrush);
    DeleteObject(barBrush);

    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);

    const int savedDc = SaveDC(hdc);
    IntersectClipRect(hdc, viewportRect.left, viewportRect.top, viewportRect.right, viewportRect.bottom);

    const int count = static_cast<int>(g_tabs.size());
    for (int index = 0; index < count; ++index)
    {
        const bool active = index == g_activeTabIndex;
        RECT tabRect = GetTabBarTabRect(tabBar, index);
        tabRect.bottom -= 1;
        if (tabRect.right <= viewportRect.left || tabRect.left >= viewportRect.right)
            continue;

        FillTopRoundedRect(hdc, tabRect, active ? ThemeTabActiveBack() : ThemeTabInactiveBack());

        RECT saveRect = GetTabSaveIconRect(tabRect);
        RECT closeRect = GetTabCloseButtonRect(tabRect);
        DrawTabSaveIcon(hdc, saveRect, GetTabModifiedState(index));
        DrawTabCloseButton(hdc, closeRect, index == g_hoveredTabCloseIndex);

        RECT textRect{
            saveRect.right + 6,
            tabRect.top,
            closeRect.left - 6,
            tabRect.bottom
        };
        if (textRect.right > textRect.left)
        {
            SetTextColor(hdc, ThemeTabText());
            const std::wstring title = GetTabBarDisplayTitle(g_tabs[index]);
            DrawTextW(hdc, title.c_str(), -1, &textRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        }
    }
    RestoreDC(hdc, savedDc);

    if (overflow)
    {
        DrawTabScrollButton(tabBar, hdc, -1);
        DrawTabScrollButton(tabBar, hdc, 1);
    }

    HPEN bottomPen = CreatePen(PS_SOLID, 1, ThemeTabBorder());
    HGDIOBJ oldPen = SelectObject(hdc, bottomPen);
    MoveToEx(hdc, clientRect.left, clientRect.bottom - 1, nullptr);
    LineTo(hdc, clientRect.right, clientRect.bottom - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(bottomPen);

    SelectObject(hdc, oldFont);
}

LRESULT CALLBACK TabBarWndProc(HWND tabBar, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(tabBar, &ps);
        DrawTabBar(tabBar, hdc);
        EndPaint(tabBar, &ps);
        return 0;
    }

    case WM_SIZE:
        StopTabScrollAnimation(tabBar);
        ClampTabScrollOffset(tabBar);
        if (!HasTabOverflow(tabBar))
            g_hoveredTabScrollButton = 0;
        InvalidateRect(tabBar, nullptr, TRUE);
        HideTabTooltip();
        if (g_hTabTooltip && IsWindow(g_hTabTooltip))
        {
            TOOLINFOW toolInfo{};
            toolInfo.cbSize = sizeof(toolInfo);
            toolInfo.hwnd = tabBar;
            toolInfo.uId = 1;
            GetClientRect(tabBar, &toolInfo.rect);
            SendMessageW(g_hTabTooltip, TTM_NEWTOOLRECTW, 0, reinterpret_cast<LPARAM>(&toolInfo));
        }
        return 0;

    case WM_TIMER:
        if (wParam == kTabTooltipHoverTimerId)
        {
            ShowPendingTabTooltip(tabBar);
            return 0;
        }
        if (wParam == kTabScrollAnimationTimerId)
        {
            UpdateTabScrollAnimation(tabBar);
            return 0;
        }
        return DefWindowProc(tabBar, message, wParam, lParam);

    case WM_MOUSEMOVE:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        UpdateTabHoverState(tabBar, point);
        return 0;
    }

    case WM_MOUSELEAVE:
        g_trackingTabMouse = false;
        if (g_hoveredTabIndex >= 0 || g_hoveredTabCloseIndex >= 0 || g_hoveredTabScrollButton != 0)
        {
            g_hoveredTabIndex = -1;
            g_hoveredTabCloseIndex = -1;
            g_hoveredTabScrollButton = 0;
            InvalidateRect(tabBar, nullptr, TRUE);
        }
        HideTabTooltip();
        return 0;

    case WM_MOUSEWHEEL:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(tabBar, &point);
        RECT clientRect{};
        GetClientRect(tabBar, &clientRect);
        if (!PtInRect(&clientRect, point))
            return DefWindowProc(tabBar, message, wParam, lParam);

        const int wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (wheelDelta != 0 && ScrollTabsBy(tabBar, wheelDelta > 0 ? -kTabWheelScrollPixels : kTabWheelScrollPixels))
        {
            UpdateTabHoverState(tabBar, point);
            return 0;
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HideTabTooltip();
        const int scrollButton = HitTestTabScrollButton(tabBar, point);
        if (scrollButton != 0)
        {
            if (CanScrollTabs(tabBar, scrollButton))
                AnimateTabsBy(tabBar, scrollButton * kTabWidth);
            UpdateTabHoverState(tabBar, point);
            return 0;
        }

        if (CloseTabAtPoint(tabBar, point))
            return 0;

        const int tabIndex = HitTestTabBar(tabBar, point, nullptr);
        if (tabIndex < 0)
            return 0;

        SwitchToTab(tabIndex);
        return 0;
    }

    case WM_LBUTTONDBLCLK:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HideTabTooltip();
        if (HitTestTabScrollButton(tabBar, point) != 0)
            return 0;

        if (CloseTabAtPoint(tabBar, point))
            return 0;

        const int tabIndex = HitTestTabBar(tabBar, point, nullptr);
        if (tabIndex < 0)
            NewFile();
        return 0;
    }

    case WM_RBUTTONUP:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HideTabTooltip();
        if (HitTestTabScrollButton(tabBar, point) != 0)
            return 0;

        const int tabIndex = HitTestTabBar(tabBar, point, nullptr);
        if (tabIndex < 0)
            return 0;

        g_contextTabIndex = tabIndex;
        POINT screenPoint = point;
        ClientToScreen(tabBar, &screenPoint);

        HMENU menu = CreatePopupMenu();
        if (menu)
        {
            AppendMenuW(menu, MF_STRING, IDM_TAB_CLOSE,
                MenuLabelWithShortcut(L"\u5173\u95ED\u6807\u7B7E", L"Close Tab", IDM_TAB_CLOSE).c_str());
            AppendMenuW(menu, MF_STRING, IDM_TAB_CLOSE_OTHERS,
                UiText(L"\u5173\u95ED\u5176\u4ED6\u6807\u7B7E", L"Close Other Tabs"));
            AppendMenuW(menu, MF_STRING, IDM_TAB_CLOSE_ALL,
                UiText(L"\u5173\u95ED\u6240\u6709\u6807\u7B7E", L"Close All Tabs"));
            std::vector<std::unique_ptr<ThemedMenuItem>> menuItems;
            ApplyPopupMenuTheme(menu, menuItems);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hWnd, nullptr);
            g_contextTabIndex = -1;
            DestroyMenu(menu);
        }
        return 0;
    }

    default:
        return DefWindowProc(tabBar, message, wParam, lParam);
    }
}

DocumentEncoding GetActiveDocumentEncoding()
{
    if (!IsActiveTabValid())
        return DocumentEncoding::Utf8;
    return g_tabs[g_activeTabIndex].encoding;
}

int GetActiveDocumentEolMode()
{
    if (g_hSci)
        return static_cast<int>(Sci(SCI_GETEOLMODE));
    if (IsActiveTabValid())
        return g_tabs[g_activeTabIndex].eolMode;
    return SC_EOL_CRLF;
}

sptr_t CountCharactersInRange(sptr_t start, sptr_t end)
{
    start = (std::max)(start, static_cast<sptr_t>(0));
    end = (std::max)(end, static_cast<sptr_t>(0));
    if (!g_hSci || end <= start)
        return 0;

    const sptr_t count = Sci(SCI_COUNTCHARACTERS, static_cast<uptr_t>(start), end);
    return count >= 0 ? count : end - start;
}

std::wstring BuildStatusSummary()
{
    if (!g_hSci)
        return UiText(L"\u957F\u5EA6: 0    \u884C\u6570: 0    \u884C 1, \u5217 1, \u4F4D\u7F6E 0",
            L"Length: 0    Lines: 0    Ln 1, Col 1, Pos 0");

    const sptr_t byteLength = Sci(SCI_GETLENGTH);
    const sptr_t lineCount = Sci(SCI_GETLINECOUNT);
    const sptr_t position = Sci(SCI_GETCURRENTPOS);
    const sptr_t line = Sci(SCI_LINEFROMPOSITION, static_cast<uptr_t>(position), 0);
    const sptr_t lineStart = Sci(SCI_POSITIONFROMLINE, static_cast<uptr_t>(line), 0);
    const sptr_t length = CountCharactersInRange(0, byteLength);
    const sptr_t column = CountCharactersInRange(lineStart, position);
    const sptr_t characterPosition = CountCharactersInRange(0, position);

    if (g_appLanguage == AppLanguage::Chinese)
    {
        return std::wstring(L"\u957F\u5EA6: ") + std::to_wstring(length) +
            L"    \u884C\u6570: " + std::to_wstring(lineCount) +
            L"    \u884C " + std::to_wstring(line + 1) +
            L", \u5217 " + std::to_wstring(column + 1) +
            L", \u4F4D\u7F6E " + std::to_wstring(characterPosition);
    }

    return std::wstring(L"Length: ") + std::to_wstring(length) +
        L"    Lines: " + std::to_wstring(lineCount) +
        L"    Ln " + std::to_wstring(line + 1) +
        L", Col " + std::to_wstring(column + 1) +
        L", Pos " + std::to_wstring(characterPosition);
}

RECT GetStatusEolRect(HWND statusBar)
{
    RECT clientRect{};
    GetClientRect(statusBar, &clientRect);
    return RECT{ clientRect.right - 150, clientRect.top, clientRect.right - 8, clientRect.bottom };
}

RECT GetStatusFolderToggleRect(HWND statusBar)
{
    RECT clientRect{};
    GetClientRect(statusBar, &clientRect);
    return RECT{ clientRect.left, clientRect.top, clientRect.left + kStatusFolderToggleWidth, clientRect.bottom };
}

RECT GetStatusEncodingRect(HWND statusBar)
{
    RECT eolRect = GetStatusEolRect(statusBar);
    return RECT{ eolRect.left - 122, eolRect.top, eolRect.left - 8, eolRect.bottom };
}

StatusHitArea HitTestStatusBar(HWND statusBar, POINT point)
{
    RECT folderToggleRect = GetStatusFolderToggleRect(statusBar);
    if (PtInRect(&folderToggleRect, point))
        return StatusHitArea::FolderToggle;

    RECT encodingRect = GetStatusEncodingRect(statusBar);
    if (PtInRect(&encodingRect, point))
        return StatusHitArea::Encoding;

    RECT eolRect = GetStatusEolRect(statusBar);
    if (PtInRect(&eolRect, point))
        return StatusHitArea::EolFormat;

    return StatusHitArea::None;
}

void DrawStatusItem(HDC hdc, const RECT& itemRect, const std::wstring& text)
{
    RECT textRect = itemRect;
    textRect.left += 8;
    textRect.right -= 8;
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
}

void DrawStatusSeparator(HDC hdc, int x, int top, int bottom)
{
    HPEN pen = CreatePen(PS_SOLID, 1, ThemeStatusLine());
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, x, top + 5, nullptr);
    LineTo(hdc, x, bottom - 5);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawStatusFolderToggleIcon(HDC hdc, const RECT& buttonRect)
{
    DrawStatusSeparator(hdc, buttonRect.right, buttonRect.top, buttonRect.bottom);

    const int iconLeft = buttonRect.left + ((buttonRect.right - buttonRect.left) - kStatusFolderToggleIconWidth) / 2;
    const int iconTop = buttonRect.top + ((buttonRect.bottom - buttonRect.top) - kStatusFolderToggleIconHeight) / 2;
    RECT iconRect{
        iconLeft,
        iconTop,
        iconLeft + kStatusFolderToggleIconWidth,
        iconTop + kStatusFolderToggleIconHeight
    };
    RECT sidebarRect{ iconRect.left + 1, iconRect.top + 1, iconRect.left + 6, iconRect.bottom - 1 };

    HBRUSH iconBrush = CreateSolidBrush(g_folderPaneVisible ? ThemeAccent() : ThemeStatusBack());
    HPEN iconPen = CreatePen(PS_SOLID, 1, g_folderPaneVisible ? ThemeAccent() : ThemeStatusText());
    HGDIOBJ oldBrush = SelectObject(hdc, iconBrush);
    HGDIOBJ oldPen = SelectObject(hdc, iconPen);
    RoundRect(hdc, iconRect.left, iconRect.top, iconRect.right, iconRect.bottom, 3, 3);
    Rectangle(hdc, sidebarRect.left, sidebarRect.top, sidebarRect.right, sidebarRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(iconPen);
    DeleteObject(iconBrush);

    HPEN dividerPen = CreatePen(PS_SOLID, 1, g_folderPaneVisible ? RGB(255, 255, 255) : ThemeStatusLine());
    oldPen = SelectObject(hdc, dividerPen);
    MoveToEx(hdc, sidebarRect.right + 1, iconRect.top + 2, nullptr);
    LineTo(hdc, sidebarRect.right + 1, iconRect.bottom - 2);
    SelectObject(hdc, oldPen);
    DeleteObject(dividerPen);
}

void DrawStatusBar(HWND statusBar, HDC hdc)
{
    RECT clientRect{};
    GetClientRect(statusBar, &clientRect);

    HBRUSH background = CreateSolidBrush(ThemeStatusBack());
    FillRect(hdc, &clientRect, background);
    DeleteObject(background);

    HPEN topPen = CreatePen(PS_SOLID, 1, ThemeStatusLine());
    HGDIOBJ oldPen = SelectObject(hdc, topPen);
    MoveToEx(hdc, clientRect.left, clientRect.top, nullptr);
    LineTo(hdc, clientRect.right, clientRect.top);
    SelectObject(hdc, oldPen);
    DeleteObject(topPen);

    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ThemeStatusText());

    RECT eolRect = GetStatusEolRect(statusBar);
    RECT encodingRect = GetStatusEncodingRect(statusBar);
    RECT folderToggleRect = GetStatusFolderToggleRect(statusBar);
    RECT summaryRect{ folderToggleRect.right + 8, clientRect.top, encodingRect.left - 8, clientRect.bottom };

    DrawStatusFolderToggleIcon(hdc, folderToggleRect);
    const std::wstring summary = BuildStatusSummary();
    DrawTextW(hdc, summary.c_str(), -1, &summaryRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    DrawStatusSeparator(hdc, encodingRect.left, clientRect.top, clientRect.bottom);
    DrawStatusItem(hdc, encodingRect, EncodingDisplayName(GetActiveDocumentEncoding()));

    DrawStatusSeparator(hdc, eolRect.left, clientRect.top, clientRect.bottom);
    DrawStatusItem(hdc, eolRect, EolDisplayName(GetActiveDocumentEolMode()));

    SelectObject(hdc, oldFont);
}

void ShowEncodingMenu(HWND statusBar, POINT point)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, IDM_ENCODING_UTF8, L"UTF-8");
    AppendMenuW(menu, MF_STRING, IDM_ENCODING_UTF8_BOM, L"UTF-8 BOM");
    AppendMenuW(menu, MF_STRING, IDM_ENCODING_ANSI, L"ANSI");
    AppendMenuW(menu, MF_STRING, IDM_ENCODING_UTF16_LE, L"UTF-16 LE");
    AppendMenuW(menu, MF_STRING, IDM_ENCODING_UTF16_BE, L"UTF-16 BE");
    CheckMenuRadioItem(menu, IDM_ENCODING_UTF8, IDM_ENCODING_UTF16_BE,
        CommandFromEncoding(GetActiveDocumentEncoding()), MF_BYCOMMAND);

    ClientToScreen(statusBar, &point);
    std::vector<std::unique_ptr<ThemedMenuItem>> menuItems;
    ApplyPopupMenuTheme(menu, menuItems);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, nullptr);
    DestroyMenu(menu);
}

void ShowEolMenu(HWND statusBar, POINT point)
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, IDM_EOL_WINDOWS, L"Windows (CRLF)");
    AppendMenuW(menu, MF_STRING, IDM_EOL_UNIX, L"Unix (LF)");
    AppendMenuW(menu, MF_STRING, IDM_EOL_MAC, L"macOS (CR)");
    CheckMenuRadioItem(menu, IDM_EOL_WINDOWS, IDM_EOL_MAC,
        CommandFromEolMode(GetActiveDocumentEolMode()), MF_BYCOMMAND);

    ClientToScreen(statusBar, &point);
    std::vector<std::unique_ptr<ThemedMenuItem>> menuItems;
    ApplyPopupMenuTheme(menu, menuItems);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hWnd, nullptr);
    DestroyMenu(menu);
}

LRESULT CALLBACK StatusBarWndProc(HWND statusBar, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(statusBar, &ps);
        DrawStatusBar(statusBar, hdc);
        EndPaint(statusBar, &ps);
        return 0;
    }

    case WM_SIZE:
        InvalidateRect(statusBar, nullptr, TRUE);
        return 0;

    case WM_RBUTTONUP:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        const StatusHitArea hitArea = HitTestStatusBar(statusBar, point);
        if (hitArea == StatusHitArea::FolderToggle)
        {
            return 0;
        }
        else if (hitArea == StatusHitArea::Encoding)
        {
            ShowEncodingMenu(statusBar, point);
        }
        else if (hitArea == StatusHitArea::EolFormat)
        {
            ShowEolMenu(statusBar, point);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (HitTestStatusBar(statusBar, point) == StatusHitArea::FolderToggle)
        {
            ToggleFolderPane();
            return 0;
        }
        return 0;
    }

    default:
        return DefWindowProc(statusBar, message, wParam, lParam);
    }
}

// 关于对话框
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

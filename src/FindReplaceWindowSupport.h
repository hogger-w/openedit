#pragma once

#include <windows.h>

#include <string>

namespace FindReplaceSupport
{
inline constexpr wchar_t kClassName[] = L"OpenEditFindReplaceWindow";
inline constexpr wchar_t kSliderClassName[] = L"OpenEditOpacitySlider";
inline constexpr UINT kFindWindowGetDarkThemeMessage = WM_APP + 91;
inline constexpr int kWindowWidth = 660;
inline constexpr int kClientHeight = 254;
inline constexpr int kEditInsetX = 10;
inline constexpr int kEditInsetY = 5;
inline constexpr int kOpacityMaxPercent = 70;

inline constexpr int IDC_FIND_TEXT = 2102;
inline constexpr int IDC_REPLACE_TEXT = 2103;
inline constexpr int IDC_FIND_NEXT = 2104;
inline constexpr int IDC_FIND_PREVIOUS = 2105;
inline constexpr int IDC_COUNT = 2106;
inline constexpr int IDC_MARK = 2125;
inline constexpr int IDC_REPLACE = 2107;
inline constexpr int IDC_REPLACE_ALL = 2108;
inline constexpr int IDC_REVERSE = 2109;
inline constexpr int IDC_MATCH_CASE = 2110;
inline constexpr int IDC_STATUS = 2112;
inline constexpr int IDC_WRAP = 2116;
inline constexpr int IDC_MODE_LABEL = 2117;
inline constexpr int IDC_MODE_NORMAL = 2118;
inline constexpr int IDC_MODE_REGEX = 2119;
inline constexpr int IDC_OPACITY_ENABLED = 2120;
inline constexpr int IDC_OPACITY_ON_BLUR = 2121;
inline constexpr int IDC_OPACITY_ALWAYS = 2122;
inline constexpr int IDC_OPACITY_SLIDER = 2123;
inline constexpr int IDC_OPACITY_VALUE = 2124;

COLORREF PanelBack(bool dark);
COLORREF PanelSurface(bool dark);
COLORREF PanelText(bool dark);
COLORREF MutedText(bool dark);
COLORREF EditBack(bool dark);
COLORREF Border(bool dark);
COLORREF Accent();
COLORREF ButtonBack(bool dark, bool pressed);
COLORREF ButtonBorder(bool dark);
COLORREF SliderTrack(bool dark);
COLORREF SliderThumb(bool dark);

bool IsPanelControl(int controlId);
bool IsCheckBoxControl(int controlId);
bool IsRadioControl(int controlId);
const wchar_t* Text(bool chinese, const wchar_t* zh, const wchar_t* en);
void SetDefaultFont(HWND control);
std::wstring GetControlText(HWND parent, int id);
void DrawRoundedRect(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius);
void DrawTextInRect(HDC hdc, const RECT& rect, const std::wstring& text, COLORREF color, UINT format);
void DrawOptionControl(const DRAWITEMSTRUCT* drawItem, bool dark, bool radio, bool checked);
bool EnsureSliderClass(HINSTANCE instance);
void ApplyWindowDarkMode(HWND window, bool darkTheme);
}

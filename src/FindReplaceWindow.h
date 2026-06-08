#pragma once

#include <windows.h>

#include <string>

struct OpenEditFindRequest
{
    std::wstring findText;
    std::wstring replaceText;
    bool matchCase = false;
    bool reverse = false;
    bool regex = false;
    bool wrap = true;
};

class OpenEditFindWindow
{
public:
    struct Callbacks
    {
        void* context = nullptr;
        bool (*find)(void* context, const OpenEditFindRequest& request, bool previous) = nullptr;
        int (*count)(void* context, const OpenEditFindRequest& request) = nullptr;
        int (*mark)(void* context, const OpenEditFindRequest& request) = nullptr;
        bool (*replace)(void* context, const OpenEditFindRequest& request) = nullptr;
        int (*replaceAll)(void* context, const OpenEditFindRequest& request) = nullptr;
    };

    explicit OpenEditFindWindow(HINSTANCE instance);
    ~OpenEditFindWindow();

    bool Show(HWND owner, bool replaceVisible, const std::wstring& findText,
        const std::wstring& replaceText, bool darkTheme, bool chineseLanguage,
        const Callbacks& callbacks);
    void Hide();
    void Destroy();
    bool TranslateDialogMessage(MSG& message);
    void UpdateThemeAndLanguage(bool darkTheme, bool chineseLanguage);
    HWND Window() const { return window_; }
    bool IsVisible() const;

    std::wstring FindText() const;
    std::wstring ReplaceText() const;

private:
    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    bool EnsureWindow(HWND owner);
    void CreateControls();
    HWND CreateControl(const wchar_t* className, const wchar_t* text, DWORD style, DWORD exStyle, int id);
    void LayoutControls();
    void PositionWindow();
    void SaveWindowPosition();
    void UpdateTexts();
    void UpdateBrushes();
    void RedrawThemedControls();
    void RedrawControl(int id);
    bool IsOptionChecked(int id) const;
    void SetOptionChecked(int id, bool checked);
    void ToggleCheckBox(int id);
    void SelectRadio(int firstId, int lastId, int selectedId);
    void UpdateOpacityControls();
    void ApplyOpacity(bool active);
    void SetStatus(const std::wstring& text);
    OpenEditFindRequest BuildRequest() const;
    void FocusFindText();
    void ExecuteFind(bool previous);
    void ExecuteCount();
    void ExecuteMark();
    void ExecuteReplace();
    void ExecuteReplaceAll();

    HINSTANCE instance_ = nullptr;
    HWND owner_ = nullptr;
    HWND window_ = nullptr;
    Callbacks callbacks_{};
    bool darkTheme_ = false;
    bool chineseLanguage_ = true;
    HBRUSH backgroundBrush_ = nullptr;
    HBRUSH surfaceBrush_ = nullptr;
    HBRUSH editBrush_ = nullptr;
    std::wstring statusText_;
    RECT findEditFrame_{};
    RECT replaceEditFrame_{};
    bool active_ = true;
    bool reverse_ = false;
    bool matchCase_ = false;
    bool wrap_ = true;
    bool regex_ = false;
    bool opacityEnabled_ = false;
    bool opacityAlways_ = false;
    POINT lastPosition_{};
    bool hasLastPosition_ = false;
};

#pragma once

#include "framework.h"
#include "openedit.h"
#include "FindReplaceWindow.h"

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <shobjidl.h>
#include "Scintilla.h"
#include "SciLexer.h"
#include "ILexer.h"
#include "Lexilla.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#define MAX_LOADSTRING 100

#ifndef TVS_NOHSCROLL
#define TVS_NOHSCROLL 0x8000
#endif

#ifndef TVS_EX_DOUBLEBUFFER
#define TVS_EX_DOUBLEBUFFER 0x0004
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

inline constexpr int kFolderPaneMinWidth = 160;
inline constexpr int kFolderPaneMaxWidth = 520;
inline constexpr int kFolderPaneDividerWidth = 5;
inline constexpr int kFolderPaneOpenFilesDefaultHeight = 100;
inline constexpr int kFolderPaneOpenFilesMaxHeight = 200;
inline constexpr int kFolderPaneSectionMinHeight = 72;
inline constexpr int kFolderPaneSectionDividerHeight = 5;
inline constexpr int kFolderPaneHeaderHeight = 26;
inline constexpr int kFolderPaneHeaderTextInset = 10;
inline constexpr int kEditorMinWidth = 160;
inline constexpr int kTabBarHeight = 26;
inline constexpr int kStatusBarHeight = 24;
inline constexpr int kStatusFolderToggleWidth = 34;
inline constexpr int kStatusFolderToggleIconWidth = 17;
inline constexpr int kStatusFolderToggleIconHeight = 14;
inline constexpr int kTabWidth = 100;
inline constexpr int kTabSaveIconSize = 12;
inline constexpr int kTabCloseSize = 14;
inline constexpr int kTabCornerRadius = 7;
inline constexpr int kDefaultMainWindowWidth = 800;
inline constexpr int kDefaultMainWindowHeight = 600;
inline constexpr int kMinRestoredMainWindowWidth = 200;
inline constexpr int kMinRestoredMainWindowHeight = 160;
inline constexpr int kLineNumberMargin = 0;
inline constexpr int kFoldMargin = 1;
inline constexpr int kViewMenuIndex = 3;
inline constexpr int kLanguageMenuIndex = 4;
inline constexpr int kEditorFontSize = 14;
inline constexpr int kWordHighlightIndicator = 31;
inline constexpr DWORD kFindOptionRegex = 0x10000000;
inline constexpr DWORD kFindOptionMask = FR_DOWN | FR_MATCHCASE | FR_WHOLEWORD | kFindOptionRegex;
inline constexpr WORD kSearchTextBufferLength = 256;
inline constexpr BYTE kShortcutCtrl = 0x01;
inline constexpr BYTE kShortcutShift = 0x02;
inline constexpr BYTE kShortcutAlt = 0x04;
inline constexpr int HTFOLDERTOGGLE = 100;
inline constexpr int HTFOLDERSECTIONDIVIDER = 101;
inline constexpr COLORREF kFolderPaneBackColor = RGB(236, 242, 248);
inline constexpr COLORREF kFolderPaneTextColor = RGB(32, 45, 58);
inline constexpr COLORREF kFolderPaneLineColor = RGB(188, 202, 216);
inline constexpr COLORREF kTabBarBackColor = RGB(238, 241, 245);
inline constexpr COLORREF kTabActiveBackColor = RGB(255, 255, 255);
inline constexpr COLORREF kTabInactiveBackColor = RGB(226, 232, 238);
inline constexpr COLORREF kTabBorderColor = RGB(190, 198, 208);
inline constexpr COLORREF kTabTextColor = RGB(35, 45, 58);
inline constexpr COLORREF kTabSaveBlue = RGB(35, 111, 195);
inline constexpr COLORREF kTabSaveRed = RGB(214, 54, 54);
inline constexpr COLORREF kVsCodeEditorBack = RGB(30, 30, 30);
inline constexpr COLORREF kVsCodeCurrentLineBack = RGB(42, 45, 46);
inline constexpr COLORREF kVsCodeText = RGB(212, 212, 212);
inline constexpr COLORREF kVsCodeLineNumber = RGB(133, 133, 133);
inline constexpr COLORREF kVsCodeInvisible = RGB(64, 64, 64);
inline constexpr COLORREF kVsCodeSelectionBack = RGB(38, 79, 120);
inline constexpr COLORREF kVsCodeCaret = RGB(174, 175, 173);
inline constexpr COLORREF kVsCodeComment = RGB(106, 153, 85);
inline constexpr COLORREF kVsCodeKeyword = RGB(86, 156, 214);
inline constexpr COLORREF kVsCodeControlKeyword = RGB(197, 134, 192);
inline constexpr COLORREF kVsCodeType = RGB(78, 201, 176);
inline constexpr COLORREF kVsCodeFunction = RGB(220, 220, 170);
inline constexpr COLORREF kVsCodeVariable = RGB(156, 220, 254);
inline constexpr COLORREF kVsCodeString = RGB(206, 145, 120);
inline constexpr COLORREF kVsCodeNumber = RGB(181, 206, 168);
inline constexpr COLORREF kVsCodeOperator = RGB(212, 212, 212);
inline constexpr COLORREF kVsCodeTag = RGB(86, 156, 214);
inline constexpr COLORREF kVsCodeCssSelector = RGB(215, 186, 125);
inline constexpr COLORREF kVsCodeLink = RGB(78, 190, 255);
inline constexpr COLORREF kVsCodeEscape = RGB(209, 105, 105);
inline constexpr COLORREF kVsCodeError = RGB(244, 71, 71);
inline constexpr COLORREF kVsCodeMarkdownCodeBack = RGB(37, 37, 38);
inline constexpr COLORREF kVsCodeLightEditorBack = RGB(255, 255, 255);
inline constexpr COLORREF kVsCodeLightCurrentLineBack = RGB(245, 245, 245);
inline constexpr COLORREF kVsCodeLightText = RGB(0, 0, 0);
inline constexpr COLORREF kVsCodeLightLineNumber = RGB(35, 92, 147);
inline constexpr COLORREF kVsCodeLightInvisible = RGB(191, 191, 191);
inline constexpr COLORREF kVsCodeLightSelectionBack = RGB(173, 214, 255);
inline constexpr COLORREF kVsCodeLightCaret = RGB(0, 0, 0);
inline constexpr COLORREF kVsCodeLightComment = RGB(0, 128, 0);
inline constexpr COLORREF kVsCodeLightKeyword = RGB(0, 0, 255);
inline constexpr COLORREF kVsCodeLightControlKeyword = RGB(175, 0, 219);
inline constexpr COLORREF kVsCodeLightType = RGB(38, 127, 153);
inline constexpr COLORREF kVsCodeLightFunction = RGB(121, 94, 38);
inline constexpr COLORREF kVsCodeLightVariable = RGB(0, 16, 128);
inline constexpr COLORREF kVsCodeLightString = RGB(163, 21, 21);
inline constexpr COLORREF kVsCodeLightNumber = RGB(9, 134, 88);
inline constexpr COLORREF kVsCodeLightOperator = RGB(0, 0, 0);
inline constexpr COLORREF kVsCodeLightTag = RGB(128, 0, 0);
inline constexpr COLORREF kVsCodeLightCssSelector = RGB(128, 0, 0);
inline constexpr COLORREF kVsCodeLightLink = RGB(0, 0, 255);
inline constexpr COLORREF kVsCodeLightEscape = RGB(129, 31, 63);
inline constexpr COLORREF kVsCodeLightError = RGB(220, 0, 0);
inline constexpr COLORREF kVsCodeLightMarkdownCodeBack = RGB(247, 247, 247);
inline constexpr int IDC_SETTINGS_THEME_LIGHT = 1004;
inline constexpr int IDC_SETTINGS_THEME_DARK = 1005;
inline constexpr int IDC_SETTINGS_LANGUAGE_CHINESE = 1006;
inline constexpr int IDC_SETTINGS_LANGUAGE_ENGLISH = 1007;
inline constexpr int IDC_SETTINGS_RESTORE_PREVIOUS_FILES = 1008;
inline constexpr int IDC_SETTINGS_TAB_GENERAL = 1009;
inline constexpr int IDC_SETTINGS_TAB_SHORTCUTS = 1010;
inline constexpr int IDC_SETTINGS_GENERAL_THEME_LABEL = 1011;
inline constexpr int IDC_SETTINGS_GENERAL_LANGUAGE_LABEL = 1012;
inline constexpr int IDC_SETTINGS_GENERAL_STARTUP_LABEL = 1013;
inline constexpr int IDC_SETTINGS_SHORTCUT_RESET = 1014;
inline constexpr int IDC_SETTINGS_SHORTCUT_LABEL_BASE = 1100;
inline constexpr int IDC_SETTINGS_SHORTCUT_HOTKEY_BASE = 1200;
inline constexpr int IDC_COLUMN_MODE_TEXT = 1400;
inline constexpr int IDC_COLUMN_MODE_NUMBER = 1401;
inline constexpr int IDC_COLUMN_TEXT_VALUE = 1402;
inline constexpr int IDC_COLUMN_INITIAL_VALUE = 1403;
inline constexpr int IDC_COLUMN_INCREMENT_VALUE = 1404;
inline constexpr int IDC_COLUMN_REPEAT_VALUE = 1405;
inline constexpr int IDC_COLUMN_PADDING_VALUE = 1406;
inline constexpr int kFolderSplitterPreviewWindowWidth = 5;
inline constexpr COLORREF kFolderSplitterPreviewBackColor = RGB(255, 0, 255);
inline constexpr COLORREF kFolderSplitterPreviewLineColor = RGB(180, 180, 180);
inline constexpr COLORREF kFolderTreeIconMaskColor = RGB(255, 0, 255);
inline constexpr sptr_t kColumnEditMaxDocumentBytes = 200LL * 1024LL * 1024LL;
inline constexpr sptr_t kColumnEditMaxAffectedLines = 100000;
inline constexpr sptr_t kColumnEditMaxLineBytes = 2LL * 1024LL * 1024LL;
inline constexpr sptr_t kColumnEditMaxVisualColumn = 200000;
inline constexpr size_t kColumnEditMaxInsertBytes = 64ULL * 1024ULL * 1024ULL;

inline constexpr wchar_t kTabBarClassName[] = L"OpenEditTabBar";
inline constexpr wchar_t kStatusBarClassName[] = L"OpenEditStatusBar";
inline constexpr wchar_t kSettingsWindowClassName[] = L"OpenEditSettingsWindow";
inline constexpr wchar_t kAboutWindowClassName[] = L"OpenEditAboutWindow";
inline constexpr wchar_t kFolderSplitterPreviewClassName[] = L"OpenEditFolderSplitterPreview";
inline constexpr wchar_t kColumnEditorWindowClassName[] = L"OpenEditColumnEditorWindow";
inline constexpr wchar_t kAppDisplayName[] = L"openedit";
inline constexpr wchar_t kAppUserModelId[] = L"openedit";

inline constexpr size_t kShortcutBindingCount = 17;

enum class AppTheme
{
    Light,
    Dark,
};

enum class AppLanguage
{
    Chinese,
    English,
};

struct MainWindowPlacement
{
    bool hasPlacement = false;
    RECT normalRect{ 0, 0, kDefaultMainWindowWidth, kDefaultMainWindowHeight };
    bool maximized = false;
};

enum class ColumnEditorMode
{
    Text,
    Number,
};

enum class ColumnPaddingMode
{
    None,
    Zero,
    Space,
};


struct ThemedMenuItem
{
    std::wstring text;
    bool topLevel = false;
    bool hasSubMenu = false;
    bool separator = false;
};

struct ShortcutBinding
{
    int commandId;
    WORD key;
    BYTE modifiers;
    WORD defaultKey;
    BYTE defaultModifiers;
};


enum class DocumentEncoding
{
    Utf8,
    Utf8Bom,
    Ansi,
    Utf16LE,
    Utf16BE,
};

enum class StatusHitArea
{
    None,
    FolderToggle,
    Encoding,
    EolFormat,
};

struct FolderItem
{
    std::wstring name;
    std::wstring path;
    bool isDirectory = false;
    bool childrenLoaded = false;
};

struct OpenFileItem
{
    int tabIndex = -1;
    std::wstring title;
};


enum class SyntaxFamily
{
    Plain,
    CppLike,
    Python,
    HyperText,
    Css,
    Json,
    Sql,
    Bash,
    PowerShell,
    Rust,
    Lua,
    Ruby,
    Markdown,
    Yaml,
    Toml,
    Properties,
    Makefile,
    Diff,
    Batch,
    Zig,
    Nim,
    Registry,
    Inno,
};

struct LanguageDefinition
{
    int commandId;
    const char* lexerName;
    SyntaxFamily family;
    std::array<const char*, 8> keywords;
};


struct LanguageMenuLabel
{
    int commandId;
    const wchar_t* chinese;
    const wchar_t* english;
};

struct LanguageMenuGroup
{
    const wchar_t* title;
    wchar_t firstLetter;
    wchar_t lastLetter;
};


struct ColumnEditSelection
{
    sptr_t firstLine = 0;
    sptr_t lastLine = 0;
    sptr_t insertColumn = 0;
    sptr_t lineCount = 0;
};

struct ColumnEditRequest
{
    ColumnEditorMode mode = ColumnEditorMode::Text;
    ColumnPaddingMode padding = ColumnPaddingMode::None;
    std::wstring text;
    long long initialValue = 1;
    long long increment = 1;
    int repeatCount = 1;
    size_t minimumNumberWidth = 0;
};

struct ColumnInsertPoint
{
    sptr_t position = 0;
    sptr_t spacesBeforeValue = 0;
};

struct ColumnEditInsertion
{
    sptr_t line = 0;
    sptr_t position = 0;
    size_t spacesBeforeValue = 0;
    size_t valueBytes = 0;
    std::string text;
};




struct DocumentTab
{
    std::wstring title;
    std::wstring path;
    std::wstring sessionTempPath;
    std::string text;
    std::string savedText;
    int languageCommand = IDM_LANG_TEXT;
    DocumentEncoding encoding = DocumentEncoding::Utf8;
    int eolMode = SC_EOL_CRLF;
    sptr_t caretPosition = 0;
    sptr_t anchorPosition = 0;
    sptr_t firstVisibleLine = 0;
    sptr_t xOffset = 0;
    bool modified = false;
    bool untitled = true;
    bool openedFromFolder = false;
};

class ScopedRedrawPause
{
public:
    explicit ScopedRedrawPause(HWND window) : window_(window)
    {
        if (!window_)
            return;

        auto& depths = PauseDepths();
        auto entry = std::find_if(depths.begin(), depths.end(), [this](const auto& candidate) {
            return candidate.first == window_;
        });
        if (entry == depths.end())
        {
            depths.push_back({ window_, 1 });
            SendMessageW(window_, WM_SETREDRAW, FALSE, 0);
        }
        else
        {
            ++entry->second;
        }
    }

    ~ScopedRedrawPause()
    {
        if (!window_)
            return;

        auto& depths = PauseDepths();
        auto entry = std::find_if(depths.begin(), depths.end(), [this](const auto& candidate) {
            return candidate.first == window_;
        });
        if (entry == depths.end())
            return;

        --entry->second;
        if (entry->second > 0)
            return;

        depths.erase(entry);
        SendMessageW(window_, WM_SETREDRAW, TRUE, 0);
        RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
    }

    ScopedRedrawPause(const ScopedRedrawPause&) = delete;
    ScopedRedrawPause& operator=(const ScopedRedrawPause&) = delete;

private:
    static std::vector<std::pair<HWND, int>>& PauseDepths()
    {
        static std::vector<std::pair<HWND, int>> depths;
        return depths;
    }

    HWND window_ = nullptr;
};

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];
extern HWND hWnd;
extern HWND g_hSci;
extern HWND g_hOpenFilesTree;
extern HWND g_hFolderTree;
extern HWND g_hTabBar;
extern HWND g_hStatusBar;
extern HWND g_hTabTooltip;
extern HWND g_hFindReplaceDialog;
extern HWND g_hSettingsWindow;
extern HWND g_hAboutWindow;
extern HWND g_hFolderSplitterPreview;
extern HWND g_hColumnEditorWindow;
extern HACCEL g_hAccelTable;
extern HMENU g_hSettingsMenu;
extern HIMAGELIST g_hFolderTreeImageList;
extern WNDPROC g_originalFolderTreeProc;
extern WNDPROC g_originalOpenFilesTreeProc;
extern WNDPROC g_originalEditorProc;
extern std::unique_ptr<OpenEditFindWindow> g_findWindow;
extern HBRUSH g_hPopupBackBrush;
extern HBRUSH g_hPopupSurfaceBrush;
extern HBRUSH g_hPopupInputBrush;
extern HBRUSH g_hMenuBackBrush;
extern SciFnDirect g_pSciFn;
extern sptr_t g_pSciPtr;
extern UINT g_findReplaceMessage;
extern int g_currentLanguageCommand;
extern AppTheme g_appTheme;
extern AppLanguage g_appLanguage;
extern bool g_folderPaneVisible;
extern bool g_draggingFolderSplitter;
extern bool g_draggingFolderSectionSplitter;
extern bool g_applyingFolderTreeTheme;
extern int g_folderPaneWidth;
extern int g_folderSplitterPreviewWidth;
extern int g_openFilesPaneHeight;
extern bool g_restorePreviousFilesOnStartup;
extern bool g_restoreFolderInSession;
extern MainWindowPlacement g_mainWindowPlacement;
extern AppTheme g_settingsDraftTheme;
extern AppLanguage g_settingsDraftLanguage;
extern ColumnEditorMode g_columnEditorMode;
extern bool g_settingsDraftRestorePreviousFiles;
extern int g_settingsActiveTab;
extern bool g_showSpaceAndTab;
extern bool g_showEndOfLine;
extern bool g_wordWrapEnabled;
extern std::wstring g_currentFilePath;
extern std::wstring g_currentFolderPath;
extern std::wstring g_tabTooltipText;
extern int g_hoveredTabIndex;
extern int g_hoveredTabCloseIndex;
extern bool g_trackingTabMouse;
extern FINDREPLACEW g_findReplace;
extern wchar_t g_findText[kSearchTextBufferLength];
extern wchar_t g_replaceText[kSearchTextBufferLength];
extern DWORD g_lastFindOptions;
extern std::string g_highlightedWord;
extern std::vector<std::unique_ptr<ThemedMenuItem>> g_mainMenuItems;
extern std::array<ShortcutBinding, kShortcutBindingCount> g_shortcutBindings;
extern std::array<ShortcutBinding, kShortcutBindingCount> g_settingsDraftShortcuts;
extern std::vector<std::unique_ptr<FolderItem>> g_folderItems;
extern std::vector<std::unique_ptr<OpenFileItem>> g_openFileItems;
extern int g_folderIconIndex;
extern int g_folderOpenIconIndex;
extern int g_textFileIconIndex;
extern std::vector<DocumentTab> g_tabs;
extern int g_activeTabIndex;
extern int g_contextTabIndex;
extern int g_nextUntitledIndex;
extern bool g_loadingTabContent;

sptr_t Sci(unsigned int message, uptr_t wParam = 0, sptr_t lParam = 0);
bool IsDarkTheme();
const wchar_t* UiText(const wchar_t* chinese, const wchar_t* english);
ShortcutBinding* FindShortcutBinding(int commandId);
BYTE AcceleratorFlagsFromShortcut(BYTE modifiers);
WORD HotKeyControlModifiers(BYTE modifiers);
BYTE ShortcutModifiersFromHotKey(WORD hotKey);
void RebuildAcceleratorTable();
COLORREF ThemeEditorBack();
COLORREF ThemeCurrentLineBack();
COLORREF ThemeEditorText();
COLORREF ThemeLineNumber();
COLORREF ThemeInvisible();
COLORREF ThemeSelectionBack();
COLORREF ThemeCaret();
COLORREF ThemeComment();
COLORREF ThemeKeyword();
COLORREF ThemeControlKeyword();
COLORREF ThemeType();
COLORREF ThemeFunction();
COLORREF ThemeVariable();
COLORREF ThemeString();
COLORREF ThemeNumber();
COLORREF ThemeOperator();
COLORREF ThemeTag();
COLORREF ThemeCssSelector();
COLORREF ThemeLink();
COLORREF ThemeEscape();
COLORREF ThemeError();
COLORREF ThemeMarkdownCodeBack();
COLORREF ThemeFolderPaneBack();
COLORREF ThemeFolderPaneText();
COLORREF ThemeFolderPaneLine();
COLORREF ThemeFolderTreeSelectionBack();
COLORREF ThemeFolderTreeSelectionText();
COLORREF ThemeTabBarBack();
COLORREF ThemeTabActiveBack();
COLORREF ThemeTabInactiveBack();
COLORREF ThemeTabBorder();
COLORREF ThemeTabText();
COLORREF ThemeTabClose();
COLORREF ThemeTabCloseHoverBack();
COLORREF ThemeTabCloseHover();
COLORREF ThemeStatusBack();
COLORREF ThemeStatusLine();
COLORREF ThemeStatusText();
COLORREF ThemePopupBack();
COLORREF ThemePopupSurface();
COLORREF ThemePopupText();
COLORREF ThemePopupMutedText();
COLORREF ThemePopupBorder();
COLORREF ThemePopupInputBack();
COLORREF ThemePopupButtonBack(bool pressed);
COLORREF ThemeAccent();
COLORREF ThemeMenuBack();
COLORREF ThemeMenuHoverBack();
COLORREF ThemeMenuText();
COLORREF ThemeMenuMutedText();
COLORREF ThemeMenuBorder();
sptr_t ScintillaColourAlpha(COLORREF color);
void EnableNativeDarkMenuMode(bool dark);
void ApplyWindowChromeTheme(HWND window);
void ApplyControlTheme(HWND window);
HICON LoadSharedAppIcon(HINSTANCE instance, int width, int height);
void ApplyWindowAppIcons(HWND window);
void SetTaskbarStringProperty(IPropertyStore* propertyStore, REFPROPERTYKEY key, const wchar_t* value);
void ConfigureTaskbarProperties(HWND window);
void ApplyFolderPaneTreeTheme(HWND treeWindow);
void ApplyFolderTreeTheme();
ThemedMenuItem* GetThemedMenuItemData(HMENU menu, int index);
void ApplyMainMenuTheme();
void DrawThemedMenuItem(const DRAWITEMSTRUCT* drawItem);
void MeasureThemedMenuItem(MEASUREITEMSTRUCT* measureItem);
void ApplyEditorViewOptions();
void SetRepresentation(const char* character, const char* representation, int appearance);
void ConfigureControlCharacterRepresentations();
void ConfigureWordHighlightIndicator();
void SetMenuItemChecked(HMENU hMenu, UINT commandId, bool checked);
void SetMenuText(HMENU menu, UINT commandId, const wchar_t* text);
bool MenuHasCommand(HMENU menu, UINT commandId);
int MenuCommandIndex(HMENU menu, UINT commandId);
bool IsMenuSeparatorAt(HMENU menu, int index);
void EnsureEditColumnEditorCommand(HMENU editMenu);
void EnsureEditSearchCommands(HMENU editMenu);
wchar_t LanguageMenuSortLetter(const LanguageMenuLabel& label);
bool LanguageBelongsToGroup(const LanguageMenuLabel& label, const LanguageMenuGroup& group);
void ClearMenuItems(HMENU menu);
void RebuildLanguageMenuItems(HMENU syntaxMenu);
int FindSubMenuIndex(HMENU menu, HMENU submenu);
HMENU FindMenuContainingCommand(HMENU menu, UINT commandId);
void SetTopMenuText(HMENU menu, int index, const wchar_t* text);
void EnsureSettingsMenu(HMENU menu);
void UpdateMainMenuText();
void UpdateViewMenuCheck();
void ToggleShowSpaceAndTab();
void ToggleShowEndOfLine();
void ToggleShowAllCharacters();
void ToggleWordWrap();
const LanguageDefinition* FindLanguageDefinition(int commandId);
bool IsLanguageCommand(int commandId);
void ClearKeywordLists();
void SetEditorProperty(const char* name, const char* value);
bool SupportsCodeFolding(SyntaxFamily family);
void ConfigureFoldMarkers();
void ConfigureCodeFolding(SyntaxFamily family);
void ApplyBaseEditorStyles();
void ApplyCppLikeStyles();
void ApplyPythonStyles();
void ApplyHyperTextStyles();
void ApplyCssStyles();
void ApplyJsonStyles();
void ApplySqlStyles();
void ApplyBashStyles();
void ApplyPowerShellStyles();
void ApplyRustStyles();
void ApplyLuaStyles();
void ApplyRubyStyles();
void ApplyMarkdownStyles();
void ApplyYamlStyles();
void ApplyTomlStyles();
void ApplyPropertiesStyles();
void ApplyMakefileStyles();
void ApplyDiffStyles();
void ApplyBatchStyles();
void ApplyZigStyles();
void ApplyNimStyles();
void ApplyRegistryStyles();
void ApplyInnoStyles();
void ApplySyntaxFamilyStyles(SyntaxFamily family);
void UpdateLanguageMenuCheckRecursive(HMENU menu);
void UpdateLanguageMenuCheck();
void ApplyLanguage(int commandId);
void InitScintillaEditor();
void HandleEditorMarginClick(const SCNotification* notification);
void ApplyAppTheme();
bool IsActiveTabValid();
bool IsValidUtf8Bytes(const char* text, int length);
const wchar_t* EncodingDisplayName(DocumentEncoding encoding);
const wchar_t* EolDisplayName(int eolMode);
bool IsEditCommand(int commandId);
void ExecuteEditCommand(int commandId);
void DestroyPopupWindowWithoutFlash(HWND popupWindow);
void RestoreMainWindowAfterPopupClose(bool focusEditor);
void InvalidateTabBar();
void InvalidateStatusBar();
bool GetTabModifiedState(int tabIndex);
void SetActiveTabModified(bool modified);
bool ShouldShowTabInOpenFilesTree(const DocumentTab& tab);
bool HasOpenFilesTreeTabs();
OpenFileItem* GetOpenFileItemData(HTREEITEM treeItem);
HTREEITEM InsertOpenFilesTreeItem(int tabIndex);
void RefreshOpenFilesTree();
bool HandleOpenFilesTreeItemClickAt(POINT clientPoint);
sptr_t ClampEditorPosition(sptr_t position);
void CaptureActiveTab();
DocumentTab CreateUntitledTab();
DocumentTab CreateDefaultUntitledTab();
void UpdateWindowTitle();
void LoadTabIntoEditor(int tabIndex);
void SwitchToTab(int tabIndex);
int AddDocumentTab(DocumentTab tab);
bool IsSingleEmptyUntitledTab();
bool HasFileTabs();
bool IsEmptyUntitledTab(const DocumentTab& tab);
void RemoveEmptyUntitledTabs();
int AddDocumentTabReplacingDefaultBlank(DocumentTab tab);
int ClampFolderPaneWidth(int requestedWidth, int clientWidth);
int GetEffectiveFolderPaneWidth();
int ClampOpenFilesPaneHeight(int requestedHeight, int contentHeight);
int GetEffectiveOpenFilesPaneHeight(int contentHeight);
RECT GetFolderSplitterRect();
RECT GetFolderSectionSplitterRect();
bool IsPointOnFolderSplitter(POINT point);
bool IsPointOnFolderSectionSplitter(POINT point);
HWND EnsureFolderSplitterPreviewWindow();
void ShowFolderSplitterPreview(int paneWidth);
void HideFolderSplitterPreview();
void UpdateFolderSplitterPreview(int paneWidth);
void DrawFolderSplitter(HDC hdc);
void DrawFolderPaneHeader(HDC hdc, const RECT& headerRect, const wchar_t* title);
void DrawFolderSectionSplitter(HDC hdc);
void DrawFolderPaneDecorations(HDC hdc, int paneWidth, int contentHeight);
void FillMainClientBackground(HDC hdc);
void LayoutChildWindows();
RECT GetFolderToggleButtonRect(HWND owner);
void RedrawFolderToggleButton();
void DrawFolderToggleButton(HWND owner);
void SetFolderPaneVisible(bool visible);
void ToggleFolderPane();
void ClearFolderPane();
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow);
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
std::wstring ShortcutCommandName(int commandId);
std::wstring ShortcutText(BYTE modifiers, WORD key);
std::wstring MenuLabelWithShortcut(const wchar_t* chinese, const wchar_t* english, int commandId);
void ApplyPopupMenuTheme(HMENU menu, std::vector<std::unique_ptr<ThemedMenuItem>>& storage);
std::wstring FileNameFromPath(const std::wstring& path);
int DetectLanguageFromPath(const std::wstring& path);
std::string GetEditorText();
std::string WideToUtf8(const std::wstring& text);
std::wstring Utf8ToWide(const std::string& text);
std::string DecodeFileBytesToUtf8(const std::vector<char>& content, size_t byteCount, DocumentEncoding& encoding);
std::vector<char> EncodeUtf8ForFile(const std::string& utf8Text, DocumentEncoding encoding);
int DetectEolModeFromText(const std::string& text);
std::wstring GetTabDisplayTitle(const DocumentTab& tab);
std::wstring GetTabTooltipText(int tabIndex);
int FindOpenTabByPath(const std::wstring& path);
const wchar_t* ColumnEditorTitle();
void ShowColumnEditorMessage(HWND owner, const std::wstring& message);
void ShowColumnEditorMessage(HWND owner, const wchar_t* chinese, const wchar_t* english);
std::wstring TrimWhitespace(const std::wstring& text);
std::wstring GetControlText(HWND parent, int controlId);
bool TryParseInt64(const std::wstring& text, long long& value);
bool TryParsePositiveInt(const std::wstring& text, int& value);
size_t MinimumNumberWidthFromInitialText(const std::wstring& text);
bool GetActiveColumnSelection(ColumnEditSelection& selection, std::wstring& error);
bool CheckedAddInt64(long long left, long long right, long long& result);
bool IsRectangularSelectionMode(int selectionMode);
sptr_t PositionForSelectionColumn(sptr_t line, sptr_t column, sptr_t& virtualSpace);
void SetRectangularSelectionByColumns(sptr_t firstLine, sptr_t lastLine, sptr_t startColumn, sptr_t endColumn);
bool ApplyColumnEditRequest(HWND owner, const ColumnEditRequest& request);
DocumentEncoding EncodingFromCommand(int commandId);
int CommandFromEncoding(DocumentEncoding encoding);
int EolModeFromCommand(int commandId);
int CommandFromEolMode(int eolMode);
void SetActiveDocumentEncoding(DocumentEncoding encoding);
void SetActiveDocumentEolMode(int eolMode);
void SetControlFont(HWND control);
HBRUSH GetPopupBackBrush();
HBRUSH GetPopupSurfaceBrush();
HBRUSH GetPopupInputBrush();
HBRUSH GetMenuBackBrush();
void InvalidateFolderTree();
void InvalidateOpenFilesTree();
void DrawRoundedPanel(HDC hdc, const RECT& rect, COLORREF fill, COLORREF border, int radius);
void DrawOwnerButton(const DRAWITEMSTRUCT* drawItem);
bool IsSettingsOptionControl(int controlId);
bool IsColumnEditorOptionControl(int controlId);
bool IsSettingsNavigationControl(int controlId);
bool IsSettingsShortcutHotKeyControl(int controlId);
int SettingsShortcutIndexFromControlId(int controlId);
bool IsShortcutModifierKey(WORD key);
BYTE ShortcutModifiersFromKeyboardState();
bool IsSettingsRadioControl(int controlId);
bool IsColumnEditorRadioControl(int controlId);
bool IsSettingsOptionChecked(int controlId);
bool IsColumnEditorOptionChecked(int controlId);
void RedrawSettingsOptions(HWND settingsWindow);
void DrawPopupOptionControl(const DRAWITEMSTRUCT* drawItem);
void DrawSettingsNavigationItem(const DRAWITEMSTRUCT* drawItem);
HWND CreateSettingsControl(HWND parent, const wchar_t* className, const wchar_t* text,
    DWORD style, int x, int y, int width, int height, int id);
void SetSettingsControlVisible(HWND settingsWindow, int controlId, bool visible);
void SetHotKeyControlShortcut(HWND control, const ShortcutBinding& shortcut);
LRESULT CALLBACK SettingsShortcutEditProc(HWND editControl, UINT message, WPARAM wParam, LPARAM lParam,
    UINT_PTR subclassId, DWORD_PTR refData);
void ResetSettingsShortcutControls(HWND settingsWindow);
bool ShortcutNeedsModifier(WORD key);
void UpdateSettingsTabVisibility(HWND settingsWindow);
void CreateSettingsShortcutControls(HWND settingsWindow);
void InitializeSettingsControls(HWND settingsWindow);
bool ApplySettingsFromWindow(HWND settingsWindow);
void ShowSettingsWindow();
void InitializeAboutControls(HWND aboutWindow);
void ShowAboutWindow();
void UpdateColumnEditorModeControls(HWND columnWindow);
HWND CreateColumnEditorControl(HWND parent, const wchar_t* className, const wchar_t* text,
    DWORD style, int x, int y, int width, int height, int id);
void InitializeColumnEditorControls(HWND columnWindow);
ColumnPaddingMode ReadColumnPaddingMode(HWND columnWindow);
bool ReadColumnEditorRequest(HWND columnWindow, ColumnEditRequest& request);
void ShowColumnEditorWindow();
LRESULT CALLBACK ColumnEditorWndProc(HWND columnWindow, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK AboutWndProc(HWND aboutWindow, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SettingsWndProc(HWND settingsWindow, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FolderSplitterPreviewWndProc(HWND previewWindow, UINT message, WPARAM wParam, LPARAM lParam);
void ShowErrorMessage(const wchar_t* message);
bool SaveCurrentFileAs();
bool SaveCurrentFile();
bool PromptSaveIfModified();
void CloseTab(int tabIndex);
bool PromptSaveAllTabs();
void NewFile();
bool HandleDroppedFiles(HDROP drop);
void OpenFileCommand();
bool LoadFileIntoEditor(const std::wstring& path, bool openedFromFolder = false);
bool IsRestoredMainWindowRectSizeValid(const RECT& rect);
void CaptureMainWindowPlacement();
bool AdjustMainWindowPlacementToMonitor(RECT& rect);
bool ApplyMainWindowPlacement(HWND window, int nCmdShow);
char HexDigit(unsigned char value);
int HexValue(char ch);
DocumentEncoding SessionEncodingFromInt(int value);
int SessionEncodingToInt(DocumentEncoding encoding);
void LoadAppSettings();
void SaveAppSettings();
bool SaveSessionState();
bool LoadStartupTabs();
int GetScintillaSearchFlags(DWORD findOptions);
void ClearWordHighlights();
void HighlightSelectedWordOccurrences();
bool SelectCurrentTarget(bool focusEditor);
bool FindTextInEditor(const wchar_t* findText, DWORD findOptions, bool searchDown, bool wrap, bool focusEditor = true);
bool ReplaceCurrentSelection(const wchar_t* findText, const wchar_t* replaceText, DWORD findOptions);
sptr_t PositionAfter(sptr_t position);
int ReplaceAllMatches(const wchar_t* findText, const wchar_t* replaceText, DWORD findOptions);
int CountMatches(const wchar_t* findText, DWORD findOptions);
DWORD FindOptionsFromRequest(const OpenEditFindRequest& request, bool searchDown);
void RememberFindRequest(const OpenEditFindRequest& request, bool searchDown);
bool FindWindowFind(void*, const OpenEditFindRequest& request, bool previous);
int FindWindowCount(void*, const OpenEditFindRequest& request);
bool FindWindowReplace(void*, const OpenEditFindRequest& request);
int FindWindowReplaceAll(void*, const OpenEditFindRequest& request);
void OpenFindReplaceDialog(bool replaceDialog);
void FindNextCommand(bool searchDown);
LRESULT HandleFindReplaceMessage(LPARAM lParam);
int AddThemedFolderIconToImageList(HIMAGELIST imageList, int iconWidth, int iconHeight, bool open);
int AddThemedFileIconToImageList(HIMAGELIST imageList, int iconWidth, int iconHeight);
void InitializeFolderTreeImageList();
LRESULT HandleFolderTreeCustomDraw(LPARAM lParam);
LRESULT CALLBACK FolderTreeWndProc(HWND treeWindow, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OpenFilesTreeWndProc(HWND treeWindow, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditorWndProc(HWND editorWindow, UINT message, WPARAM wParam, LPARAM lParam);
FolderItem* StoreFolderItem(FolderItem item);
void DeleteTreeChildren(HTREEITEM parentItem);
HTREEITEM InsertFolderTreeItem(HTREEITEM parentItem, const FolderItem& source);
void UpdateFolderNodeIcon(HTREEITEM treeItem, bool expanded);
FolderItem* GetFolderItemData(HTREEITEM treeItem);
void LoadFolderNodeChildren(HTREEITEM treeItem, FolderItem* folderItem);
void PrepareFolderTreeItemForExpand(HTREEITEM treeItem);
void ExpandFolderTreeItem(HTREEITEM treeItem);
void ToggleFolderTreeItem(HTREEITEM treeItem);
void OpenFolderCommand();
void PopulateFolderTree(const std::wstring& folderPath);
bool HandleFolderTreeItemClickAt(POINT clientPoint, bool doubleClick);
bool HandleFolderTreeSingleClick();
HTREEITEM GetFolderTreeSelection();
void RefreshFolderTreeItem(HTREEITEM treeItem);
void CopySelectedFolderItemName();
void OpenSelectedFolderItemInExplorer();
void ShowFolderTreeContextMenu(POINT screenPoint);
int GetTabBarTabWidth(HWND tabBar);
RECT GetTabBarTabRect(HWND tabBar, int tabIndex);
RECT GetTabCloseButtonRect(const RECT& tabRect);
RECT GetTabSaveIconRect(const RECT& tabRect);
int HitTestTabBar(HWND tabBar, POINT point, bool* closeButton);
HWND EnsureTabTooltip(HWND tabBar);
void HideTabTooltip();
void UpdateTabTooltip(HWND tabBar, int tabIndex, POINT point);
void UpdateTabHoverState(HWND tabBar, POINT point);
bool CloseTabAtPoint(HWND tabBar, POINT point);
void DrawTabSaveIcon(HDC hdc, RECT iconRect, bool modified);
void DrawTabCloseButton(HDC hdc, RECT closeRect, bool hovered);
void FillTopRoundedRect(HDC hdc, const RECT& rect, COLORREF color);
void DrawTabBar(HWND tabBar, HDC hdc);
LRESULT CALLBACK TabBarWndProc(HWND tabBar, UINT message, WPARAM wParam, LPARAM lParam);
DocumentEncoding GetActiveDocumentEncoding();
int GetActiveDocumentEolMode();
sptr_t CountCharactersInRange(sptr_t start, sptr_t end);
RECT GetStatusEolRect(HWND statusBar);
RECT GetStatusFolderToggleRect(HWND statusBar);
RECT GetStatusEncodingRect(HWND statusBar);
StatusHitArea HitTestStatusBar(HWND statusBar, POINT point);
void DrawStatusSeparator(HDC hdc, int x, int top, int bottom);
void DrawStatusFolderToggleIcon(HDC hdc, const RECT& buttonRect);
void DrawStatusBar(HWND statusBar, HDC hdc);
void ShowEncodingMenu(HWND statusBar, POINT point);
void ShowEolMenu(HWND statusBar, POINT point);
LRESULT CALLBACK StatusBarWndProc(HWND statusBar, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

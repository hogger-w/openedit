#include "openedit_internal.h"

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "UxTheme.lib")

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hWnd = nullptr;
HWND g_hSci = nullptr;          // Scintilla窗口句柄
HWND g_hOpenFilesTree = nullptr;
HWND g_hFolderTree = nullptr;
HWND g_hTabBar = nullptr;
HWND g_hStatusBar = nullptr;
HWND g_hTabTooltip = nullptr;
HWND g_hFindReplaceDialog = nullptr;
HWND g_hSettingsWindow = nullptr;
HWND g_hAboutWindow = nullptr;
HWND g_hFolderSplitterPreview = nullptr;
HWND g_hColumnEditorWindow = nullptr;
HACCEL g_hAccelTable = nullptr;
HMENU g_hSettingsMenu = nullptr;
HIMAGELIST g_hFolderTreeImageList = nullptr;
WNDPROC g_originalFolderTreeProc = nullptr;
WNDPROC g_originalOpenFilesTreeProc = nullptr;
WNDPROC g_originalEditorProc = nullptr;
std::unique_ptr<OpenEditFindWindow> g_findWindow;
HBRUSH g_hPopupBackBrush = nullptr;
HBRUSH g_hPopupSurfaceBrush = nullptr;
HBRUSH g_hPopupInputBrush = nullptr;
HBRUSH g_hMenuBackBrush = nullptr;

SciFnDirect g_pSciFn = nullptr; // Scintilla官方直接调用函数
sptr_t g_pSciPtr = 0;           // Scintilla实例指针
UINT g_findReplaceMessage = 0;
int g_currentLanguageCommand = IDM_LANG_TEXT;
AppTheme g_appTheme = AppTheme::Dark;
AppLanguage g_appLanguage = AppLanguage::Chinese;
bool g_folderPaneVisible = false;
bool g_draggingFolderSplitter = false;
bool g_draggingFolderSectionSplitter = false;
bool g_applyingFolderTreeTheme = false;
int g_folderPaneWidth = kFolderPaneMinWidth;
int g_folderSplitterPreviewWidth = -1;
int g_openFilesPaneHeight = kFolderPaneOpenFilesDefaultHeight;
bool g_restorePreviousFilesOnStartup = true;
bool g_restoreFolderInSession = false;
MainWindowPlacement g_mainWindowPlacement;
AppTheme g_settingsDraftTheme = AppTheme::Dark;
AppLanguage g_settingsDraftLanguage = AppLanguage::Chinese;
ColumnEditorMode g_columnEditorMode = ColumnEditorMode::Text;
bool g_settingsDraftRestorePreviousFiles = true;
int g_settingsActiveTab = 0;
bool g_showSpaceAndTab = false;
bool g_showEndOfLine = false;
bool g_wordWrapEnabled = true;
std::wstring g_currentFilePath;
std::wstring g_currentFolderPath;
std::wstring g_tabTooltipText;
int g_hoveredTabIndex = -1;
int g_hoveredTabCloseIndex = -1;
bool g_trackingTabMouse = false;
FINDREPLACEW g_findReplace{};
wchar_t g_findText[kSearchTextBufferLength]{};
wchar_t g_replaceText[kSearchTextBufferLength]{};
DWORD g_lastFindOptions = FR_DOWN;
std::string g_highlightedWord;

std::vector<std::unique_ptr<ThemedMenuItem>> g_mainMenuItems;

std::array<ShortcutBinding, kShortcutBindingCount> g_shortcutBindings = { {
    { IDM_FILE_NEW, 'N', kShortcutCtrl, 'N', kShortcutCtrl },
    { IDM_FILE_OPEN, 'O', kShortcutCtrl, 'O', kShortcutCtrl },
    { IDM_FILE_OPEN_FOLDER, 'O', kShortcutCtrl | kShortcutShift, 'O', kShortcutCtrl | kShortcutShift },
    { IDM_FILE_SAVE, 'S', kShortcutCtrl, 'S', kShortcutCtrl },
    { IDM_FILE_SAVE_AS, 'S', kShortcutCtrl | kShortcutShift, 'S', kShortcutCtrl | kShortcutShift },
    { IDM_TAB_CLOSE, 'W', kShortcutCtrl, 'W', kShortcutCtrl },
    { IDM_EDIT_UNDO, 'Z', kShortcutCtrl, 'Z', kShortcutCtrl },
    { IDM_EDIT_REDO, 'Y', kShortcutCtrl, 'Y', kShortcutCtrl },
    { IDM_EDIT_CUT, 'X', kShortcutCtrl, 'X', kShortcutCtrl },
    { IDM_EDIT_COPY, 'C', kShortcutCtrl, 'C', kShortcutCtrl },
    { IDM_EDIT_PASTE, 'V', kShortcutCtrl, 'V', kShortcutCtrl },
    { IDM_EDIT_DELETE, VK_DELETE, 0, VK_DELETE, 0 },
    { IDM_EDIT_SELECT_ALL, 'A', kShortcutCtrl, 'A', kShortcutCtrl },
    { IDM_SEARCH_FIND, 'F', kShortcutCtrl, 'F', kShortcutCtrl },
    { IDM_SEARCH_FIND_NEXT, VK_F3, 0, VK_F3, 0 },
    { IDM_SEARCH_FIND_PREVIOUS, VK_F3, kShortcutShift, VK_F3, kShortcutShift },
    { IDM_SEARCH_REPLACE, 'H', kShortcutCtrl, 'H', kShortcutCtrl },
} };

std::array<ShortcutBinding, kShortcutBindingCount> g_settingsDraftShortcuts{};

std::vector<std::unique_ptr<FolderItem>> g_folderItems;
std::vector<std::unique_ptr<OpenFileItem>> g_openFileItems;
int g_folderIconIndex = 0;
int g_folderOpenIconIndex = 0;
int g_textFileIconIndex = 0;

std::vector<DocumentTab> g_tabs;
int g_activeTabIndex = -1;
int g_contextTabIndex = -1;
int g_nextUntitledIndex = 1;
bool g_loadingTabContent = false;

const std::array<LanguageDefinition, 28> kLanguages = { {
    { IDM_LANG_TEXT, nullptr, SyntaxFamily::Plain, {} },
    { IDM_LANG_CPP, "cpp", SyntaxFamily::CppLike, {
        "alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern false final float for friend goto if import inline int long module mutable namespace new noexcept not not_eq nullptr operator or or_eq override private protected public register reinterpret_cast requires return short signed sizeof static static_assert static_cast struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq",
        "int8_t int16_t int32_t int64_t intptr_t size_t ssize_t ptrdiff_t uint8_t uint16_t uint32_t uint64_t uintptr_t FILE std string vector map unordered_map set unordered_set array optional variant shared_ptr unique_ptr weak_ptr",
        "attention author brief bug code date deprecated note param return see since todo version warning",
        "HWND HINSTANCE LPARAM LRESULT WPARAM WCHAR DWORD BOOL HANDLE COLORREF RECT POINT SIZE"
    } },
    { IDM_LANG_CSHARP, "cpp", SyntaxFamily::CppLike, {
        "abstract as async await base bool break byte case catch char checked class const continue decimal default delegate do double else enum event explicit extern false finally fixed float for foreach get goto if implicit in init int interface internal is lock long namespace new null object operator out override params partial private protected public readonly record ref required return sbyte sealed set short sizeof stackalloc static string struct switch this throw true try typeof uint ulong unchecked unsafe ushort using var virtual void volatile when where while yield",
        "Action Console DateTime Dictionary Enumerable Exception Func Guid IEnumerable IList List Math Nullable Object Regex String StringBuilder Task ValueTask"
    } },
    { IDM_LANG_JAVA, "cpp", SyntaxFamily::CppLike, {
        "abstract assert boolean break byte case catch char class const continue default do double else enum extends final finally float for goto if implements import instanceof int interface long native new null package private protected public return short static strictfp super switch synchronized this throw throws transient true try var void volatile while yield record sealed permits",
        "ArrayList Boolean Double Exception HashMap Integer List Long Map Object Optional Set Stream String StringBuilder System"
    } },
    { IDM_LANG_JAVASCRIPT, "cpp", SyntaxFamily::CppLike, {
        "async await break case catch class const continue debugger default delete do else export extends false finally for from function get if import in instanceof let new null of return set static super switch this throw true try typeof undefined var void while with yield",
        "Array Boolean Date Error Function JSON Map Math Number Object Promise Proxy Reflect RegExp Set String Symbol WeakMap WeakSet console document window"
    } },
    { IDM_LANG_TYPESCRIPT, "cpp", SyntaxFamily::CppLike, {
        "abstract any as asserts async await bigint boolean break case catch class const constructor continue debugger declare default delete do else enum export extends false finally for from function get if implements import in infer instanceof interface is keyof let module namespace never new null number object of override package private protected public readonly require return set static string super switch symbol this throw true try type typeof undefined unique unknown var void while with yield",
        "Array Boolean Date Error Function JSON Map Math Number Object Promise ReadonlyArray Record RegExp Set String Symbol WeakMap WeakSet console document window"
    } },
    { IDM_LANG_PYTHON, "python", SyntaxFamily::Python, {
        "False None True and as assert async await break case class continue def del elif else except finally for from global if import in is lambda match nonlocal not or pass raise return try while with yield",
        "__init__ abs all any bool bytearray bytes callable chr classmethod dict dir enumerate filter float format frozenset getattr hasattr int isinstance issubclass len list map max min next object open print property range repr reversed round set setattr slice sorted staticmethod str sum super tuple type zip"
    } },
    { IDM_LANG_HTML, "hypertext", SyntaxFamily::HyperText, {
        "a abbr address article aside audio b base blockquote body br button canvas code div dl dt dd em fieldset figcaption figure footer form h1 h2 h3 h4 h5 h6 head header hr html i iframe img input label legend li link main meta nav noscript ol option p pre script section select small span strong style table tbody td textarea th thead title tr ul video class id href src alt rel type name value placeholder role aria-label data",
        "async await break case catch class const continue default delete do else export extends false finally for function if import in instanceof let new null return static super switch this throw true try typeof undefined var void while yield",
        "",
        "False None True and as assert async await break class continue def elif else except finally for from if import in is lambda not or pass raise return try while with yield",
        "abstract and array as bool boolean break callable case catch class clone const continue declare default die do echo else elseif empty enddeclare endfor endforeach endif endswitch endwhile enum eval exit extends false final finally float fn for foreach function global goto if implements include include_once instanceof insteadof int interface isset list match mixed namespace new null object or parent print private protected public readonly require require_once resource return self static string switch throw trait true try unset use var void while xor yield"
    } },
    { IDM_LANG_XML, "xml", SyntaxFamily::HyperText, {} },
    { IDM_LANG_CSS, "css", SyntaxFamily::Css, {
        "align-content align-items align-self animation animation-delay animation-duration animation-name background background-color background-image border border-bottom border-color border-radius border-top box-shadow box-sizing color content cursor display flex flex-direction flex-wrap font font-family font-size font-weight gap grid grid-template-columns height justify-content left line-height margin max-height max-width min-height min-width opacity overflow padding position right text-align text-decoration top transform transition visibility width z-index",
        "active checked disabled empty enabled first-child first-of-type focus hover last-child last-of-type not nth-child nth-of-type root visited",
        "align-content align-items animation background-size border-radius box-shadow box-sizing flex flex-basis flex-direction flex-grow flex-shrink grid-template-areas grid-template-columns grid-template-rows opacity outline transform transition",
        "accent-color aspect-ratio backdrop-filter block-size clip-path container container-name container-type filter grid-template inset inline-size object-fit place-content place-items user-select",
        "after before first-letter first-line marker placeholder selection"
    } },
    { IDM_LANG_JSON, "json", SyntaxFamily::Json, {
        "false null true",
        "@base @container @context @graph @id @index @language @list @reverse @set @type @value @vocab"
    } },
    { IDM_LANG_SQL, "sql", SyntaxFamily::Sql, {
        "add alter and as asc between by case check column constraint create database default delete desc distinct drop else exists false foreign from full group having in index inner insert into is join key left like limit not null on or order outer primary procedure references right select set table then true union unique update values view when where",
        "bigint binary bit blob boolean char clob date datetime decimal double float int integer json money nchar numeric nvarchar real smallint text time timestamp tinyint varchar"
    } },
    { IDM_LANG_BASH, "bash", SyntaxFamily::Bash, {
        "alias bg bind break builtin case cd command compgen complete continue declare dirs disown do done echo elif else enable esac eval exec exit export false fg fi for function getopts hash help history if in jobs kill let local logout popd printf pushd pwd read readonly return select set shift shopt source suspend test then time times trap true type typeset ulimit umask unalias unset until wait while"
    } },
    { IDM_LANG_POWERSHELL, "powershell", SyntaxFamily::PowerShell, {
        "begin break catch class continue data define do dynamicparam else elseif end enum exit filter finally for foreach from function if in param process return switch throw trap try until using var while workflow",
        "add-content clear-content clear-item clear-variable compare-object convertfrom-json convertto-json copy-item foreach-object get-childitem get-command get-content get-help get-item get-location get-member get-process get-service get-variable invoke-command join-path measure-object move-item new-item out-file remove-item rename-item select-object set-content set-item set-location sort-object start-process stop-process test-path where-object write-host write-output",
        "cat cd clear cp curl del dir echo erase foreach gci grep history ls man md mkdir mv popd ps pushd pwd rm rmdir sleep sort tee type wget"
    } },
    { IDM_LANG_RUST, "rust", SyntaxFamily::Rust, {
        "as async await break const continue crate dyn else enum extern false fn for if impl in let loop match mod move mut pub ref return self Self static struct super trait true type unsafe use where while",
        "bool char f32 f64 i8 i16 i32 i64 i128 isize str u8 u16 u32 u64 u128 usize",
        "Box Clone Copy Debug Default Drop Eq Err Error Fn FnMut FnOnce From Into Iterator None Ok Option Result Send Some String Sync Vec"
    } },
    { IDM_LANG_LUA, "lua", SyntaxFamily::Lua, {
        "and break do else elseif end false for function goto if in local nil not or repeat return then true until while",
        "assert collectgarbage dofile error getmetatable ipairs load loadfile next pairs pcall print rawequal rawget rawlen rawset require select setmetatable tonumber tostring type xpcall",
        "coroutine.create coroutine.resume coroutine.running coroutine.status coroutine.wrap coroutine.yield math.abs math.ceil math.floor math.max math.min math.random math.sin math.sqrt string.byte string.find string.format string.gsub string.len string.lower string.match string.sub string.upper table.concat table.insert table.remove table.sort",
        "io.close io.flush io.input io.lines io.open io.output io.read io.type io.write os.clock os.date os.difftime os.execute os.exit os.getenv os.remove os.rename os.time"
    } },
    { IDM_LANG_RUBY, "ruby", SyntaxFamily::Ruby, {
        "__FILE__ __LINE__ alias and begin break case class def defined? do else elsif end ensure false for if in module next nil not or redo rescue retry return self super then true undef unless until when while yield"
    } },
    { IDM_LANG_MARKDOWN, "markdown", SyntaxFamily::Markdown, {} },
    { IDM_LANG_YAML, "yaml", SyntaxFamily::Yaml, {
        "false no null off on true yes"
    } },
    { IDM_LANG_TOML, "toml", SyntaxFamily::Toml, {
        "false inf nan true"
    } },
    { IDM_LANG_PROPERTIES, "props", SyntaxFamily::Properties, {} },
    { IDM_LANG_MAKEFILE, "makefile", SyntaxFamily::Makefile, {
        "define else endef endif export ifdef ifeq ifndef ifneq include override private undefine unexport vpath",
        ".DEFAULT .DELETE_ON_ERROR .EXPORT_ALL_VARIABLES .IGNORE .INTERMEDIATE .LOW_RESOLUTION_TIME .NOTINTERMEDIATE .ONESHELL .PHONY .POSIX .PRECIOUS .SECONDARY .SECONDEXPANSION .SILENT .SUFFIXES"
    } },
    { IDM_LANG_DIFF, "diff", SyntaxFamily::Diff, {} },
    { IDM_LANG_BATCH, "batch", SyntaxFamily::Batch, {
        "assoc attrib call cd chcp chdir choice cls color copy date del dir do echo else endlocal erase errorlevel exist exit for ftype goto if in md mkdir move not pause popd prompt pushd rd rem ren rename rmdir set setlocal shift start time title type ver verify vol",
        "break cacls chkdsk comp compact convert diskcopy doskey find findstr format graftabl help ipconfig label mode more net path ping replace robocopy sort subst tree where xcopy"
    } },
    { IDM_LANG_ZIG, "zig", SyntaxFamily::Zig, {
        "addrspace align allowzero and anyframe anytype asm async await break callconv catch comptime const continue defer else enum errdefer error export extern fn for if inline linksection noalias noinline nosuspend opaque or orelse packed pub resume return struct suspend switch test threadlocal try union unreachable usingnamespace var volatile while",
        "bool c_char c_int c_long c_longdouble c_longlong c_short c_uint c_ulong c_ulonglong c_ushort comptime_float comptime_int f16 f32 f64 f80 f128 isize noreturn type usize void",
        "i8 i16 i32 i64 i128 u8 u16 u32 u64 u128",
        "false null true undefined"
    } },
    { IDM_LANG_NIM, "nim", SyntaxFamily::Nim, {
        "addr and as asm bind block break case cast concept const continue converter defer discard distinct div do elif else end enum except export finally for from func if import in include interface is isnot iterator let macro method mixin mod nil not notin object of or out proc ptr raise ref return shl shr static template try tuple type using var when while xor yield",
        "array bool char cstring float float32 float64 int int8 int16 int32 int64 openArray pointer seq string uint uint8 uint16 uint32 uint64 varargs void"
    } },
    { IDM_LANG_REGISTRY, "registry", SyntaxFamily::Registry, {} },
    { IDM_LANG_INNO, "inno", SyntaxFamily::Inno, {
        "AppName AppVersion Compression DefaultDirName DefaultGroupName OutputBaseFilename SetupIconFile SolidCompression",
        "ArchitecturesAllowed ArchitecturesInstallIn64BitMode ChangesAssociations ChangesEnvironment CloseApplications CreateAppDir DisableDirPage DisableProgramGroupPage PrivilegesRequired UninstallDisplayIcon",
        "Code Components CustomMessages Dirs Files Icons InstallDelete Languages Messages Registry Run Setup Tasks Types UninstallDelete",
        "begin break case const continue do downto else end except finally for function if nil not of or procedure repeat then to try until var while"
    } },
} };

sptr_t Sci(unsigned int message, uptr_t wParam, sptr_t lParam)
{
    if (!g_pSciFn || !g_pSciPtr)
        return 0;
    return g_pSciFn(g_pSciPtr, message, wParam, lParam);
}

bool IsDarkTheme()
{
    return g_appTheme == AppTheme::Dark;
}

const wchar_t* UiText(const wchar_t* chinese, const wchar_t* english)
{
    return g_appLanguage == AppLanguage::Chinese ? chinese : english;
}

ShortcutBinding* FindShortcutBinding(int commandId)
{
    for (ShortcutBinding& shortcut : g_shortcutBindings)
    {
        if (shortcut.commandId == commandId)
            return &shortcut;
    }
    return nullptr;
}

const ShortcutBinding* FindShortcutBinding(int commandId, const std::array<ShortcutBinding, kShortcutBindingCount>& shortcuts)
{
    for (const ShortcutBinding& shortcut : shortcuts)
    {
        if (shortcut.commandId == commandId)
            return &shortcut;
    }
    return nullptr;
}

std::wstring ShortcutCommandName(int commandId)
{
    switch (commandId)
    {
    case IDM_FILE_NEW:
        return UiText(L"\u65B0\u5EFA", L"New");
    case IDM_FILE_OPEN:
        return UiText(L"\u6253\u5F00\u6587\u4EF6", L"Open File");
    case IDM_FILE_OPEN_FOLDER:
        return UiText(L"\u6253\u5F00\u6587\u4EF6\u5939", L"Open Folder");
    case IDM_FILE_SAVE:
        return UiText(L"\u4FDD\u5B58", L"Save");
    case IDM_FILE_SAVE_AS:
        return UiText(L"\u53E6\u5B58\u4E3A", L"Save As");
    case IDM_TAB_CLOSE:
        return UiText(L"\u5173\u95ED\u5F53\u524D\u6807\u7B7E", L"Close Current Tab");
    case IDM_EDIT_UNDO:
        return UiText(L"\u64A4\u9500", L"Undo");
    case IDM_EDIT_REDO:
        return UiText(L"\u91CD\u505A", L"Redo");
    case IDM_EDIT_CUT:
        return UiText(L"\u526A\u5207", L"Cut");
    case IDM_EDIT_COPY:
        return UiText(L"\u590D\u5236", L"Copy");
    case IDM_EDIT_PASTE:
        return UiText(L"\u7C98\u8D34", L"Paste");
    case IDM_EDIT_DELETE:
        return UiText(L"\u5220\u9664", L"Delete");
    case IDM_EDIT_SELECT_ALL:
        return UiText(L"\u5168\u9009", L"Select All");
    case IDM_SEARCH_FIND:
        return UiText(L"\u67E5\u627E", L"Find");
    case IDM_SEARCH_FIND_NEXT:
        return UiText(L"\u67E5\u627E\u4E0B\u4E00\u4E2A", L"Find Next");
    case IDM_SEARCH_FIND_PREVIOUS:
        return UiText(L"\u67E5\u627E\u4E0A\u4E00\u4E2A", L"Find Previous");
    case IDM_SEARCH_REPLACE:
        return UiText(L"\u66FF\u6362", L"Replace");
    default:
        return L"";
    }
}

std::wstring ShortcutKeyName(WORD key)
{
    if (key >= 'A' && key <= 'Z')
        return std::wstring(1, static_cast<wchar_t>(key));
    if (key >= '0' && key <= '9')
        return std::wstring(1, static_cast<wchar_t>(key));
    if (key >= VK_F1 && key <= VK_F24)
        return L"F" + std::to_wstring((key - VK_F1) + 1);

    switch (key)
    {
    case VK_DELETE:
        return L"Del";
    case VK_INSERT:
        return L"Ins";
    case VK_HOME:
        return L"Home";
    case VK_END:
        return L"End";
    case VK_PRIOR:
        return L"Page Up";
    case VK_NEXT:
        return L"Page Down";
    case VK_LEFT:
        return L"Left";
    case VK_RIGHT:
        return L"Right";
    case VK_UP:
        return L"Up";
    case VK_DOWN:
        return L"Down";
    case VK_BACK:
        return L"Backspace";
    case VK_TAB:
        return L"Tab";
    case VK_RETURN:
        return L"Enter";
    case VK_ESCAPE:
        return L"Esc";
    case VK_SPACE:
        return L"Space";
    default:
        break;
    }

    const UINT scanCode = MapVirtualKeyW(key, MAPVK_VK_TO_VSC);
    wchar_t name[64]{};
    if (scanCode != 0 && GetKeyNameTextW(static_cast<LONG>(scanCode << 16), name,
        static_cast<int>(sizeof(name) / sizeof(name[0]))) > 0)
        return name;

    return std::wstring(L"VK ") + std::to_wstring(key);
}

std::wstring ShortcutText(BYTE modifiers, WORD key)
{
    if (key == 0)
        return L"";

    std::wstring text;
    if (modifiers & kShortcutCtrl)
        text += L"Ctrl+";
    if (modifiers & kShortcutShift)
        text += L"Shift+";
    if (modifiers & kShortcutAlt)
        text += L"Alt+";
    text += ShortcutKeyName(key);
    return text;
}

std::wstring ShortcutTextForCommand(int commandId)
{
    const ShortcutBinding* shortcut = FindShortcutBinding(commandId, g_shortcutBindings);
    return shortcut ? ShortcutText(shortcut->modifiers, shortcut->key) : L"";
}

std::wstring StripMenuMnemonic(std::wstring text)
{
    for (size_t index = 0; index < text.size();)
    {
        if (text[index] == L'(' && index + 3 < text.size() &&
            text[index + 1] == L'&' && text[index + 3] == L')')
        {
            text.erase(index, 4);
            continue;
        }

        if (text[index] == L'&')
        {
            text.erase(index, 1);
            continue;
        }

        ++index;
    }
    return text;
}

std::wstring MenuLabelWithShortcut(const wchar_t* chinese, const wchar_t* english, int commandId)
{
    std::wstring label = StripMenuMnemonic(UiText(chinese, english));
    const std::wstring shortcut = ShortcutTextForCommand(commandId);
    if (!shortcut.empty())
        label += L"\t" + shortcut;
    return label;
}

BYTE AcceleratorFlagsFromShortcut(BYTE modifiers)
{
    BYTE flags = FVIRTKEY;
    if (modifiers & kShortcutCtrl)
        flags |= FCONTROL;
    if (modifiers & kShortcutShift)
        flags |= FSHIFT;
    if (modifiers & kShortcutAlt)
        flags |= FALT;
    return flags;
}

WORD HotKeyControlModifiers(BYTE modifiers)
{
    WORD hotKeyModifiers = 0;
    if (modifiers & kShortcutCtrl)
        hotKeyModifiers |= HOTKEYF_CONTROL;
    if (modifiers & kShortcutShift)
        hotKeyModifiers |= HOTKEYF_SHIFT;
    if (modifiers & kShortcutAlt)
        hotKeyModifiers |= HOTKEYF_ALT;
    return hotKeyModifiers;
}

BYTE ShortcutModifiersFromHotKey(WORD hotKey)
{
    const BYTE hotKeyModifiers = HIBYTE(hotKey);
    BYTE modifiers = 0;
    if (hotKeyModifiers & HOTKEYF_CONTROL)
        modifiers |= kShortcutCtrl;
    if (hotKeyModifiers & HOTKEYF_SHIFT)
        modifiers |= kShortcutShift;
    if (hotKeyModifiers & HOTKEYF_ALT)
        modifiers |= kShortcutAlt;
    return modifiers;
}

void RebuildAcceleratorTable()
{
    std::vector<ACCEL> accelerators;
    accelerators.reserve(g_shortcutBindings.size() + 2);
    for (const ShortcutBinding& shortcut : g_shortcutBindings)
    {
        if (shortcut.key == 0)
            continue;
        accelerators.push_back(ACCEL{
            AcceleratorFlagsFromShortcut(shortcut.modifiers),
            shortcut.key,
            static_cast<WORD>(shortcut.commandId)
        });
    }

    accelerators.push_back(ACCEL{ FALT, static_cast<WORD>('?'), IDM_ABOUT });
    accelerators.push_back(ACCEL{ FALT, static_cast<WORD>('/'), IDM_ABOUT });

    HACCEL newTable = CreateAcceleratorTableW(accelerators.data(), static_cast<int>(accelerators.size()));
    if (!newTable)
        return;

    if (g_hAccelTable)
        DestroyAcceleratorTable(g_hAccelTable);
    g_hAccelTable = newTable;
}

COLORREF ThemeEditorBack() { return IsDarkTheme() ? kVsCodeEditorBack : kVsCodeLightEditorBack; }
COLORREF ThemeCurrentLineBack() { return IsDarkTheme() ? kVsCodeCurrentLineBack : kVsCodeLightCurrentLineBack; }
COLORREF ThemeEditorText() { return IsDarkTheme() ? kVsCodeText : kVsCodeLightText; }
COLORREF ThemeLineNumber() { return IsDarkTheme() ? kVsCodeLineNumber : kVsCodeLightLineNumber; }
COLORREF ThemeInvisible() { return IsDarkTheme() ? kVsCodeInvisible : kVsCodeLightInvisible; }
COLORREF ThemeSelectionBack() { return IsDarkTheme() ? kVsCodeSelectionBack : kVsCodeLightSelectionBack; }
COLORREF ThemeCaret() { return IsDarkTheme() ? kVsCodeCaret : kVsCodeLightCaret; }
COLORREF ThemeComment() { return IsDarkTheme() ? kVsCodeComment : kVsCodeLightComment; }
COLORREF ThemeKeyword() { return IsDarkTheme() ? kVsCodeKeyword : kVsCodeLightKeyword; }
COLORREF ThemeControlKeyword() { return IsDarkTheme() ? kVsCodeControlKeyword : kVsCodeLightControlKeyword; }
COLORREF ThemeType() { return IsDarkTheme() ? kVsCodeType : kVsCodeLightType; }
COLORREF ThemeFunction() { return IsDarkTheme() ? kVsCodeFunction : kVsCodeLightFunction; }
COLORREF ThemeVariable() { return IsDarkTheme() ? kVsCodeVariable : kVsCodeLightVariable; }
COLORREF ThemeString() { return IsDarkTheme() ? kVsCodeString : kVsCodeLightString; }
COLORREF ThemeNumber() { return IsDarkTheme() ? kVsCodeNumber : kVsCodeLightNumber; }
COLORREF ThemeOperator() { return IsDarkTheme() ? kVsCodeOperator : kVsCodeLightOperator; }
COLORREF ThemeTag() { return IsDarkTheme() ? kVsCodeTag : kVsCodeLightTag; }
COLORREF ThemeCssSelector() { return IsDarkTheme() ? kVsCodeCssSelector : kVsCodeLightCssSelector; }
COLORREF ThemeLink() { return IsDarkTheme() ? kVsCodeLink : kVsCodeLightLink; }
COLORREF ThemeEscape() { return IsDarkTheme() ? kVsCodeEscape : kVsCodeLightEscape; }
COLORREF ThemeError() { return IsDarkTheme() ? kVsCodeError : kVsCodeLightError; }
COLORREF ThemeMarkdownCodeBack() { return IsDarkTheme() ? kVsCodeMarkdownCodeBack : kVsCodeLightMarkdownCodeBack; }
COLORREF ThemeFolderPaneBack() { return IsDarkTheme() ? RGB(37, 37, 38) : kFolderPaneBackColor; }
COLORREF ThemeFolderPaneText() { return IsDarkTheme() ? RGB(204, 204, 204) : kFolderPaneTextColor; }
COLORREF ThemeFolderPaneLine() { return IsDarkTheme() ? RGB(63, 63, 70) : kFolderPaneLineColor; }
COLORREF ThemeFolderTreeSelectionBack() { return RGB(0, 120, 215); }
COLORREF ThemeFolderTreeSelectionText() { return RGB(255, 255, 255); }
COLORREF ThemeTabBarBack() { return IsDarkTheme() ? RGB(37, 37, 38) : kTabBarBackColor; }
COLORREF ThemeTabActiveBack() { return IsDarkTheme() ? kVsCodeEditorBack : kTabActiveBackColor; }
COLORREF ThemeTabInactiveBack() { return IsDarkTheme() ? RGB(45, 45, 45) : kTabInactiveBackColor; }
COLORREF ThemeTabBorder() { return IsDarkTheme() ? RGB(63, 63, 70) : kTabBorderColor; }
COLORREF ThemeTabText() { return IsDarkTheme() ? RGB(212, 212, 212) : kTabTextColor; }
COLORREF ThemeTabClose() { return IsDarkTheme() ? RGB(204, 204, 204) : RGB(86, 96, 108); }
COLORREF ThemeTabCloseHoverBack() { return IsDarkTheme() ? RGB(65, 65, 65) : RGB(209, 216, 224); }
COLORREF ThemeTabCloseHover() { return IsDarkTheme() ? RGB(255, 255, 255) : RGB(32, 42, 56); }
COLORREF ThemeStatusBack() { return IsDarkTheme() ? RGB(37, 37, 38) : RGB(244, 247, 250); }
COLORREF ThemeStatusLine() { return IsDarkTheme() ? RGB(63, 63, 70) : RGB(205, 212, 220); }
COLORREF ThemeStatusText() { return IsDarkTheme() ? RGB(212, 212, 212) : RGB(55, 65, 78); }
COLORREF ThemePopupBack() { return IsDarkTheme() ? RGB(45, 45, 48) : RGB(248, 250, 252); }
COLORREF ThemePopupSurface() { return IsDarkTheme() ? RGB(52, 52, 56) : RGB(255, 255, 255); }
COLORREF ThemePopupText() { return IsDarkTheme() ? RGB(232, 232, 232) : RGB(31, 41, 55); }
COLORREF ThemePopupMutedText() { return IsDarkTheme() ? RGB(170, 170, 170) : RGB(91, 101, 113); }
COLORREF ThemePopupBorder() { return IsDarkTheme() ? RGB(82, 82, 88) : RGB(200, 210, 222); }
COLORREF ThemePopupInputBack() { return IsDarkTheme() ? RGB(38, 38, 42) : RGB(255, 255, 255); }
COLORREF ThemePopupButtonBack(bool pressed) { return pressed ? (IsDarkTheme() ? RGB(70, 70, 76) : RGB(221, 232, 246)) : (IsDarkTheme() ? RGB(58, 58, 63) : RGB(242, 246, 251)); }
COLORREF ThemeAccent() { return RGB(0, 120, 215); }
COLORREF ThemeMenuBack() { return IsDarkTheme() ? RGB(37, 37, 38) : RGB(248, 250, 252); }
COLORREF ThemeMenuHoverBack() { return IsDarkTheme() ? RGB(62, 62, 66) : RGB(229, 241, 251); }
COLORREF ThemeMenuText() { return IsDarkTheme() ? RGB(232, 232, 232) : RGB(31, 41, 55); }
COLORREF ThemeMenuMutedText() { return IsDarkTheme() ? RGB(150, 150, 150) : RGB(120, 128, 140); }
COLORREF ThemeMenuBorder() { return IsDarkTheme() ? RGB(74, 74, 78) : RGB(205, 213, 224); }

sptr_t ScintillaColourAlpha(COLORREF color)
{
    return static_cast<sptr_t>(color | 0xff000000u);
}

using SetPreferredAppModeFn = int(WINAPI*)(int appMode);
using FlushMenuThemesFn = void(WINAPI*)();

void EnableNativeDarkMenuMode(bool dark)
{
    static bool initialized = false;
    static SetPreferredAppModeFn setPreferredAppMode = nullptr;
    static FlushMenuThemesFn flushMenuThemes = nullptr;

    if (!initialized)
    {
        initialized = true;
        HMODULE uxTheme = LoadLibraryW(L"uxtheme.dll");
        if (uxTheme)
        {
            setPreferredAppMode = reinterpret_cast<SetPreferredAppModeFn>(GetProcAddress(uxTheme, MAKEINTRESOURCEA(135)));
            flushMenuThemes = reinterpret_cast<FlushMenuThemesFn>(GetProcAddress(uxTheme, MAKEINTRESOURCEA(136)));
        }
    }

    if (setPreferredAppMode)
        setPreferredAppMode(dark ? 1 : 0);
    if (flushMenuThemes)
        flushMenuThemes();
}

void ApplyWindowChromeTheme(HWND window)
{
    if (!window)
        return;

    const BOOL dark = IsDarkTheme() ? TRUE : FALSE;
    DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
}

void ApplyControlTheme(HWND window)
{
    if (!window)
        return;

    SetWindowTheme(window, IsDarkTheme() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
}

HICON LoadSharedAppIcon(HINSTANCE instance, int width, int height)
{
    return static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_OPENEDIT), IMAGE_ICON,
        width, height, LR_DEFAULTCOLOR | LR_SHARED));
}

void ApplyWindowAppIcons(HWND window)
{
    if (!window || !hInst)
        return;

    SendMessageW(window, WM_SETICON, ICON_BIG,
        reinterpret_cast<LPARAM>(LoadSharedAppIcon(hInst, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON))));
    SendMessageW(window, WM_SETICON, ICON_SMALL,
        reinterpret_cast<LPARAM>(LoadSharedAppIcon(hInst, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON))));
}

std::wstring GetExecutablePath()
{
    wchar_t path[MAX_PATH]{};
    constexpr DWORD pathCapacity = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
    const DWORD length = GetModuleFileNameW(nullptr, path, pathCapacity);
    if (length == 0 || length >= pathCapacity)
        return L"";
    return std::wstring(path, length);
}

std::wstring QuoteCommandPath(const std::wstring& path)
{
    if (path.empty())
        return L"";
    return L"\"" + path + L"\"";
}

void SetTaskbarStringProperty(IPropertyStore* propertyStore, REFPROPERTYKEY key, const wchar_t* value)
{
    if (!propertyStore || !value || !*value)
        return;

    PROPVARIANT propertyValue{};
    if (SUCCEEDED(InitPropVariantFromString(value, &propertyValue)))
    {
        propertyStore->SetValue(key, propertyValue);
        PropVariantClear(&propertyValue);
    }
}

void SetTaskbarStringProperty(IPropertyStore* propertyStore, REFPROPERTYKEY key, const std::wstring& value)
{
    SetTaskbarStringProperty(propertyStore, key, value.c_str());
}

void ConfigureTaskbarProperties(HWND window)
{
    if (!window)
        return;

    IPropertyStore* propertyStore = nullptr;
    if (FAILED(SHGetPropertyStoreForWindow(window, IID_PPV_ARGS(&propertyStore))))
        return;

    SetTaskbarStringProperty(propertyStore, PKEY_AppUserModel_ID, kAppUserModelId);
    SetTaskbarStringProperty(propertyStore, PKEY_AppUserModel_RelaunchDisplayNameResource, kAppDisplayName);

    const std::wstring executablePath = GetExecutablePath();
    if (!executablePath.empty())
        SetTaskbarStringProperty(propertyStore, PKEY_AppUserModel_RelaunchCommand, QuoteCommandPath(executablePath));

    propertyStore->Commit();
    propertyStore->Release();
}

void ApplyFolderPaneTreeTheme(HWND treeWindow)
{
    if (!treeWindow)
        return;

    if (!g_applyingFolderTreeTheme)
    {
        g_applyingFolderTreeTheme = true;
        ApplyControlTheme(treeWindow);
        g_applyingFolderTreeTheme = false;
    }

    TreeView_SetBkColor(treeWindow, ThemeFolderPaneBack());
    TreeView_SetTextColor(treeWindow, ThemeFolderPaneText());
    TreeView_SetLineColor(treeWindow, ThemeFolderPaneLine());
}

void ApplyFolderTreeTheme()
{
    ApplyFolderPaneTreeTheme(g_hOpenFilesTree);
    ApplyFolderPaneTreeTheme(g_hFolderTree);
}

ThemedMenuItem* GetThemedMenuItemData(HMENU menu, int index)
{
    MENUITEMINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_FTYPE | MIIM_DATA;
    if (!GetMenuItemInfoW(menu, index, TRUE, &info))
        return nullptr;
    if ((info.fType & MFT_OWNERDRAW) == 0)
        return nullptr;
    return reinterpret_cast<ThemedMenuItem*>(info.dwItemData);
}

std::wstring GetMenuItemDisplayText(HMENU menu, int index)
{
    wchar_t buffer[256]{};
    const int length = GetMenuStringW(menu, index, buffer, static_cast<int>(sizeof(buffer) / sizeof(buffer[0])), MF_BYPOSITION);
    if (length > 0)
        return std::wstring(buffer, static_cast<size_t>(length));

    if (ThemedMenuItem* item = GetThemedMenuItemData(menu, index))
        return item->text;

    return L"";
}

void ApplyOwnerDrawToMenu(HMENU menu, bool topLevel, std::vector<std::unique_ptr<ThemedMenuItem>>& storage)
{
    if (!menu)
        return;

    MENUINFO menuInfo{};
    menuInfo.cbSize = sizeof(menuInfo);
    menuInfo.fMask = MIM_BACKGROUND;
    menuInfo.hbrBack = GetMenuBackBrush();
    SetMenuInfo(menu, &menuInfo);

    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index)
    {
        MENUITEMINFOW info{};
        info.cbSize = sizeof(info);
        info.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_STATE | MIIM_DATA;
        if (!GetMenuItemInfoW(menu, index, TRUE, &info))
            continue;

        const ThemedMenuItem* existingItem = ((info.fType & MFT_OWNERDRAW) != 0) ?
            reinterpret_cast<const ThemedMenuItem*>(info.dwItemData) : nullptr;
        const std::wstring text = GetMenuItemDisplayText(menu, index);
        auto item = std::make_unique<ThemedMenuItem>();
        item->text = text;
        item->topLevel = topLevel;
        item->hasSubMenu = info.hSubMenu != nullptr;
        item->separator = existingItem ? existingItem->separator : ((info.fType & MFT_SEPARATOR) != 0);
        ThemedMenuItem* itemData = item.get();
        storage.push_back(std::move(item));

        MENUITEMINFOW update{};
        update.cbSize = sizeof(update);
        update.fMask = MIIM_FTYPE | MIIM_DATA;
        update.fType = itemData->separator ? MFT_OWNERDRAW : (MFT_OWNERDRAW | (info.fType & MFT_RADIOCHECK));
        update.dwItemData = reinterpret_cast<ULONG_PTR>(itemData);
        SetMenuItemInfoW(menu, index, TRUE, &update);

        if (info.hSubMenu)
            ApplyOwnerDrawToMenu(info.hSubMenu, false, storage);
    }
}

void ApplyMainMenuTheme()
{
    HMENU menu = GetMenu(hWnd);
    if (!menu)
        return;

    EnableNativeDarkMenuMode(IsDarkTheme());
    std::vector<std::unique_ptr<ThemedMenuItem>> storage;
    ApplyOwnerDrawToMenu(menu, true, storage);
    g_mainMenuItems = std::move(storage);
    DrawMenuBar(hWnd);
}

void ApplyPopupMenuTheme(HMENU menu, std::vector<std::unique_ptr<ThemedMenuItem>>& storage)
{
    EnableNativeDarkMenuMode(IsDarkTheme());
    ApplyOwnerDrawToMenu(menu, false, storage);
}

void SplitMenuItemText(const std::wstring& text, std::wstring& label, std::wstring& accelerator)
{
    label = text;
    accelerator.clear();

    const size_t tab = label.find(L'\t');
    if (tab == std::wstring::npos)
        return;

    accelerator = label.substr(tab + 1);
    label.resize(tab);
}

void DrawThemedMenuItem(const DRAWITEMSTRUCT* drawItem)
{
    if (!drawItem || drawItem->CtlType != ODT_MENU)
        return;

    const ThemedMenuItem* item = reinterpret_cast<const ThemedMenuItem*>(drawItem->itemData);
    if (!item)
        return;

    const bool selected = (drawItem->itemState & ODS_SELECTED) != 0;
    const bool disabled = (drawItem->itemState & ODS_DISABLED) != 0;
    const bool checked = (drawItem->itemState & ODS_CHECKED) != 0;
    const COLORREF back = selected ? ThemeMenuHoverBack() : ThemeMenuBack();
    const COLORREF text = disabled ? ThemeMenuMutedText() : ThemeMenuText();

    RECT rect = drawItem->rcItem;
    HBRUSH backBrush = CreateSolidBrush(back);
    FillRect(drawItem->hDC, &rect, backBrush);
    DeleteObject(backBrush);

    if (item->separator)
    {
        const int y = rect.top + ((rect.bottom - rect.top) / 2);
        HPEN pen = CreatePen(PS_SOLID, 1, ThemeMenuBorder());
        HGDIOBJ oldPen = SelectObject(drawItem->hDC, pen);
        MoveToEx(drawItem->hDC, rect.left + 28, y, nullptr);
        LineTo(drawItem->hDC, rect.right - 8, y);
        SelectObject(drawItem->hDC, oldPen);
        DeleteObject(pen);
        return;
    }

    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(drawItem->hDC, font);
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, text);

    std::wstring label;
    std::wstring accelerator;
    SplitMenuItemText(item->text, label, accelerator);

    if (item->topLevel)
    {
        RECT textRect = rect;
        textRect.left += 4;
        textRect.right -= 4;
        DrawTextW(drawItem->hDC, label.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
    else
    {
        RECT checkRect{ rect.left + 6, rect.top, rect.left + 24, rect.bottom };
        if (checked)
        {
            HPEN checkPen = CreatePen(PS_SOLID, 2, ThemeAccent());
            HGDIOBJ oldPen = SelectObject(drawItem->hDC, checkPen);
            const int cy = checkRect.top + ((checkRect.bottom - checkRect.top) / 2);
            MoveToEx(drawItem->hDC, checkRect.left + 3, cy, nullptr);
            LineTo(drawItem->hDC, checkRect.left + 7, cy + 4);
            LineTo(drawItem->hDC, checkRect.right - 2, cy - 5);
            SelectObject(drawItem->hDC, oldPen);
            DeleteObject(checkPen);
        }

        constexpr int kMenuTextLeft = 30;
        constexpr int kMenuRightPadding = 12;
        constexpr int kMenuAcceleratorGap = 34;

        const int rightReserve = kMenuRightPadding;
        int labelRight = rect.right - rightReserve;
        if (!accelerator.empty())
        {
            SIZE acceleratorSize{};
            GetTextExtentPoint32W(drawItem->hDC, accelerator.c_str(),
                static_cast<int>(accelerator.size()), &acceleratorSize);
            labelRight -= acceleratorSize.cx + kMenuAcceleratorGap;
        }

        const int textLeft = static_cast<int>(rect.left) + kMenuTextLeft;
        RECT textRect{ textLeft, rect.top, (std::max)(textLeft + 1, labelRight), rect.bottom };
        DrawTextW(drawItem->hDC, label.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (!accelerator.empty())
        {
            RECT acceleratorRect{ rect.left + kMenuTextLeft, rect.top, rect.right - rightReserve, rect.bottom };
            DrawTextW(drawItem->hDC, accelerator.c_str(), -1, &acceleratorRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }

    SelectObject(drawItem->hDC, oldFont);
}

void MeasureThemedMenuItem(MEASUREITEMSTRUCT* measureItem)
{
    if (!measureItem || measureItem->CtlType != ODT_MENU)
        return;

    const ThemedMenuItem* item = reinterpret_cast<const ThemedMenuItem*>(measureItem->itemData);
    if (!item)
        return;

    if (item->separator)
    {
        measureItem->itemWidth = 180;
        measureItem->itemHeight = 9;
        return;
    }

    HDC hdc = GetDC(hWnd);
    HGDIOBJ oldFont = hdc ? SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT)) : nullptr;
    std::wstring label;
    std::wstring accelerator;
    SplitMenuItemText(item->text, label, accelerator);
    SIZE labelSize{};
    SIZE acceleratorSize{};
    if (hdc)
    {
        GetTextExtentPoint32W(hdc, label.c_str(), static_cast<int>(label.size()), &labelSize);
        if (!accelerator.empty())
            GetTextExtentPoint32W(hdc, accelerator.c_str(), static_cast<int>(accelerator.size()), &acceleratorSize);
    }

    if (hdc && oldFont)
        SelectObject(hdc, oldFont);
    if (hdc)
        ReleaseDC(hWnd, hdc);

    if (item->topLevel)
    {
        measureItem->itemWidth = static_cast<UINT>((std::max)(34L, labelSize.cx + 8));
        measureItem->itemHeight = 22;
    }
    else
    {
        constexpr LONG kMenuTextLeft = 30;
        constexpr LONG kMenuRightPadding = 12;
        constexpr LONG kMenuAcceleratorGap = 34;

        LONG width = kMenuTextLeft + labelSize.cx;
        if (!accelerator.empty())
            width += kMenuAcceleratorGap + acceleratorSize.cx;
        width += kMenuRightPadding;

        measureItem->itemWidth = static_cast<UINT>((std::max)(180L, width));
        measureItem->itemHeight = 26;
    }
}

void ApplyEditorViewOptions()
{
    Sci(SCI_SETVIEWWS, g_showSpaceAndTab ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
    Sci(SCI_SETVIEWEOL, g_showEndOfLine ? TRUE : FALSE, 0);
    Sci(SCI_SETWRAPMODE, g_wordWrapEnabled ? SC_WRAP_WORD : SC_WRAP_NONE, 0);
}

void SetRepresentation(const char* character, const char* representation, int appearance)
{
    Sci(SCI_SETREPRESENTATION, reinterpret_cast<uptr_t>(character), reinterpret_cast<sptr_t>(representation));
    Sci(SCI_SETREPRESENTATIONAPPEARANCE, reinterpret_cast<uptr_t>(character), appearance);
}

void ConfigureControlCharacterRepresentations()
{
    SetRepresentation("\r\n", "CRLF", SC_REPRESENTATION_PLAIN);
    SetRepresentation("\r", "CR", SC_REPRESENTATION_PLAIN);
    SetRepresentation("\n", "LF", SC_REPRESENTATION_PLAIN);
}

void ConfigureWordHighlightIndicator()
{
    Sci(SCI_INDICSETSTYLE, kWordHighlightIndicator, INDIC_ROUNDBOX);
    Sci(SCI_INDICSETFORE, kWordHighlightIndicator, IsDarkTheme() ? RGB(215, 168, 60) : RGB(255, 196, 45));
    Sci(SCI_INDICSETALPHA, kWordHighlightIndicator, IsDarkTheme() ? 80 : 90);
    Sci(SCI_INDICSETOUTLINEALPHA, kWordHighlightIndicator, IsDarkTheme() ? 150 : 170);
    Sci(SCI_INDICSETUNDER, kWordHighlightIndicator, TRUE);
}

void ConfigureSearchMarkIndicator()
{
    Sci(SCI_INDICSETSTYLE, kSearchMarkIndicator, INDIC_FULLBOX);
    Sci(SCI_INDICSETFORE, kSearchMarkIndicator, RGB(255, 242, 153));
    Sci(SCI_INDICSETALPHA, kSearchMarkIndicator, IsDarkTheme() ? 90 : 70);
    Sci(SCI_INDICSETOUTLINEALPHA, kSearchMarkIndicator, IsDarkTheme() ? 110 : 90);
    Sci(SCI_INDICSETUNDER, kSearchMarkIndicator, TRUE);
}

void SetMenuItemChecked(HMENU hMenu, UINT commandId, bool checked)
{
    if (!hMenu)
        return;

    CheckMenuItem(hMenu, commandId, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
}

void SetMenuText(HMENU menu, UINT commandId, const wchar_t* text)
{
    if (!menu)
        return;

    const std::wstring displayText = StripMenuMnemonic(text ? text : L"");
    ModifyMenuW(menu, commandId, MF_BYCOMMAND | MF_STRING, commandId, displayText.c_str());
}

bool MenuHasCommand(HMENU menu, UINT commandId)
{
    if (!menu)
        return false;

    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index)
    {
        if (GetMenuItemID(menu, index) == commandId)
            return true;
    }
    return false;
}

int MenuCommandIndex(HMENU menu, UINT commandId)
{
    if (!menu)
        return -1;

    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index)
    {
        if (GetMenuItemID(menu, index) == commandId)
            return index;
    }
    return -1;
}

bool IsMenuSeparatorAt(HMENU menu, int index)
{
    if (!menu || index < 0 || index >= GetMenuItemCount(menu))
        return false;

    MENUITEMINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = MIIM_FTYPE;
    return GetMenuItemInfoW(menu, index, TRUE, &info) &&
        (info.fType & MFT_SEPARATOR) != 0;
}

void EnsureEditColumnEditorCommand(HMENU editMenu)
{
    if (!editMenu || MenuHasCommand(editMenu, IDM_EDIT_COLUMN_EDITOR))
        return;

    int insertIndex = GetMenuItemCount(editMenu);
    const int searchIndex = MenuCommandIndex(editMenu, IDM_SEARCH_FIND);
    if (searchIndex >= 0)
        insertIndex = IsMenuSeparatorAt(editMenu, searchIndex - 1) ? searchIndex - 1 : searchIndex;

    const std::wstring text = StripMenuMnemonic(UiText(L"\u5217\u7F16\u8F91\u5668(&L)...", L"Column Editor(&L)..."));
    InsertMenuW(editMenu, insertIndex, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
    InsertMenuW(editMenu, insertIndex + 1, MF_BYPOSITION | MF_STRING, IDM_EDIT_COLUMN_EDITOR, text.c_str());
}

void EnsureEditSearchCommands(HMENU editMenu)
{
    if (!editMenu || MenuHasCommand(editMenu, IDM_SEARCH_FIND))
        return;

    AppendMenuW(editMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(editMenu, MF_STRING, IDM_SEARCH_FIND,
        MenuLabelWithShortcut(L"\u67E5\u627E(&F)", L"Find(&F)", IDM_SEARCH_FIND).c_str());
    AppendMenuW(editMenu, MF_STRING, IDM_SEARCH_REPLACE,
        MenuLabelWithShortcut(L"\u66FF\u6362(&R)", L"Replace(&R)", IDM_SEARCH_REPLACE).c_str());
}

const std::array<LanguageMenuLabel, 28> kLanguageMenuLabels = { {
    { IDM_LANG_TEXT, L"\u7EAF\u6587\u672C(&T)", L"Plain Text(&T)" },
    { IDM_LANG_CPP, L"C/C++(&C)", L"C/C++(&C)" },
    { IDM_LANG_CSHARP, L"C#(&S)", L"C#(&S)" },
    { IDM_LANG_JAVA, L"Java(&J)", L"Java(&J)" },
    { IDM_LANG_JAVASCRIPT, L"JavaScript(&A)", L"JavaScript(&A)" },
    { IDM_LANG_TYPESCRIPT, L"TypeScript(&Y)", L"TypeScript(&Y)" },
    { IDM_LANG_PYTHON, L"Python(&P)", L"Python(&P)" },
    { IDM_LANG_HTML, L"HTML(&H)", L"HTML(&H)" },
    { IDM_LANG_XML, L"XML(&X)", L"XML(&X)" },
    { IDM_LANG_CSS, L"CSS(&C)", L"CSS(&C)" },
    { IDM_LANG_JSON, L"JSON(&N)", L"JSON(&N)" },
    { IDM_LANG_SQL, L"SQL(&Q)", L"SQL(&Q)" },
    { IDM_LANG_BASH, L"Bash/Shell(&B)", L"Bash/Shell(&B)" },
    { IDM_LANG_POWERSHELL, L"PowerShell(&O)", L"PowerShell(&O)" },
    { IDM_LANG_RUST, L"Rust(&R)", L"Rust(&R)" },
    { IDM_LANG_LUA, L"Lua(&U)", L"Lua(&U)" },
    { IDM_LANG_RUBY, L"Ruby(&G)", L"Ruby(&G)" },
    { IDM_LANG_MARKDOWN, L"Markdown(&M)", L"Markdown(&M)" },
    { IDM_LANG_YAML, L"YAML(&L)", L"YAML(&L)" },
    { IDM_LANG_TOML, L"TOML(&K)", L"TOML(&K)" },
    { IDM_LANG_PROPERTIES, L"INI/Properties(&I)", L"INI/Properties(&I)" },
    { IDM_LANG_MAKEFILE, L"Makefile(&F)", L"Makefile(&F)" },
    { IDM_LANG_DIFF, L"Diff/Patch(&D)", L"Diff/Patch(&D)" },
    { IDM_LANG_BATCH, L"Batch/CMD(&B)", L"Batch/CMD(&B)" },
    { IDM_LANG_ZIG, L"Zig(&Z)", L"Zig(&Z)" },
    { IDM_LANG_NIM, L"Nim(&N)", L"Nim(&N)" },
    { IDM_LANG_REGISTRY, L"Registry(&E)", L"Registry(&E)" },
    { IDM_LANG_INNO, L"Inno Setup(&P)", L"Inno Setup(&P)" },
} };

const std::array<LanguageMenuGroup, 4> kLanguageMenuGroups = { {
    { L"A-F", L'A', L'F' },
    { L"G-M", L'G', L'M' },
    { L"N-S", L'N', L'S' },
    { L"T-Z", L'T', L'Z' },
} };

std::wstring LanguageMenuDisplayText(const LanguageMenuLabel& label)
{
    return StripMenuMnemonic(UiText(label.chinese, label.english));
}

std::wstring LanguageMenuSortText(const LanguageMenuLabel& label)
{
    return StripMenuMnemonic(label.english);
}

wchar_t LanguageMenuSortLetter(const LanguageMenuLabel& label)
{
    const std::wstring sortText = LanguageMenuSortText(label);
    for (wchar_t ch : sortText)
    {
        if (iswalpha(ch))
            return static_cast<wchar_t>(towupper(ch));
    }
    return L'#';
}

bool LanguageBelongsToGroup(const LanguageMenuLabel& label, const LanguageMenuGroup& group)
{
    const wchar_t firstLetter = LanguageMenuSortLetter(label);
    return firstLetter >= group.firstLetter && firstLetter <= group.lastLetter;
}

void ClearMenuItems(HMENU menu)
{
    if (!menu)
        return;

    while (GetMenuItemCount(menu) > 0)
        DeleteMenu(menu, 0, MF_BYPOSITION);
}

std::vector<const LanguageMenuLabel*> GetSortedLanguageMenuLabels()
{
    std::vector<const LanguageMenuLabel*> labels;
    labels.reserve(kLanguageMenuLabels.size());
    for (const LanguageMenuLabel& label : kLanguageMenuLabels)
        labels.push_back(&label);

    std::sort(labels.begin(), labels.end(), [](const LanguageMenuLabel* left, const LanguageMenuLabel* right) {
        const std::wstring leftText = LanguageMenuSortText(*left);
        const std::wstring rightText = LanguageMenuSortText(*right);
        return CompareStringOrdinal(leftText.c_str(), -1, rightText.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    });

    return labels;
}

void RebuildLanguageMenuItems(HMENU syntaxMenu)
{
    if (!syntaxMenu)
        return;

    ClearMenuItems(syntaxMenu);

    const std::vector<const LanguageMenuLabel*> labels = GetSortedLanguageMenuLabels();
    for (const LanguageMenuGroup& group : kLanguageMenuGroups)
    {
        HMENU groupMenu = CreatePopupMenu();
        if (!groupMenu)
            continue;

        for (const LanguageMenuLabel* label : labels)
        {
            if (!LanguageBelongsToGroup(*label, group))
                continue;

            const std::wstring displayText = LanguageMenuDisplayText(*label);
            AppendMenuW(groupMenu, MF_STRING, label->commandId,
                displayText.c_str());
        }

        if (GetMenuItemCount(groupMenu) == 0)
        {
            DestroyMenu(groupMenu);
            continue;
        }

        AppendMenuW(syntaxMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(groupMenu), group.title);
    }
}

int FindSubMenuIndex(HMENU menu, HMENU submenu)
{
    if (!menu || !submenu)
        return -1;

    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index)
    {
        if (GetSubMenu(menu, index) == submenu)
            return index;
    }
    return -1;
}

HMENU FindMenuContainingCommand(HMENU menu, UINT commandId)
{
    if (!menu)
        return nullptr;

    const int count = GetMenuItemCount(menu);
    for (int index = 0; index < count; ++index)
    {
        if (GetMenuItemID(menu, index) == commandId)
            return menu;

        HMENU submenu = GetSubMenu(menu, index);
        HMENU found = FindMenuContainingCommand(submenu, commandId);
        if (found)
            return found;
    }
    return nullptr;
}

void SetTopMenuText(HMENU menu, int index, const wchar_t* text)
{
    HMENU submenu = GetSubMenu(menu, index);
    if (!submenu)
        return;

    const std::wstring displayText = StripMenuMnemonic(text ? text : L"");
    ModifyMenuW(menu, index, MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(submenu), displayText.c_str());
}

void EnsureSettingsMenu(HMENU menu)
{
    if (!menu)
        return;

    if (!g_hSettingsMenu || FindSubMenuIndex(menu, g_hSettingsMenu) < 0)
        g_hSettingsMenu = FindMenuContainingCommand(menu, IDM_SETTINGS_OPEN);

    if (!g_hSettingsMenu)
    {
        g_hSettingsMenu = CreatePopupMenu();
        if (!g_hSettingsMenu)
            return;

        const int insertIndex = (std::max)(0, GetMenuItemCount(menu) - 1);
        const std::wstring settingsText = StripMenuMnemonic(UiText(L"\u8BBE\u7F6E(&T)", L"Settings(&T)"));
        InsertMenuW(menu, insertIndex, MF_BYPOSITION | MF_POPUP,
            reinterpret_cast<UINT_PTR>(g_hSettingsMenu),
            settingsText.c_str());
    }

    if (GetMenuItemCount(g_hSettingsMenu) == 0)
    {
        const std::wstring preferencesText = StripMenuMnemonic(UiText(L"\u8BBE\u7F6E(&P)...", L"Preferences(&P)..."));
        AppendMenuW(g_hSettingsMenu, MF_STRING, IDM_SETTINGS_OPEN,
            preferencesText.c_str());
    }
    else
    {
        const std::wstring preferencesText = StripMenuMnemonic(UiText(L"\u8BBE\u7F6E(&P)...", L"Preferences(&P)..."));
        ModifyMenuW(g_hSettingsMenu, IDM_SETTINGS_OPEN, MF_BYCOMMAND | MF_STRING,
            IDM_SETTINGS_OPEN, preferencesText.c_str());
    }

    const int settingsIndex = FindSubMenuIndex(menu, g_hSettingsMenu);
    if (settingsIndex >= 0)
    {
        const std::wstring settingsText = StripMenuMnemonic(UiText(L"\u8BBE\u7F6E(&T)", L"Settings(&T)"));
        ModifyMenuW(menu, settingsIndex, MF_BYPOSITION | MF_POPUP,
            reinterpret_cast<UINT_PTR>(g_hSettingsMenu),
            settingsText.c_str());
    }
}

void UpdateMainMenuText()
{
    HMENU menu = GetMenu(hWnd);
    if (!menu)
        return;

    EnsureSettingsMenu(menu);

    SetTopMenuText(menu, 0, UiText(L"\u6587\u4EF6(&F)", L"File(&F)"));
    SetTopMenuText(menu, 1, UiText(L"\u7F16\u8F91(&E)", L"Edit(&E)"));
    SetTopMenuText(menu, 2, UiText(L"\u641C\u7D22(&S)", L"Search(&S)"));
    SetTopMenuText(menu, kViewMenuIndex, UiText(L"\u67E5\u770B(&V)", L"View(&V)"));
    SetTopMenuText(menu, kLanguageMenuIndex, UiText(L"\u8BED\u6CD5(&L)", L"Syntax(&L)"));

    const int helpIndex = GetMenuItemCount(menu) - 1;
    SetTopMenuText(menu, helpIndex, UiText(L"\u5E2E\u52A9(&H)", L"Help(&H)"));

    HMENU fileMenu = GetSubMenu(menu, 0);
    SetMenuText(fileMenu, IDM_FILE_NEW, MenuLabelWithShortcut(L"\u65B0\u5EFA(&N)", L"New(&N)", IDM_FILE_NEW).c_str());
    SetMenuText(fileMenu, IDM_FILE_OPEN, MenuLabelWithShortcut(L"\u6253\u5F00\u6587\u4EF6(&O)", L"Open File(&O)", IDM_FILE_OPEN).c_str());
    SetMenuText(fileMenu, IDM_FILE_OPEN_FOLDER, MenuLabelWithShortcut(L"\u6253\u5F00\u6587\u4EF6\u5939(&F)", L"Open Folder(&F)", IDM_FILE_OPEN_FOLDER).c_str());
    SetMenuText(fileMenu, IDM_FILE_SAVE, MenuLabelWithShortcut(L"\u4FDD\u5B58(&S)", L"Save(&S)", IDM_FILE_SAVE).c_str());
    SetMenuText(fileMenu, IDM_FILE_SAVE_AS, MenuLabelWithShortcut(L"\u53E6\u5B58\u4E3A(&A)", L"Save As(&A)", IDM_FILE_SAVE_AS).c_str());
    SetMenuText(fileMenu, IDM_FILE_CLOSE_FOLDER, UiText(L"\u5173\u95ED\u6587\u4EF6\u5939(&C)", L"Close Folder(&C)"));
    SetMenuText(fileMenu, IDM_EXIT, UiText(L"\u9000\u51FA(&X)", L"Exit(&X)"));

    HMENU editMenu = GetSubMenu(menu, 1);
    EnsureEditColumnEditorCommand(editMenu);
    EnsureEditSearchCommands(editMenu);
    SetMenuText(editMenu, IDM_EDIT_UNDO, MenuLabelWithShortcut(L"\u64A4\u9500(&U)", L"Undo(&U)", IDM_EDIT_UNDO).c_str());
    SetMenuText(editMenu, IDM_EDIT_REDO, MenuLabelWithShortcut(L"\u91CD\u505A(&R)", L"Redo(&R)", IDM_EDIT_REDO).c_str());
    SetMenuText(editMenu, IDM_EDIT_CUT, MenuLabelWithShortcut(L"\u526A\u5207(&T)", L"Cut(&T)", IDM_EDIT_CUT).c_str());
    SetMenuText(editMenu, IDM_EDIT_COPY, MenuLabelWithShortcut(L"\u590D\u5236(&C)", L"Copy(&C)", IDM_EDIT_COPY).c_str());
    SetMenuText(editMenu, IDM_EDIT_PASTE, MenuLabelWithShortcut(L"\u7C98\u8D34(&P)", L"Paste(&P)", IDM_EDIT_PASTE).c_str());
    SetMenuText(editMenu, IDM_EDIT_DELETE, MenuLabelWithShortcut(L"\u5220\u9664(&D)", L"Delete(&D)", IDM_EDIT_DELETE).c_str());
    SetMenuText(editMenu, IDM_EDIT_SELECT_ALL, MenuLabelWithShortcut(L"\u5168\u9009(&A)", L"Select All(&A)", IDM_EDIT_SELECT_ALL).c_str());
    SetMenuText(editMenu, IDM_EDIT_COLUMN_EDITOR, UiText(L"\u5217\u7F16\u8F91\u5668(&L)...", L"Column Editor(&L)..."));
    SetMenuText(editMenu, IDM_SEARCH_FIND, MenuLabelWithShortcut(L"\u67E5\u627E(&F)", L"Find(&F)", IDM_SEARCH_FIND).c_str());
    SetMenuText(editMenu, IDM_SEARCH_REPLACE, MenuLabelWithShortcut(L"\u66FF\u6362(&R)", L"Replace(&R)", IDM_SEARCH_REPLACE).c_str());

    HMENU searchMenu = GetSubMenu(menu, 2);
    SetMenuText(searchMenu, IDM_SEARCH_FIND, MenuLabelWithShortcut(L"\u67E5\u627E(&F)", L"Find(&F)", IDM_SEARCH_FIND).c_str());
    SetMenuText(searchMenu, IDM_SEARCH_FIND_NEXT, MenuLabelWithShortcut(L"\u67E5\u627E\u4E0B\u4E00\u4E2A(&N)", L"Find Next(&N)", IDM_SEARCH_FIND_NEXT).c_str());
    SetMenuText(searchMenu, IDM_SEARCH_FIND_PREVIOUS, MenuLabelWithShortcut(L"\u67E5\u627E\u4E0A\u4E00\u4E2A(&P)", L"Find Previous(&P)", IDM_SEARCH_FIND_PREVIOUS).c_str());
    SetMenuText(searchMenu, IDM_SEARCH_REPLACE, MenuLabelWithShortcut(L"\u66FF\u6362(&R)", L"Replace(&R)", IDM_SEARCH_REPLACE).c_str());

    HMENU viewMenu = GetSubMenu(menu, kViewMenuIndex);
    HMENU symbolMenu = viewMenu ? GetSubMenu(viewMenu, 0) : nullptr;
    if (viewMenu && symbolMenu)
    {
        const std::wstring symbolMenuText = StripMenuMnemonic(UiText(L"\u67E5\u770B\u7B26\u53F7(&S)", L"Symbols(&S)"));
        ModifyMenuW(viewMenu, 0, MF_BYPOSITION | MF_POPUP,
            reinterpret_cast<UINT_PTR>(symbolMenu),
            symbolMenuText.c_str());
    }
    SetMenuText(symbolMenu, IDM_VIEW_SHOW_SPACE_TAB, UiText(L"\u663E\u793A\u7A7A\u683C\u548C\u5236\u8868\u7B26(&S)", L"Show Space and Tab(&S)"));
    SetMenuText(symbolMenu, IDM_VIEW_SHOW_EOL, UiText(L"\u663E\u793A\u884C\u5C3E\u7B26(&E)", L"Show End of Line(&E)"));
    SetMenuText(symbolMenu, IDM_VIEW_SHOW_ALL_CHARS, UiText(L"\u663E\u793A\u6240\u6709\u5B57\u7B26(&A)", L"Show All Characters(&A)"));
    SetMenuText(viewMenu, IDM_VIEW_WORD_WRAP, UiText(L"\u81EA\u52A8\u6362\u884C(&W)", L"Word Wrap(&W)"));
    SetMenuText(viewMenu, IDM_VIEW_TOGGLE_FOLDER, UiText(L"\u663E\u793A/\u9690\u85CF\u6587\u4EF6\u5939(&F)", L"Show/Hide Folder(&F)"));

    HMENU syntaxMenu = GetSubMenu(menu, kLanguageMenuIndex);
    RebuildLanguageMenuItems(syntaxMenu);

    HMENU helpMenu = GetSubMenu(menu, helpIndex);
    SetMenuText(helpMenu, IDM_ABOUT, UiText(L"\u5173\u4E8E(&A)...", L"About(&A)..."));

    UpdateViewMenuCheck();
    UpdateLanguageMenuCheck();
    ApplyMainMenuTheme();
}

void UpdateViewMenuCheck()
{
    HMENU hMenu = GetMenu(hWnd);
    if (!hMenu)
        return;

    HMENU hViewMenu = GetSubMenu(hMenu, kViewMenuIndex);
    if (!hViewMenu)
        return;

    HMENU hSymbolMenu = GetSubMenu(hViewMenu, 0);
    SetMenuItemChecked(hSymbolMenu, IDM_VIEW_SHOW_SPACE_TAB, g_showSpaceAndTab);
    SetMenuItemChecked(hSymbolMenu, IDM_VIEW_SHOW_EOL, g_showEndOfLine);
    SetMenuItemChecked(hSymbolMenu, IDM_VIEW_SHOW_ALL_CHARS, g_showSpaceAndTab && g_showEndOfLine);
    SetMenuItemChecked(hViewMenu, IDM_VIEW_WORD_WRAP, g_wordWrapEnabled);
    SetMenuItemChecked(hViewMenu, IDM_VIEW_TOGGLE_FOLDER, g_folderPaneVisible);
    DrawMenuBar(hWnd);
}

void ToggleShowSpaceAndTab()
{
    g_showSpaceAndTab = !g_showSpaceAndTab;
    ApplyEditorViewOptions();
    UpdateViewMenuCheck();
}

void ToggleShowEndOfLine()
{
    g_showEndOfLine = !g_showEndOfLine;
    ApplyEditorViewOptions();
    UpdateViewMenuCheck();
}

void ToggleShowAllCharacters()
{
    const bool showAll = !(g_showSpaceAndTab && g_showEndOfLine);
    g_showSpaceAndTab = showAll;
    g_showEndOfLine = showAll;
    ApplyEditorViewOptions();
    UpdateViewMenuCheck();
}

void ToggleWordWrap()
{
    g_wordWrapEnabled = !g_wordWrapEnabled;
    ApplyEditorViewOptions();
    UpdateViewMenuCheck();
}

void SetStyleFore(std::initializer_list<int> styles, COLORREF color)
{
    for (int style : styles)
        Sci(SCI_STYLESETFORE, style, color);
}

void SetStyleBack(std::initializer_list<int> styles, COLORREF color)
{
    for (int style : styles)
        Sci(SCI_STYLESETBACK, style, color);
}

void SetStyleBold(std::initializer_list<int> styles, bool bold)
{
    for (int style : styles)
        Sci(SCI_STYLESETBOLD, style, bold ? 1 : 0);
}

void SetStyleItalic(std::initializer_list<int> styles, bool italic)
{
    for (int style : styles)
        Sci(SCI_STYLESETITALIC, style, italic ? 1 : 0);
}

const LanguageDefinition* FindLanguageDefinition(int commandId)
{
    for (const LanguageDefinition& language : kLanguages)
    {
        if (language.commandId == commandId)
            return &language;
    }
    return nullptr;
}

bool IsLanguageCommand(int commandId)
{
    return FindLanguageDefinition(commandId) != nullptr;
}

void ClearKeywordLists()
{
    for (uptr_t index = 0; index < 8; ++index)
        Sci(SCI_SETKEYWORDS, index, reinterpret_cast<sptr_t>(""));
}

void SetEditorProperty(const char* name, const char* value)
{
    Sci(SCI_SETPROPERTY, reinterpret_cast<uptr_t>(name), reinterpret_cast<sptr_t>(value));
}

bool SupportsCodeFolding(SyntaxFamily family)
{
    return family != SyntaxFamily::Plain;
}

void ConfigureFoldMarkers()
{
    const COLORREF markerLine = IsDarkTheme() ? RGB(128, 128, 128) : RGB(90, 105, 122);
    const COLORREF markerFill = ThemeEditorBack();

    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_BOXMINUS);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_BOXPLUS);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_VLINE);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_LCORNER);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_BOXPLUSCONNECTED);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED);
    Sci(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER);

    for (int marker = SC_MARKNUM_FOLDEREND; marker <= SC_MARKNUM_FOLDEROPEN; ++marker)
    {
        Sci(SCI_MARKERSETFORE, marker, markerLine);
        Sci(SCI_MARKERSETBACK, marker, markerFill);
        Sci(SCI_MARKERSETBACKSELECTED, marker, markerFill);
        Sci(SCI_MARKERSETALPHA, marker, SC_ALPHA_OPAQUE);
        Sci(SCI_MARKERSETSTROKEWIDTH, marker, 100);
    }
}

void ConfigureCodeFolding(SyntaxFamily family)
{
    const bool enabled = SupportsCodeFolding(family);
    SetEditorProperty("fold", enabled ? "1" : "0");
    SetEditorProperty("fold.compact", enabled ? "1" : "0");
    SetEditorProperty("fold.html", enabled ? "1" : "0");
    SetEditorProperty("fold.comment", enabled ? "1" : "0");

    ConfigureFoldMarkers();
    Sci(SCI_SETMARGINTYPEN, kFoldMargin, SC_MARGIN_SYMBOL);
    Sci(SCI_SETMARGINMASKN, kFoldMargin, SC_MASK_FOLDERS);
    Sci(SCI_SETMARGINWIDTHN, kFoldMargin, enabled ? 16 : 0);
    Sci(SCI_SETMARGINSENSITIVEN, kFoldMargin, enabled ? TRUE : FALSE);
    Sci(SCI_SETMARGINCURSORN, kFoldMargin, SC_CURSORARROW);
    Sci(SCI_SETMARGINBACKN, kFoldMargin, ThemeEditorBack());
    Sci(SCI_SETFOLDMARGINCOLOUR, TRUE, ThemeEditorBack());
    Sci(SCI_SETFOLDMARGINHICOLOUR, TRUE, ThemeEditorBack());
    Sci(SCI_SETELEMENTCOLOUR, SC_ELEMENT_FOLD_LINE,
        ScintillaColourAlpha(IsDarkTheme() ? RGB(82, 82, 88) : RGB(200, 210, 222)));
    Sci(SCI_SETFOLDFLAGS, enabled ? SC_FOLDFLAG_LINEAFTER_CONTRACTED : SC_FOLDFLAG_NONE);
    Sci(SCI_SETAUTOMATICFOLD, enabled ? (SC_AUTOMATICFOLD_SHOW | SC_AUTOMATICFOLD_CHANGE) : SC_AUTOMATICFOLD_NONE);
}

void ApplyBaseEditorStyles()
{
    Sci(SCI_STYLESETBACK, STYLE_DEFAULT, ThemeEditorBack());
    Sci(SCI_STYLESETFORE, STYLE_DEFAULT, ThemeEditorText());
    Sci(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<sptr_t>("Consolas"));
    Sci(SCI_STYLESETSIZE, STYLE_DEFAULT, kEditorFontSize);
    Sci(SCI_STYLECLEARALL, 0, 0);

    Sci(SCI_STYLESETFONT, STYLE_CONTROLCHAR, reinterpret_cast<sptr_t>("Consolas"));
    Sci(SCI_STYLESETSIZE, STYLE_CONTROLCHAR, kEditorFontSize);
    Sci(SCI_STYLESETFORE, STYLE_CONTROLCHAR, ThemeInvisible());
    Sci(SCI_STYLESETBACK, STYLE_CONTROLCHAR, ThemeEditorBack());

    Sci(SCI_STYLESETBACK, STYLE_LINENUMBER, ThemeEditorBack());
    Sci(SCI_STYLESETFORE, STYLE_LINENUMBER, ThemeLineNumber());
    Sci(SCI_STYLESETBACK, STYLE_INDENTGUIDE, ThemeEditorBack());
    Sci(SCI_STYLESETFORE, STYLE_INDENTGUIDE, ThemeInvisible());
    Sci(SCI_STYLESETBACK, STYLE_BRACELIGHT, RGB(0, 122, 204));
    Sci(SCI_STYLESETFORE, STYLE_BRACELIGHT, RGB(255, 255, 255));
    Sci(SCI_STYLESETBOLD, STYLE_BRACELIGHT, 1);
    Sci(SCI_STYLESETBACK, STYLE_BRACEBAD, ThemeEditorBack());
    Sci(SCI_STYLESETFORE, STYLE_BRACEBAD, ThemeError());
    Sci(SCI_SETMARGINTYPEN, kLineNumberMargin, SC_MARGIN_NUMBER);
    Sci(SCI_SETMARGINWIDTHN, kLineNumberMargin, 56);
    Sci(SCI_SETMARGINBACKN, kLineNumberMargin, ThemeEditorBack());
    Sci(SCI_SETTABWIDTH, 4, 0);
    Sci(SCI_SETINDENT, 4, 0);
    Sci(SCI_SETUSETABS, 0, 0);
    Sci(SCI_SETINDENTATIONGUIDES, SC_IV_LOOKBOTH, 0);
    Sci(SCI_SETCARETFORE, ThemeCaret(), 0);
    Sci(SCI_SETCARETWIDTH, 2, 0);
    Sci(SCI_SETCARETLINEVISIBLE, TRUE, 0);
    Sci(SCI_SETCARETLINEBACK, ThemeCurrentLineBack(), 0);
    Sci(SCI_SETSELFORE, TRUE, ThemeEditorText());
    Sci(SCI_SETSELBACK, TRUE, ThemeSelectionBack());
    Sci(SCI_SETWHITESPACEFORE, TRUE, ThemeInvisible());
    Sci(SCI_SETWHITESPACEBACK, TRUE, ThemeEditorBack());
    Sci(SCI_SETWHITESPACESIZE, 2, 0);
    Sci(SCI_SETEDGECOLOUR, ThemeInvisible(), 0);
    ApplyEditorViewOptions();
    ConfigureWordHighlightIndicator();
    ConfigureSearchMarkIndicator();
}

void ApplyCppLikeStyles()
{
    SetStyleFore({ SCE_C_COMMENT, SCE_C_COMMENTLINE, SCE_C_COMMENTDOC, SCE_C_COMMENTLINEDOC,
        SCE_C_COMMENTDOCKEYWORD, SCE_C_PREPROCESSORCOMMENT, SCE_C_PREPROCESSORCOMMENTDOC, SCE_C_TASKMARKER }, ThemeComment());
    SetStyleFore({ SCE_C_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_C_WORD2, SCE_C_GLOBALCLASS, SCE_C_UUID }, ThemeType());
    SetStyleFore({ SCE_C_IDENTIFIER }, ThemeEditorText());
    SetStyleFore({ SCE_C_STRING, SCE_C_CHARACTER, SCE_C_STRINGRAW, SCE_C_VERBATIM,
        SCE_C_HASHQUOTEDSTRING, SCE_C_USERLITERAL }, ThemeString());
    SetStyleFore({ SCE_C_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_C_PREPROCESSOR }, ThemeControlKeyword());
    SetStyleFore({ SCE_C_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_C_REGEX, SCE_C_ESCAPESEQUENCE }, ThemeEscape());
    SetStyleFore({ SCE_C_COMMENTDOCKEYWORDERROR, SCE_C_STRINGEOL }, ThemeError());
}

void ApplyPythonStyles()
{
    SetStyleFore({ SCE_P_COMMENTLINE, SCE_P_COMMENTBLOCK }, ThemeComment());
    SetStyleFore({ SCE_P_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_P_WORD2, SCE_P_ATTRIBUTE }, ThemeVariable());
    SetStyleFore({ SCE_P_STRING, SCE_P_CHARACTER, SCE_P_TRIPLE, SCE_P_TRIPLEDOUBLE,
        SCE_P_FSTRING, SCE_P_FCHARACTER, SCE_P_FTRIPLE, SCE_P_FTRIPLEDOUBLE }, ThemeString());
    SetStyleFore({ SCE_P_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_P_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_P_CLASSNAME }, ThemeType());
    SetStyleFore({ SCE_P_DEFNAME }, ThemeFunction());
    SetStyleFore({ SCE_P_DECORATOR }, ThemeControlKeyword());
    SetStyleFore({ SCE_P_STRINGEOL }, ThemeError());
}

void ApplyHyperTextStyles()
{
    SetStyleFore({ SCE_H_TAG, SCE_H_TAGEND, SCE_H_XMLSTART, SCE_H_XMLEND, SCE_H_SCRIPT }, ThemeTag());
    SetStyleFore({ SCE_H_ATTRIBUTE, SCE_H_VALUE }, ThemeVariable());
    SetStyleFore({ SCE_H_DOUBLESTRING, SCE_H_SINGLESTRING, SCE_H_SGML_DOUBLESTRING, SCE_H_SGML_SIMPLESTRING }, ThemeString());
    SetStyleFore({ SCE_H_COMMENT, SCE_H_XCCOMMENT, SCE_H_SGML_COMMENT, SCE_H_SGML_1ST_PARAM_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_H_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_H_ENTITY, SCE_H_SGML_ENTITY, SCE_H_QUESTION, SCE_H_CDATA }, ThemeControlKeyword());
    SetStyleFore({ SCE_H_TAGUNKNOWN, SCE_H_ATTRIBUTEUNKNOWN, SCE_H_SGML_ERROR }, ThemeError());

    SetStyleFore({ SCE_HJ_WORD, SCE_HJ_KEYWORD }, ThemeKeyword());
    SetStyleFore({ SCE_HJ_COMMENT, SCE_HJ_COMMENTLINE, SCE_HJ_COMMENTDOC }, ThemeComment());
    SetStyleFore({ SCE_HJ_DOUBLESTRING, SCE_HJ_SINGLESTRING, SCE_HJ_TEMPLATELITERAL }, ThemeString());
    SetStyleFore({ SCE_HJ_REGEX }, ThemeEscape());
    SetStyleFore({ SCE_HJ_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_HJ_SYMBOLS }, ThemeOperator());

    SetStyleFore({ SCE_HPHP_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_HPHP_VARIABLE, SCE_HPHP_COMPLEX_VARIABLE }, ThemeVariable());
    SetStyleFore({ SCE_HPHP_HSTRING, SCE_HPHP_SIMPLESTRING, SCE_HPHP_HSTRING_VARIABLE }, ThemeString());
    SetStyleFore({ SCE_HPHP_COMMENT, SCE_HPHP_COMMENTLINE }, ThemeComment());
    SetStyleFore({ SCE_HPHP_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_HPHP_OPERATOR }, ThemeOperator());
}

void ApplyCssStyles()
{
    SetStyleFore({ SCE_CSS_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_CSS_TAG, SCE_CSS_CLASS, SCE_CSS_ID }, ThemeCssSelector());
    SetStyleFore({ SCE_CSS_IDENTIFIER, SCE_CSS_IDENTIFIER2, SCE_CSS_IDENTIFIER3,
        SCE_CSS_EXTENDED_IDENTIFIER, SCE_CSS_VARIABLE }, ThemeVariable());
    SetStyleFore({ SCE_CSS_VALUE, SCE_CSS_IMPORTANT }, ThemeString());
    SetStyleFore({ SCE_CSS_DIRECTIVE, SCE_CSS_GROUP_RULE }, ThemeControlKeyword());
    SetStyleFore({ SCE_CSS_PSEUDOCLASS, SCE_CSS_PSEUDOELEMENT, SCE_CSS_EXTENDED_PSEUDOCLASS,
        SCE_CSS_EXTENDED_PSEUDOELEMENT }, ThemeControlKeyword());
    SetStyleFore({ SCE_CSS_DOUBLESTRING, SCE_CSS_SINGLESTRING, SCE_CSS_ATTRIBUTE }, ThemeString());
    SetStyleFore({ SCE_CSS_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_CSS_UNKNOWN_IDENTIFIER, SCE_CSS_UNKNOWN_PSEUDOCLASS }, ThemeError());
}

void ApplyJsonStyles()
{
    SetStyleFore({ SCE_JSON_PROPERTYNAME }, ThemeVariable());
    SetStyleFore({ SCE_JSON_STRING, SCE_JSON_URI, SCE_JSON_COMPACTIRI }, ThemeString());
    SetStyleFore({ SCE_JSON_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_JSON_KEYWORD, SCE_JSON_LDKEYWORD }, ThemeKeyword());
    SetStyleFore({ SCE_JSON_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_JSON_LINECOMMENT, SCE_JSON_BLOCKCOMMENT }, ThemeComment());
    SetStyleFore({ SCE_JSON_ESCAPESEQUENCE }, ThemeEscape());
    SetStyleFore({ SCE_JSON_ERROR, SCE_JSON_STRINGEOL }, ThemeError());
}

void ApplySqlStyles()
{
    SetStyleFore({ SCE_SQL_COMMENT, SCE_SQL_COMMENTLINE, SCE_SQL_COMMENTDOC,
        SCE_SQL_COMMENTLINEDOC, SCE_SQL_SQLPLUS_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_SQL_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_SQL_WORD2, SCE_SQL_USER1, SCE_SQL_USER2, SCE_SQL_USER3, SCE_SQL_USER4 }, ThemeType());
    SetStyleFore({ SCE_SQL_STRING, SCE_SQL_CHARACTER, SCE_SQL_QUOTEDIDENTIFIER }, ThemeString());
    SetStyleFore({ SCE_SQL_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_SQL_OPERATOR, SCE_SQL_QOPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_SQL_SQLPLUS, SCE_SQL_SQLPLUS_PROMPT }, ThemeControlKeyword());
    SetStyleFore({ SCE_SQL_COMMENTDOCKEYWORDERROR }, ThemeError());
}

void ApplyBashStyles()
{
    SetStyleFore({ SCE_SH_COMMENTLINE }, ThemeComment());
    SetStyleFore({ SCE_SH_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_SH_STRING, SCE_SH_CHARACTER, SCE_SH_BACKTICKS, SCE_SH_HERE_Q }, ThemeString());
    SetStyleFore({ SCE_SH_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_SH_OPERATOR, SCE_SH_HERE_DELIM }, ThemeOperator());
    SetStyleFore({ SCE_SH_SCALAR, SCE_SH_PARAM }, ThemeVariable());
    SetStyleFore({ SCE_SH_ERROR }, ThemeError());
}

void ApplyPowerShellStyles()
{
    SetStyleFore({ SCE_POWERSHELL_COMMENT, SCE_POWERSHELL_COMMENTSTREAM, SCE_POWERSHELL_COMMENTDOCKEYWORD }, ThemeComment());
    SetStyleFore({ SCE_POWERSHELL_KEYWORD, SCE_POWERSHELL_CMDLET, SCE_POWERSHELL_ALIAS,
        SCE_POWERSHELL_USER1 }, ThemeKeyword());
    SetStyleFore({ SCE_POWERSHELL_FUNCTION }, ThemeFunction());
    SetStyleFore({ SCE_POWERSHELL_STRING, SCE_POWERSHELL_CHARACTER,
        SCE_POWERSHELL_HERE_STRING, SCE_POWERSHELL_HERE_CHARACTER }, ThemeString());
    SetStyleFore({ SCE_POWERSHELL_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_POWERSHELL_VARIABLE }, ThemeVariable());
    SetStyleFore({ SCE_POWERSHELL_OPERATOR }, ThemeOperator());
}

void ApplyRustStyles()
{
    SetStyleFore({ SCE_RUST_COMMENTBLOCK, SCE_RUST_COMMENTLINE, SCE_RUST_COMMENTBLOCKDOC, SCE_RUST_COMMENTLINEDOC }, ThemeComment());
    SetStyleFore({ SCE_RUST_WORD, SCE_RUST_WORD3, SCE_RUST_WORD4, SCE_RUST_WORD5, SCE_RUST_WORD6, SCE_RUST_WORD7 }, ThemeKeyword());
    SetStyleFore({ SCE_RUST_WORD2 }, ThemeType());
    SetStyleFore({ SCE_RUST_STRING, SCE_RUST_STRINGR, SCE_RUST_CHARACTER, SCE_RUST_BYTESTRING,
        SCE_RUST_BYTESTRINGR, SCE_RUST_BYTECHARACTER, SCE_RUST_CSTRING, SCE_RUST_CSTRINGR }, ThemeString());
    SetStyleFore({ SCE_RUST_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_RUST_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_RUST_LIFETIME, SCE_RUST_MACRO }, ThemeControlKeyword());
    SetStyleFore({ SCE_RUST_LEXERROR }, ThemeError());
}

void ApplyLuaStyles()
{
    SetStyleFore({ SCE_LUA_COMMENT, SCE_LUA_COMMENTLINE, SCE_LUA_COMMENTDOC }, ThemeComment());
    SetStyleFore({ SCE_LUA_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_LUA_WORD2, SCE_LUA_WORD3, SCE_LUA_WORD4, SCE_LUA_WORD5, SCE_LUA_WORD6, SCE_LUA_WORD7, SCE_LUA_WORD8 }, ThemeFunction());
    SetStyleFore({ SCE_LUA_STRING, SCE_LUA_CHARACTER, SCE_LUA_LITERALSTRING }, ThemeString());
    SetStyleFore({ SCE_LUA_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_LUA_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_LUA_PREPROCESSOR, SCE_LUA_LABEL }, ThemeControlKeyword());
    SetStyleFore({ SCE_LUA_STRINGEOL }, ThemeError());
}

void ApplyRubyStyles()
{
    SetStyleFore({ SCE_RB_COMMENTLINE, SCE_RB_POD, SCE_RB_DATASECTION }, ThemeComment());
    SetStyleFore({ SCE_RB_WORD, SCE_RB_WORD_DEMOTED }, ThemeKeyword());
    SetStyleFore({ SCE_RB_STRING, SCE_RB_CHARACTER, SCE_RB_BACKTICKS, SCE_RB_HERE_Q, SCE_RB_HERE_QQ,
        SCE_RB_HERE_QX, SCE_RB_STRING_Q, SCE_RB_STRING_QQ, SCE_RB_STRING_QX, SCE_RB_STRING_QW,
        SCE_RB_STRING_W, SCE_RB_STRING_I, SCE_RB_STRING_QI, SCE_RB_STRING_QS }, ThemeString());
    SetStyleFore({ SCE_RB_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_RB_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_RB_CLASSNAME, SCE_RB_MODULE_NAME }, ThemeType());
    SetStyleFore({ SCE_RB_DEFNAME }, ThemeFunction());
    SetStyleFore({ SCE_RB_GLOBAL, SCE_RB_INSTANCE_VAR, SCE_RB_CLASS_VAR, SCE_RB_SYMBOL,
        SCE_RB_STDIN, SCE_RB_STDOUT, SCE_RB_STDERR }, ThemeVariable());
    SetStyleFore({ SCE_RB_REGEX }, ThemeEscape());
    SetStyleFore({ SCE_RB_ERROR }, ThemeError());
}

void ApplyMarkdownStyles()
{
    SetStyleFore({ SCE_MARKDOWN_HEADER1, SCE_MARKDOWN_HEADER2, SCE_MARKDOWN_HEADER3,
        SCE_MARKDOWN_HEADER4, SCE_MARKDOWN_HEADER5, SCE_MARKDOWN_HEADER6 }, ThemeKeyword());
    SetStyleBold({ SCE_MARKDOWN_HEADER1, SCE_MARKDOWN_HEADER2, SCE_MARKDOWN_HEADER3,
        SCE_MARKDOWN_HEADER4, SCE_MARKDOWN_HEADER5, SCE_MARKDOWN_HEADER6 }, true);
    SetStyleFore({ SCE_MARKDOWN_STRONG1, SCE_MARKDOWN_STRONG2 }, ThemeFunction());
    SetStyleBold({ SCE_MARKDOWN_STRONG1, SCE_MARKDOWN_STRONG2 }, true);
    SetStyleFore({ SCE_MARKDOWN_EM1, SCE_MARKDOWN_EM2 }, ThemeFunction());
    SetStyleItalic({ SCE_MARKDOWN_EM1, SCE_MARKDOWN_EM2 }, true);
    SetStyleFore({ SCE_MARKDOWN_ULIST_ITEM, SCE_MARKDOWN_OLIST_ITEM, SCE_MARKDOWN_BLOCKQUOTE,
        SCE_MARKDOWN_HRULE, SCE_MARKDOWN_PRECHAR }, ThemeControlKeyword());
    SetStyleFore({ SCE_MARKDOWN_LINK }, ThemeLink());
    SetStyleFore({ SCE_MARKDOWN_CODE, SCE_MARKDOWN_CODE2, SCE_MARKDOWN_CODEBK }, ThemeString());
    SetStyleBack({ SCE_MARKDOWN_CODE, SCE_MARKDOWN_CODE2, SCE_MARKDOWN_CODEBK }, ThemeMarkdownCodeBack());
    SetStyleFore({ SCE_MARKDOWN_STRIKEOUT }, ThemeLineNumber());
}

void ApplyYamlStyles()
{
    SetStyleFore({ SCE_YAML_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_YAML_IDENTIFIER }, ThemeVariable());
    SetStyleFore({ SCE_YAML_KEYWORD }, ThemeKeyword());
    SetStyleFore({ SCE_YAML_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_YAML_REFERENCE, SCE_YAML_DOCUMENT }, ThemeControlKeyword());
    SetStyleFore({ SCE_YAML_TEXT }, ThemeString());
    SetStyleFore({ SCE_YAML_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_YAML_ERROR }, ThemeError());
}

void ApplyTomlStyles()
{
    SetStyleFore({ SCE_TOML_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_TOML_IDENTIFIER, SCE_TOML_KEY }, ThemeVariable());
    SetStyleFore({ SCE_TOML_KEYWORD }, ThemeKeyword());
    SetStyleFore({ SCE_TOML_NUMBER, SCE_TOML_DATETIME }, ThemeNumber());
    SetStyleFore({ SCE_TOML_TABLE }, ThemeType());
    SetStyleFore({ SCE_TOML_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_TOML_STRING_SQ, SCE_TOML_STRING_DQ,
        SCE_TOML_TRIPLE_STRING_SQ, SCE_TOML_TRIPLE_STRING_DQ }, ThemeString());
    SetStyleFore({ SCE_TOML_ESCAPECHAR }, ThemeEscape());
    SetStyleFore({ SCE_TOML_ERROR, SCE_TOML_STRINGEOL }, ThemeError());
}

void ApplyPropertiesStyles()
{
    SetStyleFore({ SCE_PROPS_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_PROPS_SECTION }, ThemeType());
    SetStyleFore({ SCE_PROPS_ASSIGNMENT }, ThemeOperator());
    SetStyleFore({ SCE_PROPS_DEFVAL }, ThemeString());
    SetStyleFore({ SCE_PROPS_KEY }, ThemeVariable());
}

void ApplyMakefileStyles()
{
    SetStyleFore({ SCE_MAKE_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_MAKE_PREPROCESSOR }, ThemeControlKeyword());
    SetStyleFore({ SCE_MAKE_IDENTIFIER }, ThemeVariable());
    SetStyleFore({ SCE_MAKE_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_MAKE_TARGET }, ThemeFunction());
    SetStyleFore({ SCE_MAKE_IDEOL }, ThemeError());
}

void ApplyDiffStyles()
{
    SetStyleFore({ SCE_DIFF_COMMENT, SCE_DIFF_HEADER, SCE_DIFF_POSITION }, ThemeControlKeyword());
    SetStyleFore({ SCE_DIFF_COMMAND }, ThemeKeyword());
    SetStyleFore({ SCE_DIFF_ADDED, SCE_DIFF_PATCH_ADD }, ThemeComment());
    SetStyleFore({ SCE_DIFF_DELETED, SCE_DIFF_PATCH_DELETE }, ThemeError());
    SetStyleFore({ SCE_DIFF_CHANGED }, ThemeNumber());
    SetStyleFore({ SCE_DIFF_REMOVED_PATCH_ADD, SCE_DIFF_REMOVED_PATCH_DELETE }, ThemeLineNumber());
}

void ApplyBatchStyles()
{
    SetStyleFore({ SCE_BAT_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_BAT_WORD, SCE_BAT_COMMAND }, ThemeKeyword());
    SetStyleFore({ SCE_BAT_LABEL, SCE_BAT_AFTER_LABEL }, ThemeFunction());
    SetStyleFore({ SCE_BAT_IDENTIFIER }, ThemeVariable());
    SetStyleFore({ SCE_BAT_OPERATOR }, ThemeOperator());
}

void ApplyZigStyles()
{
    SetStyleFore({ SCE_ZIG_COMMENTLINE, SCE_ZIG_COMMENTLINEDOC, SCE_ZIG_COMMENTLINETOP }, ThemeComment());
    SetStyleFore({ SCE_ZIG_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_ZIG_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_ZIG_CHARACTER, SCE_ZIG_STRING, SCE_ZIG_MULTISTRING,
        SCE_ZIG_IDENTIFIER_STRING }, ThemeString());
    SetStyleFore({ SCE_ZIG_ESCAPECHAR }, ThemeEscape());
    SetStyleFore({ SCE_ZIG_FUNCTION }, ThemeFunction());
    SetStyleFore({ SCE_ZIG_BUILTIN_FUNCTION }, ThemeControlKeyword());
    SetStyleFore({ SCE_ZIG_KW_PRIMARY, SCE_ZIG_KW_SECONDARY, SCE_ZIG_KW_TERTIARY }, ThemeKeyword());
    SetStyleFore({ SCE_ZIG_KW_TYPE }, ThemeType());
    SetStyleFore({ SCE_ZIG_STRINGEOL }, ThemeError());
}

void ApplyNimStyles()
{
    SetStyleFore({ SCE_NIM_COMMENT, SCE_NIM_COMMENTDOC, SCE_NIM_COMMENTLINE,
        SCE_NIM_COMMENTLINEDOC }, ThemeComment());
    SetStyleFore({ SCE_NIM_WORD }, ThemeKeyword());
    SetStyleFore({ SCE_NIM_NUMBER }, ThemeNumber());
    SetStyleFore({ SCE_NIM_STRING, SCE_NIM_CHARACTER, SCE_NIM_TRIPLE,
        SCE_NIM_TRIPLEDOUBLE, SCE_NIM_BACKTICKS }, ThemeString());
    SetStyleFore({ SCE_NIM_FUNCNAME }, ThemeFunction());
    SetStyleFore({ SCE_NIM_OPERATOR }, ThemeOperator());
    SetStyleFore({ SCE_NIM_STRINGEOL, SCE_NIM_NUMERROR }, ThemeError());
}

void ApplyRegistryStyles()
{
    SetStyleFore({ SCE_REG_COMMENT }, ThemeComment());
    SetStyleFore({ SCE_REG_VALUENAME, SCE_REG_PARAMETER }, ThemeVariable());
    SetStyleFore({ SCE_REG_STRING, SCE_REG_STRING_GUID }, ThemeString());
    SetStyleFore({ SCE_REG_HEXDIGIT }, ThemeNumber());
    SetStyleFore({ SCE_REG_VALUETYPE }, ThemeKeyword());
    SetStyleFore({ SCE_REG_ADDEDKEY, SCE_REG_KEYPATH_GUID }, ThemeType());
    SetStyleFore({ SCE_REG_DELETEDKEY }, ThemeError());
    SetStyleFore({ SCE_REG_ESCAPED }, ThemeEscape());
    SetStyleFore({ SCE_REG_OPERATOR }, ThemeOperator());
}

void ApplyInnoStyles()
{
    SetStyleFore({ SCE_INNO_COMMENT, SCE_INNO_COMMENT_PASCAL }, ThemeComment());
    SetStyleFore({ SCE_INNO_KEYWORD, SCE_INNO_KEYWORD_PASCAL, SCE_INNO_KEYWORD_USER }, ThemeKeyword());
    SetStyleFore({ SCE_INNO_PARAMETER, SCE_INNO_INLINE_EXPANSION }, ThemeVariable());
    SetStyleFore({ SCE_INNO_SECTION }, ThemeType());
    SetStyleFore({ SCE_INNO_PREPROC }, ThemeControlKeyword());
    SetStyleFore({ SCE_INNO_STRING_DOUBLE, SCE_INNO_STRING_SINGLE }, ThemeString());
}

void ApplySyntaxFamilyStyles(SyntaxFamily family)
{
    switch (family)
    {
    case SyntaxFamily::CppLike:
        ApplyCppLikeStyles();
        break;
    case SyntaxFamily::Python:
        ApplyPythonStyles();
        break;
    case SyntaxFamily::HyperText:
        ApplyHyperTextStyles();
        break;
    case SyntaxFamily::Css:
        ApplyCssStyles();
        break;
    case SyntaxFamily::Json:
        ApplyJsonStyles();
        break;
    case SyntaxFamily::Sql:
        ApplySqlStyles();
        break;
    case SyntaxFamily::Bash:
        ApplyBashStyles();
        break;
    case SyntaxFamily::PowerShell:
        ApplyPowerShellStyles();
        break;
    case SyntaxFamily::Rust:
        ApplyRustStyles();
        break;
    case SyntaxFamily::Lua:
        ApplyLuaStyles();
        break;
    case SyntaxFamily::Ruby:
        ApplyRubyStyles();
        break;
    case SyntaxFamily::Markdown:
        ApplyMarkdownStyles();
        break;
    case SyntaxFamily::Yaml:
        ApplyYamlStyles();
        break;
    case SyntaxFamily::Toml:
        ApplyTomlStyles();
        break;
    case SyntaxFamily::Properties:
        ApplyPropertiesStyles();
        break;
    case SyntaxFamily::Makefile:
        ApplyMakefileStyles();
        break;
    case SyntaxFamily::Diff:
        ApplyDiffStyles();
        break;
    case SyntaxFamily::Batch:
        ApplyBatchStyles();
        break;
    case SyntaxFamily::Zig:
        ApplyZigStyles();
        break;
    case SyntaxFamily::Nim:
        ApplyNimStyles();
        break;
    case SyntaxFamily::Registry:
        ApplyRegistryStyles();
        break;
    case SyntaxFamily::Inno:
        ApplyInnoStyles();
        break;
    case SyntaxFamily::Plain:
    default:
        break;
    }
}

void UpdateLanguageMenuCheckRecursive(HMENU menu)
{
    if (!menu)
        return;

    const int itemCount = GetMenuItemCount(menu);
    for (int index = 0; index < itemCount; ++index)
    {
        HMENU submenu = GetSubMenu(menu, index);
        if (submenu)
        {
            UpdateLanguageMenuCheckRecursive(submenu);
            continue;
        }

        const UINT commandId = GetMenuItemID(menu, index);
        if (commandId == static_cast<UINT>(-1) || !IsLanguageCommand(static_cast<int>(commandId)))
            continue;

        CheckMenuItem(menu, index, MF_BYPOSITION |
            (static_cast<int>(commandId) == g_currentLanguageCommand ? MF_CHECKED : MF_UNCHECKED));
    }
}

void UpdateLanguageMenuCheck()
{
    HMENU hMenu = GetMenu(hWnd);
    if (!hMenu)
        return;

    HMENU hLanguageMenu = GetSubMenu(hMenu, kLanguageMenuIndex);
    if (!hLanguageMenu)
        return;

    UpdateLanguageMenuCheckRecursive(hLanguageMenu);
    DrawMenuBar(hWnd);
}

void ApplyLanguage(int commandId)
{
    const LanguageDefinition* language = FindLanguageDefinition(commandId);
    if (!language)
        return;

    ClearKeywordLists();

    ILexer5* lexer = nullptr;
    if (language->lexerName)
        lexer = CreateLexer(language->lexerName);

    Sci(SCI_SETILEXER, 0, reinterpret_cast<sptr_t>(lexer));
    ApplyBaseEditorStyles();

    for (uptr_t index = 0; index < language->keywords.size(); ++index)
    {
        const char* keywords = language->keywords[index] ? language->keywords[index] : "";
        Sci(SCI_SETKEYWORDS, index, reinterpret_cast<sptr_t>(keywords));
    }

    ApplySyntaxFamilyStyles(language->family);
    ConfigureCodeFolding(language->family);
    Sci(SCI_COLOURISE, 0, -1);

    g_currentLanguageCommand = commandId;
    UpdateLanguageMenuCheck();
}

void InitScintillaEditor()
{
    Sci(SCI_SETCODEPAGE, SC_CP_UTF8, 0);
    Sci(SCI_SETMULTIPLESELECTION, TRUE, 0);
    Sci(SCI_SETADDITIONALSELECTIONTYPING, TRUE, 0);
    Sci(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
    Sci(SCI_SETADDITIONALCARETSVISIBLE, TRUE, 0);
    Sci(SCI_SETADDITIONALCARETSBLINK, TRUE, 0);
    Sci(SCI_SETVIRTUALSPACEOPTIONS, SCVS_RECTANGULARSELECTION, 0);
    Sci(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, TRUE, 0);
    Sci(SCI_SETRECTANGULARSELECTIONMODIFIER, SCMOD_ALT, 0);
    ConfigureControlCharacterRepresentations();
    ApplyBaseEditorStyles();
    ApplyLanguage(IDM_LANG_TEXT);
    UpdateViewMenuCheck();
}

void HandleEditorMarginClick(const SCNotification* notification)
{
    if (!notification || notification->margin != kFoldMargin)
        return;

    const sptr_t position = (std::max)(static_cast<sptr_t>(notification->position), static_cast<sptr_t>(0));
    const sptr_t line = Sci(SCI_LINEFROMPOSITION, static_cast<uptr_t>(position), 0);
    const sptr_t foldLevel = Sci(SCI_GETFOLDLEVEL, static_cast<uptr_t>(line), 0);
    if ((foldLevel & SC_FOLDLEVELHEADERFLAG) == 0)
        return;

    Sci(SCI_TOGGLEFOLD, static_cast<uptr_t>(line), 0);
}

void ApplyAppTheme()
{
    DeleteObject(g_hPopupBackBrush);
    DeleteObject(g_hPopupSurfaceBrush);
    DeleteObject(g_hPopupInputBrush);
    DeleteObject(g_hMenuBackBrush);
    g_hPopupBackBrush = nullptr;
    g_hPopupSurfaceBrush = nullptr;
    g_hPopupInputBrush = nullptr;
    g_hMenuBackBrush = nullptr;

    ApplyWindowChromeTheme(hWnd);
    ApplyWindowAppIcons(hWnd);
    ApplyMainMenuTheme();

    if (g_hFolderTree || g_hOpenFilesTree)
    {
        ApplyFolderTreeTheme();
        InitializeFolderTreeImageList();
        InvalidateOpenFilesTree();
        InvalidateFolderTree();
    }

    if (g_hSci)
    {
        ApplyControlTheme(g_hSci);
        ApplyLanguage(g_currentLanguageCommand);
    }

    if (hWnd)
        InvalidateRect(hWnd, nullptr, TRUE);
    if (g_findWindow)
        g_findWindow->UpdateThemeAndLanguage(IsDarkTheme(), g_appLanguage == AppLanguage::Chinese);
    if (g_hSettingsWindow && IsWindow(g_hSettingsWindow))
    {
        ApplyWindowChromeTheme(g_hSettingsWindow);
        RedrawWindow(g_hSettingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (g_hAboutWindow && IsWindow(g_hAboutWindow))
    {
        ApplyWindowChromeTheme(g_hAboutWindow);
        ApplyWindowAppIcons(g_hAboutWindow);
        RedrawWindow(g_hAboutWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (g_hColumnEditorWindow && IsWindow(g_hColumnEditorWindow))
    {
        ApplyWindowChromeTheme(g_hColumnEditorWindow);
        RedrawWindow(g_hColumnEditorWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    InvalidateTabBar();
    InvalidateStatusBar();
}

std::wstring FileNameFromPath(const std::wstring& path)
{
    const size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
        return path;
    std::wstring name = path.substr(pos + 1);
    return name.empty() ? path : name;
}

std::wstring ToLower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t ch) {
        return static_cast<wchar_t>(towlower(ch));
    });
    return value;
}

std::wstring ExtensionFromPath(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash))
        return L"";
    return ToLower(path.substr(dot));
}

int DetectLanguageFromPath(const std::wstring& path)
{
    const std::wstring ext = ExtensionFromPath(path);
    const std::wstring name = ToLower(FileNameFromPath(path));
    if (name == L"makefile" || name == L"gnumakefile")
        return IDM_LANG_MAKEFILE;
    if (ext == L".c" || ext == L".cc" || ext == L".cpp" || ext == L".cxx" ||
        ext == L".h" || ext == L".hh" || ext == L".hpp" || ext == L".hxx")
        return IDM_LANG_CPP;
    if (ext == L".cs")
        return IDM_LANG_CSHARP;
    if (ext == L".java")
        return IDM_LANG_JAVA;
    if (ext == L".js" || ext == L".jsx" || ext == L".mjs" || ext == L".cjs")
        return IDM_LANG_JAVASCRIPT;
    if (ext == L".ts" || ext == L".tsx")
        return IDM_LANG_TYPESCRIPT;
    if (ext == L".py" || ext == L".pyw")
        return IDM_LANG_PYTHON;
    if (ext == L".htm" || ext == L".html")
        return IDM_LANG_HTML;
    if (ext == L".xml" || ext == L".xaml" || ext == L".svg")
        return IDM_LANG_XML;
    if (ext == L".css" || ext == L".scss" || ext == L".less")
        return IDM_LANG_CSS;
    if (ext == L".json" || ext == L".jsonc")
        return IDM_LANG_JSON;
    if (ext == L".sql")
        return IDM_LANG_SQL;
    if (ext == L".sh" || ext == L".bash" || ext == L".zsh")
        return IDM_LANG_BASH;
    if (ext == L".ps1" || ext == L".psm1" || ext == L".psd1")
        return IDM_LANG_POWERSHELL;
    if (ext == L".rs")
        return IDM_LANG_RUST;
    if (ext == L".lua")
        return IDM_LANG_LUA;
    if (ext == L".rb")
        return IDM_LANG_RUBY;
    if (ext == L".md" || ext == L".markdown")
        return IDM_LANG_MARKDOWN;
    if (ext == L".yaml" || ext == L".yml")
        return IDM_LANG_YAML;
    if (ext == L".toml")
        return IDM_LANG_TOML;
    if (ext == L".ini" || ext == L".properties" || ext == L".props" ||
        ext == L".conf" || ext == L".env" || name == L".editorconfig")
        return IDM_LANG_PROPERTIES;
    if (ext == L".mk" || ext == L".mak")
        return IDM_LANG_MAKEFILE;
    if (ext == L".diff" || ext == L".patch")
        return IDM_LANG_DIFF;
    if (ext == L".bat" || ext == L".cmd")
        return IDM_LANG_BATCH;
    if (ext == L".zig")
        return IDM_LANG_ZIG;
    if (ext == L".nim" || ext == L".nims")
        return IDM_LANG_NIM;
    if (ext == L".reg")
        return IDM_LANG_REGISTRY;
    if (ext == L".iss")
        return IDM_LANG_INNO;
    return IDM_LANG_TEXT;
}

bool IsActiveTabValid()
{
    return g_activeTabIndex >= 0 && g_activeTabIndex < static_cast<int>(g_tabs.size());
}

std::string GetEditorText()
{
    const sptr_t length = Sci(SCI_GETLENGTH);
    if (length <= 0)
        return {};

    std::string text(static_cast<size_t>(length), '\0');
    std::vector<char> buffer(static_cast<size_t>(length) + 1);
    Sci(SCI_GETTEXT, static_cast<uptr_t>(buffer.size()), reinterpret_cast<sptr_t>(buffer.data()));
    text.assign(buffer.data(), static_cast<size_t>(length));
    return text;
}

void SetEditorText(const std::string& text)
{
    Sci(SCI_SETTEXT, 0, reinterpret_cast<sptr_t>(text.c_str()));
}


std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};

    const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1)
        return {};

    std::string result(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), required, nullptr, nullptr);
    result.pop_back();
    return result;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return {};

    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0)
        return {};

    std::wstring result(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), required);
    return result;
}

std::string WideToUtf8Bytes(const wchar_t* text, int length)
{
    if (!text || length <= 0)
        return {};

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if (required <= 0)
        return {};

    std::string result(static_cast<size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, length, result.data(), required, nullptr, nullptr);
    return result;
}

std::wstring BytesToWide(const char* text, int length, UINT codePage)
{
    if (!text || length <= 0)
        return {};

    const DWORD flags = codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    const int required = MultiByteToWideChar(codePage, flags, text, length, nullptr, 0);
    if (required <= 0)
        return {};

    std::wstring result(static_cast<size_t>(required), L'\0');
    MultiByteToWideChar(codePage, flags, text, length, result.data(), required);
    return result;
}

bool IsValidUtf8Bytes(const char* text, int length)
{
    if (length <= 0)
        return true;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, length, nullptr, 0) > 0;
}

std::wstring Utf16BytesToWide(const char* bytes, size_t byteCount, bool bigEndian)
{
    std::wstring result;
    result.reserve(byteCount / 2);
    for (size_t index = 0; index + 1 < byteCount; index += 2)
    {
        const unsigned char first = static_cast<unsigned char>(bytes[index]);
        const unsigned char second = static_cast<unsigned char>(bytes[index + 1]);
        const wchar_t ch = static_cast<wchar_t>(bigEndian ? ((first << 8) | second) : ((second << 8) | first));
        result.push_back(ch);
    }
    return result;
}

void AppendUtf16Bytes(std::vector<char>& output, const std::wstring& text, bool bigEndian)
{
    output.reserve(output.size() + (text.size() * 2));
    for (wchar_t ch : text)
    {
        const unsigned short value = static_cast<unsigned short>(ch);
        if (bigEndian)
        {
            output.push_back(static_cast<char>((value >> 8) & 0xff));
            output.push_back(static_cast<char>(value & 0xff));
        }
        else
        {
            output.push_back(static_cast<char>(value & 0xff));
            output.push_back(static_cast<char>((value >> 8) & 0xff));
        }
    }
}

std::string DecodeFileBytesToUtf8(const std::vector<char>& content, size_t byteCount, DocumentEncoding& encoding)
{
    if (byteCount >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF)
    {
        encoding = DocumentEncoding::Utf8Bom;
        return std::string(content.data() + 3, byteCount - 3);
    }

    if (byteCount >= 2 &&
        static_cast<unsigned char>(content[0]) == 0xFF &&
        static_cast<unsigned char>(content[1]) == 0xFE)
    {
        encoding = DocumentEncoding::Utf16LE;
        return WideToUtf8(Utf16BytesToWide(content.data() + 2, byteCount - 2, false));
    }

    if (byteCount >= 2 &&
        static_cast<unsigned char>(content[0]) == 0xFE &&
        static_cast<unsigned char>(content[1]) == 0xFF)
    {
        encoding = DocumentEncoding::Utf16BE;
        return WideToUtf8(Utf16BytesToWide(content.data() + 2, byteCount - 2, true));
    }

    if (IsValidUtf8Bytes(content.data(), static_cast<int>(byteCount)))
    {
        encoding = DocumentEncoding::Utf8;
        return std::string(content.data(), byteCount);
    }

    encoding = DocumentEncoding::Ansi;
    return WideToUtf8(BytesToWide(content.data(), static_cast<int>(byteCount), CP_ACP));
}

std::vector<char> EncodeUtf8ForFile(const std::string& utf8Text, DocumentEncoding encoding)
{
    std::vector<char> output;
    if (encoding == DocumentEncoding::Utf8)
    {
        output.assign(utf8Text.begin(), utf8Text.end());
        return output;
    }

    if (encoding == DocumentEncoding::Utf8Bom)
    {
        output = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
        output.insert(output.end(), utf8Text.begin(), utf8Text.end());
        return output;
    }

    const std::wstring wideText = Utf8ToWide(utf8Text);
    if (encoding == DocumentEncoding::Ansi)
    {
        if (wideText.empty())
            return output;

        const int required = WideCharToMultiByte(CP_ACP, 0, wideText.data(), static_cast<int>(wideText.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0)
            return output;

        output.resize(static_cast<size_t>(required));
        WideCharToMultiByte(CP_ACP, 0, wideText.data(), static_cast<int>(wideText.size()), output.data(), required, nullptr, nullptr);
        return output;
    }

    if (encoding == DocumentEncoding::Utf16LE)
    {
        output = { static_cast<char>(0xFF), static_cast<char>(0xFE) };
        AppendUtf16Bytes(output, wideText, false);
    }
    else if (encoding == DocumentEncoding::Utf16BE)
    {
        output = { static_cast<char>(0xFE), static_cast<char>(0xFF) };
        AppendUtf16Bytes(output, wideText, true);
    }
    return output;
}

const wchar_t* EncodingDisplayName(DocumentEncoding encoding)
{
    switch (encoding)
    {
    case DocumentEncoding::Utf8Bom:
        return L"UTF-8 BOM";
    case DocumentEncoding::Ansi:
        return L"ANSI";
    case DocumentEncoding::Utf16LE:
        return L"UTF-16 LE";
    case DocumentEncoding::Utf16BE:
        return L"UTF-16 BE";
    case DocumentEncoding::Utf8:
    default:
        return L"UTF-8";
    }
}

const wchar_t* EolDisplayName(int eolMode)
{
    switch (eolMode)
    {
    case SC_EOL_LF:
        return L"Unix (LF)";
    case SC_EOL_CR:
        return L"macOS (CR)";
    case SC_EOL_CRLF:
    default:
        return L"Windows (CRLF)";
    }
}

int DetectEolModeFromText(const std::string& text)
{
    const size_t crlf = text.find("\r\n");
    const size_t lf = text.find('\n');
    const size_t cr = text.find('\r');

    if (crlf != std::string::npos)
        return SC_EOL_CRLF;
    if (lf != std::string::npos)
        return SC_EOL_LF;
    if (cr != std::string::npos)
        return SC_EOL_CR;
    return SC_EOL_CRLF;
}


bool IsEditCommand(int commandId)
{
    return (commandId >= IDM_EDIT_UNDO && commandId <= IDM_EDIT_SELECT_ALL) ||
        commandId == IDM_EDIT_COLUMN_EDITOR;
}

bool IsModifyingEditCommand(int commandId)
{
    switch (commandId)
    {
    case IDM_EDIT_UNDO:
    case IDM_EDIT_REDO:
    case IDM_EDIT_CUT:
    case IDM_EDIT_PASTE:
    case IDM_EDIT_DELETE:
    case IDM_EDIT_COLUMN_EDITOR:
        return true;
    default:
        return false;
    }
}

bool IsActiveDocumentReadOnly()
{
    return g_hSci && Sci(SCI_GETREADONLY) != 0;
}

void ShowReadOnlyWarning(HWND owner)
{
    MessageBoxW(
        owner ? owner : hWnd,
        UiText(L"\u5F53\u524D\u6587\u4EF6\u662F\u53EA\u8BFB\u7684\uFF0C\u65E0\u6CD5\u4FEE\u6539\u3002",
            L"The current file is read-only and cannot be modified."),
        L"openedit",
        MB_OK | MB_ICONWARNING);
}

void ExecuteEditCommand(int commandId)
{
    if (IsModifyingEditCommand(commandId) && IsActiveDocumentReadOnly())
    {
        ShowReadOnlyWarning(hWnd);
        SetFocus(g_hSci);
        return;
    }

    switch (commandId)
    {
    case IDM_EDIT_UNDO:
        Sci(SCI_UNDO);
        break;
    case IDM_EDIT_REDO:
        Sci(SCI_REDO);
        break;
    case IDM_EDIT_CUT:
        Sci(SCI_CUT);
        break;
    case IDM_EDIT_COPY:
        Sci(SCI_COPY);
        break;
    case IDM_EDIT_PASTE:
        Sci(SCI_PASTE);
        break;
    case IDM_EDIT_DELETE:
        Sci(SCI_CLEAR);
        break;
    case IDM_EDIT_SELECT_ALL:
        Sci(SCI_SELECTALL);
        break;
    case IDM_EDIT_COLUMN_EDITOR:
        ShowColumnEditorWindow();
        break;
    default:
        break;
    }
    if (commandId != IDM_EDIT_COLUMN_EDITOR)
        SetFocus(g_hSci);
}


void DestroyPopupWindowWithoutFlash(HWND popupWindow)
{
    if (!popupWindow || !IsWindow(popupWindow))
        return;

    if (hWnd && IsWindow(hWnd) && !IsWindowEnabled(hWnd))
        EnableWindow(hWnd, TRUE);
    if (hWnd && IsWindow(hWnd))
        SetActiveWindow(hWnd);
    DestroyWindow(popupWindow);
}

void RestoreMainWindowAfterPopupClose(bool focusEditor)
{
    if (!hWnd || !IsWindow(hWnd))
        return;

    if (!IsWindowEnabled(hWnd))
        EnableWindow(hWnd, TRUE);
    SetActiveWindow(hWnd);
    if (focusEditor && g_hSci && IsWindow(g_hSci))
        SetFocus(g_hSci);
}

void InvalidateTabBar()
{
    if (g_hTabBar)
        InvalidateRect(g_hTabBar, nullptr, TRUE);
}

void InvalidateStatusBar()
{
    if (g_hStatusBar)
        InvalidateRect(g_hStatusBar, nullptr, TRUE);
}

bool GetTabModifiedState(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return false;

    return g_tabs[tabIndex].modified || (tabIndex == g_activeTabIndex && !g_loadingTabContent && Sci(SCI_GETMODIFY));
}

void SetActiveTabModified(bool modified)
{
    if (!IsActiveTabValid())
        return;

    DocumentTab& tab = g_tabs[g_activeTabIndex];
    if (tab.modified == modified)
        return;

    tab.modified = modified;
    InvalidateTabBar();
    RefreshOpenFilesTree();
    InvalidateStatusBar();
}

std::wstring GetTabDisplayTitle(const DocumentTab& tab)
{
    if (!tab.title.empty())
        return tab.title;
    if (!tab.path.empty())
        return FileNameFromPath(tab.path);
    return L"Untitled";
}

std::wstring GetTabBarDisplayTitle(const DocumentTab& tab)
{
    std::wstring title = GetTabDisplayTitle(tab);
    if (tab.readOnly)
        title = L"[R] " + title;
    return title;
}

std::wstring GetTabTooltipText(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return L"";

    const DocumentTab& tab = g_tabs[tabIndex];
    return tab.path.empty() ? GetTabDisplayTitle(tab) : FileNameFromPath(tab.path);
}

bool ShouldShowTabInOpenFilesTree(const DocumentTab& tab)
{
    return !tab.untitled && !tab.path.empty() && !tab.openedFromFolder;
}

bool HasOpenFilesTreeTabs()
{
    for (const DocumentTab& tab : g_tabs)
    {
        if (ShouldShowTabInOpenFilesTree(tab))
            return true;
    }
    return false;
}

OpenFileItem* StoreOpenFileItem(int tabIndex, std::wstring title)
{
    auto item = std::make_unique<OpenFileItem>();
    item->tabIndex = tabIndex;
    item->title = std::move(title);
    OpenFileItem* itemData = item.get();
    g_openFileItems.push_back(std::move(item));
    return itemData;
}

OpenFileItem* GetOpenFileItemData(HTREEITEM treeItem)
{
    if (!g_hOpenFilesTree || !treeItem)
        return nullptr;

    TVITEMW item{};
    item.mask = TVIF_PARAM;
    item.hItem = treeItem;
    if (!TreeView_GetItem(g_hOpenFilesTree, &item))
        return nullptr;
    return reinterpret_cast<OpenFileItem*>(item.lParam);
}

HTREEITEM InsertOpenFilesTreeItem(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return nullptr;

    std::wstring title = GetTabDisplayTitle(g_tabs[tabIndex]);
    if (GetTabModifiedState(tabIndex))
        title += L" *";

    OpenFileItem* itemData = StoreOpenFileItem(tabIndex, title);
    TVINSERTSTRUCTW insert{};
    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    insert.item.pszText = const_cast<LPWSTR>(itemData->title.c_str());
    insert.item.iImage = g_textFileIconIndex;
    insert.item.iSelectedImage = g_textFileIconIndex;
    insert.item.lParam = reinterpret_cast<LPARAM>(itemData);

    return TreeView_InsertItem(g_hOpenFilesTree, &insert);
}

void RefreshOpenFilesTree()
{
    if (!g_hOpenFilesTree)
        return;

    ScopedRedrawPause redrawPause(g_hOpenFilesTree);
    TreeView_DeleteAllItems(g_hOpenFilesTree);
    g_openFileItems.clear();

    HTREEITEM activeItem = nullptr;
    for (int index = 0; index < static_cast<int>(g_tabs.size()); ++index)
    {
        if (!ShouldShowTabInOpenFilesTree(g_tabs[index]))
            continue;

        HTREEITEM item = InsertOpenFilesTreeItem(index);
        if (index == g_activeTabIndex)
            activeItem = item;
    }

    TreeView_SelectItem(g_hOpenFilesTree, activeItem);
}

bool HandleOpenFilesTreeItemClickAt(POINT clientPoint)
{
    if (!g_hOpenFilesTree)
        return false;

    TVHITTESTINFO hitTest{};
    hitTest.pt = clientPoint;
    HTREEITEM clicked = TreeView_HitTest(g_hOpenFilesTree, &hitTest);
    if (!clicked)
        return false;

    if ((hitTest.flags & (TVHT_ONITEMICON | TVHT_ONITEMLABEL)) == 0)
        return false;

    TreeView_SelectItem(g_hOpenFilesTree, clicked);
    OpenFileItem* item = GetOpenFileItemData(clicked);
    if (!item || item->tabIndex < 0 || item->tabIndex >= static_cast<int>(g_tabs.size()))
        return true;

    SwitchToTab(item->tabIndex);
    return true;
}

sptr_t ClampEditorPosition(sptr_t position)
{
    const sptr_t length = Sci(SCI_GETLENGTH);
    return (std::min)((std::max)(position, static_cast<sptr_t>(0)), length);
}

void CaptureActiveTab()
{
    if (!IsActiveTabValid() || g_loadingTabContent)
        return;

    DocumentTab& tab = g_tabs[g_activeTabIndex];
    tab.text = GetEditorText();
    tab.languageCommand = g_currentLanguageCommand;
    tab.eolMode = static_cast<int>(Sci(SCI_GETEOLMODE));
    tab.caretPosition = Sci(SCI_GETCURRENTPOS);
    tab.anchorPosition = Sci(SCI_GETANCHOR);
    tab.firstVisibleLine = Sci(SCI_GETFIRSTVISIBLELINE);
    tab.xOffset = Sci(SCI_GETXOFFSET);
    tab.modified = tab.text != tab.savedText;
    tab.readOnly = Sci(SCI_GETREADONLY) != 0;
}

DocumentTab CreateUntitledTab()
{
    DocumentTab tab;
    tab.title = L"Untitled " + std::to_wstring(g_nextUntitledIndex++);
    tab.languageCommand = IDM_LANG_TEXT;
    tab.encoding = DocumentEncoding::Utf8;
    tab.eolMode = SC_EOL_CRLF;
    tab.modified = false;
    tab.untitled = true;
    return tab;
}

DocumentTab CreateDefaultUntitledTab()
{
    DocumentTab tab;
    tab.title = L"Untitled";
    tab.languageCommand = IDM_LANG_TEXT;
    tab.encoding = DocumentEncoding::Utf8;
    tab.eolMode = SC_EOL_CRLF;
    tab.modified = false;
    tab.untitled = true;
    return tab;
}

void UpdateWindowTitle()
{
    std::wstring title = kAppDisplayName;
    if (IsActiveTabValid())
    {
        title = GetTabDisplayTitle(g_tabs[g_activeTabIndex]) + L" - " + kAppDisplayName;
    }
    else if (!g_currentFilePath.empty())
    {
        title = FileNameFromPath(g_currentFilePath) + L" - " + kAppDisplayName;
    }
    else if (!g_currentFolderPath.empty())
    {
        title = FileNameFromPath(g_currentFolderPath) + L" - " + kAppDisplayName;
    }
    SetWindowTextW(hWnd, title.c_str());
}

void LoadTabIntoEditor(int tabIndex)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return;

    g_loadingTabContent = true;
    DocumentTab& tab = g_tabs[tabIndex];
    g_activeTabIndex = tabIndex;
    g_currentFilePath = tab.path;

    {
        ScopedRedrawPause redrawPause(g_hSci);
        Sci(SCI_SETREADONLY, FALSE, 0);
        ClearWordHighlights();
        ClearSearchMarks();
        Sci(SCI_SETEOLMODE, tab.eolMode, 0);
        SetEditorText(tab.text);
        ApplyLanguage(tab.languageCommand);
        RestoreSearchMarksForActiveTab();
        Sci(SCI_EMPTYUNDOBUFFER);
        Sci(SCI_SETSAVEPOINT);
        const sptr_t anchorPosition = ClampEditorPosition(tab.anchorPosition);
        const sptr_t caretPosition = ClampEditorPosition(tab.caretPosition);
        const sptr_t firstVisibleLine = (std::max)(tab.firstVisibleLine, static_cast<sptr_t>(0));
        const sptr_t xOffset = (std::max)(tab.xOffset, static_cast<sptr_t>(0));
        Sci(SCI_SETSEL, static_cast<uptr_t>(anchorPosition), caretPosition);
        Sci(SCI_SETFIRSTVISIBLELINE, static_cast<uptr_t>(firstVisibleLine), 0);
        Sci(SCI_SETXOFFSET, static_cast<uptr_t>(xOffset), 0);
        Sci(SCI_SETREADONLY, tab.readOnly ? TRUE : FALSE, 0);
    }

    g_loadingTabContent = false;

    EnsureActiveTabVisible();
    UpdateWindowTitle();
    InvalidateTabBar();
    RefreshOpenFilesTree();
    InvalidateStatusBar();
    SetFocus(g_hSci);
}

void SwitchToTab(int tabIndex)
{
    if (tabIndex == g_activeTabIndex || tabIndex < 0 || tabIndex >= static_cast<int>(g_tabs.size()))
        return;

    CaptureActiveTab();
    LoadTabIntoEditor(tabIndex);
}

int FindOpenTabByPath(const std::wstring& path)
{
    for (int index = 0; index < static_cast<int>(g_tabs.size()); ++index)
    {
        if (!g_tabs[index].path.empty() &&
            CompareStringOrdinal(g_tabs[index].path.c_str(), -1, path.c_str(), -1, TRUE) == CSTR_EQUAL)
            return index;
    }
    return -1;
}

int AddDocumentTab(DocumentTab tab)
{
    CaptureActiveTab();
    g_tabs.push_back(std::move(tab));
    const int newIndex = static_cast<int>(g_tabs.size()) - 1;
    LoadTabIntoEditor(newIndex);
    return newIndex;
}

bool IsSingleEmptyUntitledTab()
{
    CaptureActiveTab();
    if (g_tabs.size() != 1)
        return false;

    const DocumentTab& tab = g_tabs[0];
    return tab.untitled &&
        tab.path.empty() &&
        tab.text.empty() &&
        tab.savedText.empty() &&
        !tab.modified;
}

bool HasFileTabs()
{
    for (const DocumentTab& tab : g_tabs)
    {
        if (!tab.untitled && !tab.path.empty())
            return true;
    }
    return false;
}

bool IsEmptyUntitledTab(const DocumentTab& tab)
{
    return tab.untitled &&
        tab.path.empty() &&
        tab.text.empty() &&
        tab.savedText.empty() &&
        !tab.modified;
}

void RemoveEmptyUntitledTabs()
{
    CaptureActiveTab();
    for (int index = static_cast<int>(g_tabs.size()) - 1; index >= 0; --index)
    {
        if (g_tabs.size() <= 1)
            break;

        if (!IsEmptyUntitledTab(g_tabs[index]))
            continue;

        g_tabs.erase(g_tabs.begin() + index);
        if (index < g_activeTabIndex)
            --g_activeTabIndex;
        else if (index == g_activeTabIndex)
            g_activeTabIndex = -1;
    }

    if (g_activeTabIndex < 0 && !g_tabs.empty())
        LoadTabIntoEditor((std::min)(static_cast<int>(g_tabs.size()) - 1, 0));
    else
    {
        UpdateWindowTitle();
        InvalidateTabBar();
        RefreshOpenFilesTree();
        InvalidateStatusBar();
    }
}

int AddDocumentTabReplacingDefaultBlank(DocumentTab tab)
{
    if (IsSingleEmptyUntitledTab())
    {
        g_tabs[0] = std::move(tab);
        g_activeTabIndex = -1;
        LoadTabIntoEditor(0);
        return 0;
    }

    return AddDocumentTab(std::move(tab));
}

int ClampFolderPaneWidth(int requestedWidth, int clientWidth)
{
    const int dynamicMaxWidth = (std::max)(kFolderPaneMinWidth, clientWidth - kEditorMinWidth - kFolderPaneDividerWidth);
    const int maxWidth = (std::min)(kFolderPaneMaxWidth, dynamicMaxWidth);
    return (std::min)((std::max)(requestedWidth, kFolderPaneMinWidth), maxWidth);
}

int GetEffectiveFolderPaneWidth()
{
    if (!g_folderPaneVisible)
        return 0;

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    return ClampFolderPaneWidth(g_folderPaneWidth, clientRect.right - clientRect.left);
}

int ClampOpenFilesPaneHeight(int requestedHeight, int contentHeight)
{
    if (contentHeight <= kFolderPaneSectionDividerHeight)
        return (std::max)(0, contentHeight);

    const int minUpper = (std::min)(kFolderPaneSectionMinHeight,
        (std::max)(0, contentHeight - kFolderPaneSectionDividerHeight));
    const int remainingAfterUpperMin = (std::max)(0, contentHeight - kFolderPaneSectionDividerHeight - minUpper);
    const int minLower = (std::min)(kFolderPaneSectionMinHeight, remainingAfterUpperMin);
    const int maxUpper = (std::max)(minUpper,
        (std::min)(kFolderPaneOpenFilesMaxHeight, contentHeight - kFolderPaneSectionDividerHeight - minLower));
    return (std::min)((std::max)(requestedHeight, minUpper), maxUpper);
}

int GetEffectiveOpenFilesPaneHeight(int contentHeight)
{
    return ClampOpenFilesPaneHeight(g_openFilesPaneHeight, contentHeight);
}

RECT GetFolderSplitterRect()
{
    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);

    const int paneWidth = GetEffectiveFolderPaneWidth();
    if (!g_folderPaneVisible || paneWidth <= 0)
        return RECT{ 0, 0, 0, 0 };

    const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
    const int bottom = (std::max)(0, clientHeight - kStatusBarHeight);
    return RECT{ paneWidth, 0, paneWidth + kFolderPaneDividerWidth, bottom };
}

RECT GetFolderSectionSplitterRect()
{
    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);

    const int paneWidth = GetEffectiveFolderPaneWidth();
    if (!g_folderPaneVisible || paneWidth <= 0)
        return RECT{ 0, 0, 0, 0 };

    const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
    const int contentHeight = (std::max)(0, clientHeight - kStatusBarHeight);
    const int openFilesHeight = GetEffectiveOpenFilesPaneHeight(contentHeight);
    return RECT{ 0, openFilesHeight, paneWidth, openFilesHeight + kFolderPaneSectionDividerHeight };
}

bool IsPointOnFolderSplitter(POINT point)
{
    RECT splitterRect = GetFolderSplitterRect();
    return !IsRectEmpty(&splitterRect) && PtInRect(&splitterRect, point);
}

bool IsPointOnFolderSectionSplitter(POINT point)
{
    RECT splitterRect = GetFolderSectionSplitterRect();
    return !IsRectEmpty(&splitterRect) && PtInRect(&splitterRect, point);
}

HWND EnsureFolderSplitterPreviewWindow()
{
    if (g_hFolderSplitterPreview && IsWindow(g_hFolderSplitterPreview))
        return g_hFolderSplitterPreview;

    const DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    g_hFolderSplitterPreview = CreateWindowExW(exStyle, kFolderSplitterPreviewClassName, L"",
        WS_POPUP, 0, 0, kFolderSplitterPreviewWindowWidth, 1,
        hWnd, nullptr, hInst, nullptr);

    if (g_hFolderSplitterPreview)
        SetLayeredWindowAttributes(g_hFolderSplitterPreview, kFolderSplitterPreviewBackColor, 0, LWA_COLORKEY);

    return g_hFolderSplitterPreview;
}

void ShowFolderSplitterPreview(int paneWidth)
{
    if (!hWnd || paneWidth < 0)
        return;

    HWND previewWindow = EnsureFolderSplitterPreviewWindow();
    if (!previewWindow)
        return;

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);
    const int clientHeight = static_cast<int>(clientRect.bottom - clientRect.top);
    const int previewHeight = (std::max)(0, clientHeight - kStatusBarHeight);
    if (previewHeight <= 0)
    {
        ShowWindow(previewWindow, SW_HIDE);
        return;
    }

    const int x = paneWidth + (kFolderPaneDividerWidth / 2);
    POINT previewOrigin{ x - (kFolderSplitterPreviewWindowWidth / 2), 0 };
    ClientToScreen(hWnd, &previewOrigin);

    SetWindowPos(previewWindow, HWND_TOP, previewOrigin.x, previewOrigin.y,
        kFolderSplitterPreviewWindowWidth, previewHeight,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
    InvalidateRect(previewWindow, nullptr, TRUE);
    UpdateWindow(previewWindow);
}

void HideFolderSplitterPreview()
{
    if (g_folderSplitterPreviewWidth >= 0)
    {
        g_folderSplitterPreviewWidth = -1;
    }

    if (g_hFolderSplitterPreview && IsWindow(g_hFolderSplitterPreview))
        ShowWindow(g_hFolderSplitterPreview, SW_HIDE);
}

void UpdateFolderSplitterPreview(int paneWidth)
{
    if (paneWidth == g_folderSplitterPreviewWidth)
        return;

    g_folderSplitterPreviewWidth = paneWidth;
    ShowFolderSplitterPreview(g_folderSplitterPreviewWidth);
}

void DrawFolderSplitter(HDC hdc)
{
    RECT splitterRect = GetFolderSplitterRect();
    if (IsRectEmpty(&splitterRect))
        return;

    HBRUSH background = CreateSolidBrush(ThemeFolderPaneBack());
    FillRect(hdc, &splitterRect, background);
    DeleteObject(background);

    const int centerX = splitterRect.left + (kFolderPaneDividerWidth / 2);
    HPEN linePen = CreatePen(PS_SOLID, 1, ThemeFolderPaneLine());
    HGDIOBJ oldPen = SelectObject(hdc, linePen);
    MoveToEx(hdc, centerX, splitterRect.top, nullptr);
    LineTo(hdc, centerX, splitterRect.bottom);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

void DrawFolderPaneHeader(HDC hdc, const RECT& headerRect, const wchar_t* title)
{
    if (IsRectEmpty(&headerRect))
        return;

    HBRUSH background = CreateSolidBrush(ThemeFolderPaneBack());
    FillRect(hdc, &headerRect, background);
    DeleteObject(background);

    HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ThemeFolderPaneText());

    RECT textRect = headerRect;
    textRect.left += kFolderPaneHeaderTextInset;
    textRect.right -= kFolderPaneHeaderTextInset;
    DrawTextW(hdc, title, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    SelectObject(hdc, oldFont);

    HPEN linePen = CreatePen(PS_SOLID, 1, ThemeFolderPaneLine());
    HGDIOBJ oldPen = SelectObject(hdc, linePen);
    MoveToEx(hdc, headerRect.left, headerRect.bottom - 1, nullptr);
    LineTo(hdc, headerRect.right, headerRect.bottom - 1);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

void DrawFolderSectionSplitter(HDC hdc)
{
    RECT splitterRect = GetFolderSectionSplitterRect();
    if (IsRectEmpty(&splitterRect))
        return;

    HBRUSH background = CreateSolidBrush(ThemeFolderPaneBack());
    FillRect(hdc, &splitterRect, background);
    DeleteObject(background);

    const int centerY = splitterRect.top + (kFolderPaneSectionDividerHeight / 2);
    HPEN linePen = CreatePen(PS_SOLID, 1, ThemeFolderPaneLine());
    HGDIOBJ oldPen = SelectObject(hdc, linePen);
    MoveToEx(hdc, splitterRect.left, centerY, nullptr);
    LineTo(hdc, splitterRect.right, centerY);
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

void DrawFolderPaneDecorations(HDC hdc, int paneWidth, int contentHeight)
{
    if (paneWidth <= 0 || contentHeight <= 0)
        return;

    const int openFilesHeight = GetEffectiveOpenFilesPaneHeight(contentHeight);
    RECT openHeader{ 0, 0, paneWidth, (std::min)(kFolderPaneHeaderHeight, openFilesHeight) };
    DrawFolderPaneHeader(hdc, openHeader, UiText(L"\u6253\u5F00\u7684\u6587\u4EF6", L"Open Files"));
    DrawFolderSectionSplitter(hdc);

    const int folderHeaderTop = (std::min)(contentHeight,
        openFilesHeight + kFolderPaneSectionDividerHeight);
    RECT folderHeader{ 0, folderHeaderTop, paneWidth,
        (std::min)(contentHeight, folderHeaderTop + kFolderPaneHeaderHeight) };
    DrawFolderPaneHeader(hdc, folderHeader, UiText(L"\u6253\u5F00\u7684\u6587\u4EF6\u5939", L"Open Folders"));
}

void FillMainClientBackground(HDC hdc)
{
    if (!hdc || !hWnd)
        return;

    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);

    const int clientHeight = clientRect.bottom - clientRect.top;
    const int contentBottom = (std::max)(0, clientHeight - kStatusBarHeight);
    const int paneWidth = GetEffectiveFolderPaneWidth();
    const int editorX = paneWidth > 0 ? paneWidth + kFolderPaneDividerWidth : 0;

    if (paneWidth > 0)
    {
        RECT folderRect{ 0, 0, paneWidth, contentBottom };
        HBRUSH folderBrush = CreateSolidBrush(ThemeFolderPaneBack());
        FillRect(hdc, &folderRect, folderBrush);
        DeleteObject(folderBrush);
        DrawFolderPaneDecorations(hdc, paneWidth, contentBottom);
    }

    if (editorX < clientRect.right)
    {
        RECT editorRect{ editorX, 0, clientRect.right, contentBottom };
        HBRUSH editorBrush = CreateSolidBrush(ThemeEditorBack());
        FillRect(hdc, &editorRect, editorBrush);
        DeleteObject(editorBrush);
    }

    if (contentBottom < clientRect.bottom)
    {
        RECT statusRect{ 0, contentBottom, clientRect.right, clientRect.bottom };
        HBRUSH statusBrush = CreateSolidBrush(ThemeStatusBack());
        FillRect(hdc, &statusRect, statusBrush);
        DeleteObject(statusBrush);
    }

    DrawFolderSplitter(hdc);
}

void LayoutChildWindows()
{
    RECT clientRect{};
    GetClientRect(hWnd, &clientRect);

    const int width = clientRect.right - clientRect.left;
    const int height = clientRect.bottom - clientRect.top;
    const int contentHeight = (std::max)(0, height - kStatusBarHeight);
    const int paneWidth = GetEffectiveFolderPaneWidth();
    const int editorX = paneWidth > 0 ? paneWidth + kFolderPaneDividerWidth : 0;
    const int editorWidth = (std::max)(0, width - editorX);
    const int openFilesHeight = GetEffectiveOpenFilesPaneHeight(contentHeight);
    const int openFilesTreeTop = (std::min)(contentHeight, kFolderPaneHeaderHeight);
    const int openFilesTreeHeight = (std::max)(0, openFilesHeight - openFilesTreeTop);
    const int folderHeaderTop = (std::min)(contentHeight, openFilesHeight + kFolderPaneSectionDividerHeight);
    const int folderTreeTop = (std::min)(contentHeight, folderHeaderTop + kFolderPaneHeaderHeight);
    const int folderTreeHeight = (std::max)(0, contentHeight - folderTreeTop);

    if (g_hOpenFilesTree)
    {
        ShowWindow(g_hOpenFilesTree, g_folderPaneVisible ? SW_SHOW : SW_HIDE);
        MoveWindow(g_hOpenFilesTree, 0, openFilesTreeTop, paneWidth, openFilesTreeHeight, TRUE);
    }

    if (g_hFolderTree)
    {
        ShowWindow(g_hFolderTree, g_folderPaneVisible ? SW_SHOW : SW_HIDE);
        MoveWindow(g_hFolderTree, 0, folderTreeTop, paneWidth, folderTreeHeight, TRUE);
    }

    if (g_hTabBar)
    {
        MoveWindow(g_hTabBar, editorX, 0, editorWidth, kTabBarHeight, TRUE);
    }

    if (g_hSci)
    {
        MoveWindow(g_hSci, editorX, kTabBarHeight, editorWidth, (std::max)(0, contentHeight - kTabBarHeight), TRUE);
    }

    if (g_hStatusBar)
    {
        MoveWindow(g_hStatusBar, 0, contentHeight, width, kStatusBarHeight, TRUE);
    }

    InvalidateRect(hWnd, nullptr, TRUE);
}

RECT GetFolderToggleButtonRect(HWND owner)
{
    RECT windowRect{};
    GetWindowRect(owner, &windowRect);

    const int frameX = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    const int frameY = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
    const int buttonWidth = GetSystemMetrics(SM_CXSIZE);
    const int buttonHeight = GetSystemMetrics(SM_CYSIZE);
    const int right = windowRect.right - frameX - (buttonWidth * 3);

    return RECT{ right - buttonWidth, windowRect.top + frameY, right, windowRect.top + frameY + buttonHeight };
}

void RedrawFolderToggleButton()
{
    RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
}

void DrawFolderToggleButton(HWND owner)
{
    if (IsIconic(owner))
        return;

    RECT buttonRect = GetFolderToggleButtonRect(owner);
    RECT windowRect{};
    GetWindowRect(owner, &windowRect);
    OffsetRect(&buttonRect, -windowRect.left, -windowRect.top);

    HDC hdc = GetWindowDC(owner);
    if (!hdc)
        return;

    HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    FillRect(hdc, &buttonRect, background);
    DeleteObject(background);
    FrameRect(hdc, &buttonRect, GetSysColorBrush(COLOR_3DSHADOW));

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
    DrawTextW(hdc, g_folderPaneVisible ? L"<<" : L">>", -1, &buttonRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    ReleaseDC(owner, hdc);
}

void SetFolderPaneVisible(bool visible)
{
    g_folderPaneVisible = visible;
    LayoutChildWindows();
    RedrawFolderToggleButton();
    InvalidateStatusBar();
    UpdateViewMenuCheck();
}

void ToggleFolderPane()
{
    SetFolderPaneVisible(!g_folderPaneVisible);
}

void ClearFolderPane()
{
    g_currentFolderPath.clear();
    g_folderItems.clear();
    g_restoreFolderInSession = false;
    if (g_hFolderTree)
        TreeView_DeleteAllItems(g_hFolderTree);
    SetFolderPaneVisible(HasOpenFilesTreeTabs());
    UpdateWindowTitle();
}




int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    const HRESULT coInitializeResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    SetCurrentProcessExplicitAppUserModelID(kAppUserModelId);
    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_TREEVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_HOTKEY_CLASS;
    InitCommonControlsEx(&commonControls);
    g_findReplaceMessage = RegisterWindowMessageW(FINDMSGSTRINGW);
    LoadAppSettings();
    EnableNativeDarkMenuMode(IsDarkTheme());

    // 初始化字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OPENEDIT, szWindowClass, MAX_LOADSTRING);

    // 必须先注册Scintilla窗口类
    Scintilla_RegisterClasses(hInstance);

    // 注册主窗口类
    MyRegisterClass(hInstance);

    // 初始化实例
    if (!InitInstance(hInstance, nCmdShow))
    {
        if (SUCCEEDED(coInitializeResult))
            CoUninitialize();
        return FALSE;
    }

    // 加载快捷键
    RebuildAcceleratorTable();

    // 消息循环
    MSG msg = { 0 };
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (g_hSettingsWindow && IsDialogMessageW(g_hSettingsWindow, &msg))
            continue;
        if (g_hAboutWindow && IsDialogMessageW(g_hAboutWindow, &msg))
            continue;

        if (g_findWindow && g_findWindow->IsVisible())
        {
            HWND findWindow = g_findWindow->Window();
            if (findWindow && (msg.hwnd == findWindow || IsChild(findWindow, msg.hwnd)))
            {
                if (!g_findWindow->TranslateDialogMessage(msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                continue;
            }
        }

        if (!g_hAccelTable || !TranslateAccelerator(hWnd, g_hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (g_hAccelTable)
    {
        DestroyAcceleratorTable(g_hAccelTable);
        g_hAccelTable = nullptr;
    }

    if (SUCCEEDED(coInitializeResult))
        CoUninitialize();

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW tabClass = { 0 };
    tabClass.cbSize = sizeof(WNDCLASSEX);
    tabClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    tabClass.lpfnWndProc = TabBarWndProc;
    tabClass.hInstance = hInstance;
    tabClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    tabClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    tabClass.lpszClassName = kTabBarClassName;
    RegisterClassExW(&tabClass);

    WNDCLASSEXW statusClass = { 0 };
    statusClass.cbSize = sizeof(WNDCLASSEX);
    statusClass.style = CS_HREDRAW | CS_VREDRAW;
    statusClass.lpfnWndProc = StatusBarWndProc;
    statusClass.hInstance = hInstance;
    statusClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    statusClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    statusClass.lpszClassName = kStatusBarClassName;
    RegisterClassExW(&statusClass);

    WNDCLASSEXW settingsClass = { 0 };
    settingsClass.cbSize = sizeof(WNDCLASSEX);
    settingsClass.style = CS_HREDRAW | CS_VREDRAW;
    settingsClass.lpfnWndProc = SettingsWndProc;
    settingsClass.hInstance = hInstance;
    settingsClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    settingsClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    settingsClass.lpszClassName = kSettingsWindowClassName;
    RegisterClassExW(&settingsClass);

    WNDCLASSEXW aboutClass = { 0 };
    aboutClass.cbSize = sizeof(WNDCLASSEX);
    aboutClass.style = CS_HREDRAW | CS_VREDRAW;
    aboutClass.lpfnWndProc = AboutWndProc;
    aboutClass.hInstance = hInstance;
    aboutClass.hIcon = LoadSharedAppIcon(hInstance, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    aboutClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    aboutClass.hbrBackground = nullptr;
    aboutClass.lpszClassName = kAboutWindowClassName;
    aboutClass.hIconSm = LoadSharedAppIcon(hInstance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    RegisterClassExW(&aboutClass);

    WNDCLASSEXW columnEditorClass = { 0 };
    columnEditorClass.cbSize = sizeof(WNDCLASSEX);
    columnEditorClass.style = CS_HREDRAW | CS_VREDRAW;
    columnEditorClass.lpfnWndProc = ColumnEditorWndProc;
    columnEditorClass.hInstance = hInstance;
    columnEditorClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    columnEditorClass.hbrBackground = nullptr;
    columnEditorClass.lpszClassName = kColumnEditorWindowClassName;
    RegisterClassExW(&columnEditorClass);

    WNDCLASSEXW splitterPreviewClass = { 0 };
    splitterPreviewClass.cbSize = sizeof(WNDCLASSEX);
    splitterPreviewClass.style = CS_HREDRAW | CS_VREDRAW;
    splitterPreviewClass.lpfnWndProc = FolderSplitterPreviewWndProc;
    splitterPreviewClass.hInstance = hInstance;
    splitterPreviewClass.hCursor = LoadCursor(nullptr, IDC_SIZEWE);
    splitterPreviewClass.hbrBackground = nullptr;
    splitterPreviewClass.lpszClassName = kFolderSplitterPreviewClassName;
    RegisterClassExW(&splitterPreviewClass);

    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadSharedAppIcon(hInstance, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_OPENEDIT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadSharedAppIcon(hInstance, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // 创建主窗口
    hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, 0, kDefaultMainWindowWidth, kDefaultMainWindowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) return FALSE;

    ApplyWindowAppIcons(hWnd);
    ConfigureTaskbarProperties(hWnd);

    if (!ApplyMainWindowPlacement(hWnd, nCmdShow))
        ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (g_findReplaceMessage && message == g_findReplaceMessage)
        return HandleFindReplaceMessage(lParam);

    switch (message)
    {
    case WM_MEASUREITEM:
    {
        MEASUREITEMSTRUCT* measureItem = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measureItem && measureItem->CtlType == ODT_MENU)
        {
            MeasureThemedMenuItem(measureItem);
            return TRUE;
        }
        break;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawItem && drawItem->CtlType == ODT_MENU)
        {
            DrawThemedMenuItem(drawItem);
            return TRUE;
        }
        break;
    }

    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        if (wmId == IDM_FILE_NEW)
        {
            NewFile();
        }
        else if (wmId == IDM_FILE_OPEN)
        {
            OpenFileCommand();
        }
        else if (wmId == IDM_FILE_OPEN_FOLDER)
        {
            OpenFolderCommand();
        }
        else if (wmId == IDM_FILE_SAVE)
        {
            SaveCurrentFile();
        }
        else if (wmId == IDM_FILE_SAVE_AS)
        {
            SaveCurrentFileAs();
        }
        else if (wmId == IDM_FILE_CLOSE_FOLDER)
        {
            ClearFolderPane();
        }
        else if (wmId == IDM_FOLDER_REFRESH)
        {
            RefreshFolderTreeItem(GetFolderTreeSelection());
        }
        else if (wmId == IDM_FOLDER_COPY_NAME)
        {
            CopySelectedFolderItemName();
        }
        else if (wmId == IDM_FOLDER_OPEN_EXPLORER)
        {
            OpenSelectedFolderItemInExplorer();
        }
        else if (wmId == IDM_TAB_CLOSE)
        {
            CloseTab(g_contextTabIndex >= 0 ? g_contextTabIndex : g_activeTabIndex);
            g_contextTabIndex = -1;
        }
        else if (wmId == IDM_TAB_CLOSE_OTHERS)
        {
            const int keepIndex = g_contextTabIndex >= 0 ? g_contextTabIndex : g_activeTabIndex;
            for (int index = static_cast<int>(g_tabs.size()) - 1; index >= 0; --index)
            {
                if (index != keepIndex)
                    CloseTab(index);
            }
            g_contextTabIndex = -1;
        }
        else if (wmId == IDM_TAB_CLOSE_ALL)
        {
            while (!g_tabs.empty())
            {
                const int previousCount = static_cast<int>(g_tabs.size());
                CloseTab(0);
                if (static_cast<int>(g_tabs.size()) == previousCount)
                    break;
                if (g_tabs.size() == 1 && g_tabs[0].untitled && !g_tabs[0].modified && g_tabs[0].text.empty())
                    break;
            }
            g_contextTabIndex = -1;
        }
        else if (IsEditCommand(wmId))
        {
            ExecuteEditCommand(wmId);
        }
        else if (wmId == IDM_SEARCH_FIND)
        {
            OpenFindReplaceDialog(false);
        }
        else if (wmId == IDM_SEARCH_FIND_NEXT)
        {
            FindNextCommand(true);
        }
        else if (wmId == IDM_SEARCH_FIND_PREVIOUS)
        {
            FindNextCommand(false);
        }
        else if (wmId == IDM_SEARCH_REPLACE)
        {
            OpenFindReplaceDialog(true);
        }
        else if (wmId >= IDM_ENCODING_UTF8 && wmId <= IDM_ENCODING_UTF16_BE)
        {
            SetActiveDocumentEncoding(EncodingFromCommand(wmId));
        }
        else if (wmId >= IDM_EOL_WINDOWS && wmId <= IDM_EOL_MAC)
        {
            SetActiveDocumentEolMode(EolModeFromCommand(wmId));
        }
        else if (wmId == IDM_VIEW_SHOW_SPACE_TAB)
        {
            ToggleShowSpaceAndTab();
        }
        else if (wmId == IDM_VIEW_SHOW_EOL)
        {
            ToggleShowEndOfLine();
        }
        else if (wmId == IDM_VIEW_SHOW_ALL_CHARS)
        {
            ToggleShowAllCharacters();
        }
        else if (wmId == IDM_VIEW_WORD_WRAP)
        {
            ToggleWordWrap();
        }
        else if (wmId == IDM_VIEW_TOGGLE_FOLDER)
        {
            ToggleFolderPane();
        }
        else if (IsLanguageCommand(wmId))
        {
            ApplyLanguage(wmId);
        }
        else if (wmId == IDM_SETTINGS_OPEN)
        {
            ShowSettingsWindow();
        }
        else if (wmId == IDM_ABOUT)
        {
            ShowAboutWindow();
        }
        else if (wmId == IDM_EXIT)
        {
            SendMessageW(hWnd, WM_CLOSE, 0, 0);
        }
        else
        {
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_ERASEBKGND:
        FillMainClientBackground(reinterpret_cast<HDC>(wParam));
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        FillMainClientBackground(hdc);
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_NOTIFY:
    {
        const NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
        if (header && header->hwndFrom == g_hOpenFilesTree)
        {
            if (header->code == NM_CUSTOMDRAW)
            {
                return HandleFolderTreeCustomDraw(lParam);
            }
            else if (header->code == NM_CLICK)
            {
                DWORD cursorPosition = GetMessagePos();
                POINT clientPoint{ GET_X_LPARAM(cursorPosition), GET_Y_LPARAM(cursorPosition) };
                ScreenToClient(g_hOpenFilesTree, &clientPoint);
                HandleOpenFilesTreeItemClickAt(clientPoint);
                return 0;
            }
        }
        else if (header && header->hwndFrom == g_hFolderTree)
        {
            if (header->code == NM_CUSTOMDRAW)
            {
                return HandleFolderTreeCustomDraw(lParam);
            }
            else if (header->code == TVN_ITEMEXPANDINGW)
            {
                NMTREEVIEWW* treeView = reinterpret_cast<NMTREEVIEWW*>(lParam);
                if (treeView->action == TVE_EXPAND)
                    LoadFolderNodeChildren(treeView->itemNew.hItem, reinterpret_cast<FolderItem*>(treeView->itemNew.lParam));
            }
            else if (header->code == TVN_ITEMEXPANDEDW)
            {
                NMTREEVIEWW* treeView = reinterpret_cast<NMTREEVIEWW*>(lParam);
                UpdateFolderNodeIcon(treeView->itemNew.hItem, treeView->action == TVE_EXPAND);
            }
            else if (header->code == NM_CLICK)
            {
                HandleFolderTreeSingleClick();
                return 0;
            }
            else if (header->code == NM_RCLICK)
            {
                DWORD cursorPosition = GetMessagePos();
                POINT screenPoint{ GET_X_LPARAM(cursorPosition), GET_Y_LPARAM(cursorPosition) };
                ShowFolderTreeContextMenu(screenPoint);
                return 1;
            }
        }
        else if (header && header->hwndFrom == g_hSci && !g_loadingTabContent)
        {
            if (header->code == SCN_MODIFYATTEMPTRO)
            {
                ShowReadOnlyWarning(hWnd);
            }
            else if (header->code == SCN_SAVEPOINTLEFT)
            {
                SetActiveTabModified(true);
                InvalidateStatusBar();
            }
            else if (header->code == SCN_SAVEPOINTREACHED)
            {
                CaptureActiveTab();
                InvalidateTabBar();
                RefreshOpenFilesTree();
                InvalidateStatusBar();
            }
            else if (header->code == SCN_MODIFIED)
            {
                const SCNotification* notification = reinterpret_cast<SCNotification*>(lParam);
                if (notification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
                {
                    ClearWordHighlights();
                    SetActiveTabModified(true);
                }
                InvalidateStatusBar();
            }
            else if (header->code == SCN_UPDATEUI)
            {
                if (!g_highlightedWord.empty() && Sci(SCI_GETSELECTIONSTART) == Sci(SCI_GETSELECTIONEND))
                    ClearWordHighlights();
                InvalidateStatusBar();
            }
            else if (header->code == SCN_DOUBLECLICK)
            {
                HighlightSelectedWordOccurrences();
            }
            else if (header->code == SCN_MARGINCLICK)
            {
                HandleEditorMarginClick(reinterpret_cast<const SCNotification*>(lParam));
            }
        }
        return 0;
    }

    case WM_CREATE:
    {
        ::hWnd = hWnd;
        DragAcceptFiles(hWnd, TRUE);
        ApplyWindowChromeTheme(hWnd);
        UpdateMainMenuText();

        g_hOpenFilesTree = CreateWindowExW(
            0,
            WC_TREEVIEWW,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | TVS_SHOWSELALWAYS | TVS_NOHSCROLL,
            0, 0, 0, 0,
            hWnd,
            reinterpret_cast<HMENU>(IDC_OPEN_FILES_TREE),
            hInst,
            nullptr
        );

        if (g_hOpenFilesTree)
        {
            DragAcceptFiles(g_hOpenFilesTree, TRUE);
            TreeView_SetExtendedStyle(g_hOpenFilesTree, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            g_originalOpenFilesTreeProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(g_hOpenFilesTree, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OpenFilesTreeWndProc)));
            SendMessageW(g_hOpenFilesTree, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            ApplyFolderTreeTheme();
        }

        g_hFolderTree = CreateWindowExW(
            0,
            WC_TREEVIEWW,
            L"",
            WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT |
                TVS_SHOWSELALWAYS | TVS_NOHSCROLL,
            0, 0, 0, 0,
            hWnd,
            reinterpret_cast<HMENU>(IDC_FOLDER_TREE),
            hInst,
            nullptr
        );

        if (g_hFolderTree)
        {
            DragAcceptFiles(g_hFolderTree, TRUE);
            TreeView_SetExtendedStyle(g_hFolderTree, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            g_originalFolderTreeProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(g_hFolderTree, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(FolderTreeWndProc)));
            SendMessageW(g_hFolderTree, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
            ApplyFolderTreeTheme();
            InitializeFolderTreeImageList();
        }

        g_hTabBar = CreateWindowExW(
            0,
            kTabBarClassName,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 0, 0,
            hWnd,
            reinterpret_cast<HMENU>(IDC_TAB_BAR),
            hInst,
            nullptr
        );

        if (g_hTabBar)
        {
            DragAcceptFiles(g_hTabBar, TRUE);
            SendMessageW(g_hTabBar, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        }

        g_hStatusBar = CreateWindowExW(
            0,
            kStatusBarClassName,
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            0, 0, 0, 0,
            hWnd,
            reinterpret_cast<HMENU>(IDC_STATUS_BAR),
            hInst,
            nullptr
        );

        if (g_hStatusBar)
        {
            DragAcceptFiles(g_hStatusBar, TRUE);
            SendMessageW(g_hStatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        }

        // 创建Scintilla子窗口
        g_hSci = CreateWindowExW(
            0,
            L"Scintilla",
            L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP,
            0, 0, 0, 0,
            hWnd,
            nullptr,
            hInst,
            nullptr
        );

        if (g_hSci)
        {
            DragAcceptFiles(g_hSci, TRUE);
            g_originalEditorProc = reinterpret_cast<WNDPROC>(
                SetWindowLongPtrW(g_hSci, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditorWndProc)));
            ApplyControlTheme(g_hSci);
            g_pSciFn = reinterpret_cast<SciFnDirect>(SendMessage(g_hSci, SCI_GETDIRECTFUNCTION, 0, 0));
            g_pSciPtr = static_cast<sptr_t>(SendMessage(g_hSci, SCI_GETDIRECTPOINTER, 0, 0));

            InitScintillaEditor();
            if (!LoadStartupTabs())
                AddDocumentTab(CreateDefaultUntitledTab());
            RefreshOpenFilesTree();
            LayoutChildWindows();
            SetFocus(g_hSci);
        }
        return 0;
    }

    case WM_SIZE:
    {
        LayoutChildWindows();
        RedrawFolderToggleButton();
        return 0;
    }

    case WM_SETCURSOR:
    {
        if (LOWORD(lParam) == HTCLIENT && g_folderPaneVisible)
        {
            POINT point{};
            GetCursorPos(&point);
            ScreenToClient(hWnd, &point);
            if (g_draggingFolderSectionSplitter || IsPointOnFolderSectionSplitter(point))
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                return TRUE;
            }
            if (g_draggingFolderSplitter || IsPointOnFolderSplitter(point))
            {
                SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                return TRUE;
            }
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_CONTEXTMENU:
        if (reinterpret_cast<HWND>(wParam) == g_hFolderTree)
        {
            POINT screenPoint{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (screenPoint.x == -1 && screenPoint.y == -1)
            {
                HTREEITEM selected = GetFolderTreeSelection();
                RECT itemRect{};
                if (selected && TreeView_GetItemRect(g_hFolderTree, selected, &itemRect, TRUE))
                {
                    screenPoint.x = itemRect.left;
                    screenPoint.y = itemRect.bottom;
                    ClientToScreen(g_hFolderTree, &screenPoint);
                }
                else
                {
                    GetCursorPos(&screenPoint);
                }
            }
            ShowFolderTreeContextMenu(screenPoint);
            return 0;
        }
        break;

    case WM_LBUTTONDOWN:
    {
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (g_folderPaneVisible && IsPointOnFolderSectionSplitter(point))
        {
            g_draggingFolderSectionSplitter = true;
            SetCapture(hWnd);
            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
            return 0;
        }
        if (g_folderPaneVisible && IsPointOnFolderSplitter(point))
        {
            g_draggingFolderSplitter = true;
            UpdateFolderSplitterPreview(GetEffectiveFolderPaneWidth());
            SetCapture(hWnd);
            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE:
        if (g_draggingFolderSectionSplitter)
        {
            RECT clientRect{};
            GetClientRect(hWnd, &clientRect);
            const int clientHeight = clientRect.bottom - clientRect.top;
            const int contentHeight = (std::max)(0, clientHeight - kStatusBarHeight);
            const int requestedHeight = GET_Y_LPARAM(lParam);
            const int newHeight = ClampOpenFilesPaneHeight(requestedHeight, contentHeight);
            if (newHeight != g_openFilesPaneHeight)
            {
                g_openFilesPaneHeight = newHeight;
                LayoutChildWindows();
            }
            return 0;
        }
        if (g_draggingFolderSplitter)
        {
            RECT clientRect{};
            GetClientRect(hWnd, &clientRect);
            const int clientWidth = clientRect.right - clientRect.left;
            const int requestedWidth = GET_X_LPARAM(lParam);
            const int newWidth = ClampFolderPaneWidth(requestedWidth, clientWidth);
            UpdateFolderSplitterPreview(newWidth);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (g_draggingFolderSectionSplitter)
        {
            g_draggingFolderSectionSplitter = false;
            ReleaseCapture();
            LayoutChildWindows();
            return 0;
        }
        if (g_draggingFolderSplitter)
        {
            const int newWidth = g_folderSplitterPreviewWidth >= 0 ? g_folderSplitterPreviewWidth : g_folderPaneWidth;
            HideFolderSplitterPreview();
            g_draggingFolderSplitter = false;
            ReleaseCapture();
            if (newWidth != g_folderPaneWidth)
            {
                g_folderPaneWidth = newWidth;
                LayoutChildWindows();
            }
            return 0;
        }
        break;

    case WM_CAPTURECHANGED:
        HideFolderSplitterPreview();
        g_draggingFolderSplitter = false;
        g_draggingFolderSectionSplitter = false;
        break;

    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
        POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT buttonRect = GetFolderToggleButtonRect(hWnd);
        if (PtInRect(&buttonRect, point))
            return HTFOLDERTOGGLE;
        return hit;
    }

    case WM_NCLBUTTONDOWN:
        if (wParam == HTFOLDERTOGGLE)
        {
            ToggleFolderPane();
            return 0;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCLBUTTONUP:
        if (wParam == HTFOLDERTOGGLE)
            return 0;
        return DefWindowProc(hWnd, message, wParam, lParam);

    case WM_NCPAINT:
    {
        LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);
        DrawFolderToggleButton(hWnd);
        return result;
    }

    case WM_NCACTIVATE:
    {
        LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);
        DrawFolderToggleButton(hWnd);
        return result;
    }

    case WM_SETTEXT:
    {
        LRESULT result = DefWindowProc(hWnd, message, wParam, lParam);
        DrawFolderToggleButton(hWnd);
        return result;
    }

    case WM_CLOSE:
        if (PromptSaveAllTabs())
        {
            SaveAppSettings();
            SaveSessionState();
            DestroyWindow(hWnd);
        }
        return 0;

    case WM_DESTROY:
        if (g_hFolderTreeImageList)
        {
            if (g_hOpenFilesTree && IsWindow(g_hOpenFilesTree))
                TreeView_SetImageList(g_hOpenFilesTree, nullptr, TVSIL_NORMAL);
            if (g_hFolderTree && IsWindow(g_hFolderTree))
                TreeView_SetImageList(g_hFolderTree, nullptr, TVSIL_NORMAL);
            ImageList_Destroy(g_hFolderTreeImageList);
            g_hFolderTreeImageList = nullptr;
        }
        if (g_hFolderSplitterPreview && IsWindow(g_hFolderSplitterPreview))
        {
            DestroyWindow(g_hFolderSplitterPreview);
            g_hFolderSplitterPreview = nullptr;
        }
        if (g_hAboutWindow && IsWindow(g_hAboutWindow))
        {
            DestroyWindow(g_hAboutWindow);
            g_hAboutWindow = nullptr;
        }
        g_findWindow.reset();
        if (g_hTabTooltip && IsWindow(g_hTabTooltip))
        {
            DestroyWindow(g_hTabTooltip);
            g_hTabTooltip = nullptr;
        }
        DeleteObject(g_hPopupBackBrush);
        DeleteObject(g_hPopupSurfaceBrush);
        DeleteObject(g_hPopupInputBrush);
        DeleteObject(g_hMenuBackBrush);
        g_hPopupBackBrush = nullptr;
        g_hPopupSurfaceBrush = nullptr;
        g_hPopupInputBrush = nullptr;
        g_hMenuBackBrush = nullptr;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

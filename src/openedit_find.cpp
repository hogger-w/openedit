#include "openedit_internal.h"

int GetScintillaSearchFlags(DWORD findOptions)
{
    int flags = SCFIND_NONE;
    if (findOptions & FR_MATCHCASE)
        flags |= SCFIND_MATCHCASE;
    if (findOptions & FR_WHOLEWORD)
        flags |= SCFIND_WHOLEWORD;
    if (findOptions & kFindOptionRegex)
        flags |= SCFIND_REGEXP | SCFIND_CXX11REGEX;
    return flags;
}

sptr_t SearchTargetRange(sptr_t start, sptr_t end, const std::string& needle, DWORD findOptions)
{
    Sci(SCI_SETSEARCHFLAGS, GetScintillaSearchFlags(findOptions), 0);
    Sci(SCI_SETTARGETRANGE, static_cast<uptr_t>(start), end);
    return Sci(SCI_SEARCHINTARGET, static_cast<uptr_t>(needle.size()), reinterpret_cast<sptr_t>(needle.c_str()));
}

std::string GetEditorRangeText(sptr_t start, sptr_t end)
{
    if (!g_hSci || end <= start)
        return {};

    std::string text(static_cast<size_t>(end - start), '\0');
    std::vector<char> buffer(static_cast<size_t>(end - start) + 1, '\0');
    Sci_TextRange range{};
    range.chrg.cpMin = static_cast<Sci_PositionCR>(start);
    range.chrg.cpMax = static_cast<Sci_PositionCR>(end);
    range.lpstrText = buffer.data();
    Sci(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&range));
    text.assign(buffer.data(), static_cast<size_t>(end - start));
    return text;
}

bool IsAsciiWordText(const std::string& text)
{
    if (text.empty())
        return false;

    for (unsigned char ch : text)
    {
        if (!std::isalnum(ch) && ch != '_')
            return false;
    }
    return true;
}

void ClearWordHighlights()
{
    if (!g_hSci)
        return;

    Sci(SCI_SETINDICATORCURRENT, kWordHighlightIndicator, 0);
    Sci(SCI_INDICATORCLEARRANGE, 0, Sci(SCI_GETTEXTLENGTH));
    g_highlightedWord.clear();
}

void HighlightWordOccurrences(const std::string& word)
{
    ClearWordHighlights();
    if (word.empty())
        return;

    g_highlightedWord = word;
    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);
    sptr_t start = 0;
    Sci(SCI_SETINDICATORCURRENT, kWordHighlightIndicator, 0);
    Sci(SCI_SETINDICATORVALUE, 1, 0);

    while (start < documentLength)
    {
        const sptr_t result = SearchTargetRange(start, documentLength, word, FR_MATCHCASE | FR_WHOLEWORD);
        if (result < 0)
            break;

        const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
        const sptr_t targetEnd = Sci(SCI_GETTARGETEND);
        if (targetEnd <= targetStart)
            break;

        Sci(SCI_INDICATORFILLRANGE, static_cast<uptr_t>(targetStart), targetEnd - targetStart);
        start = targetEnd;
    }
}

void HighlightSelectedWordOccurrences()
{
    const sptr_t selectionStart = Sci(SCI_GETSELECTIONSTART);
    const sptr_t selectionEnd = Sci(SCI_GETSELECTIONEND);
    if (selectionEnd <= selectionStart)
    {
        ClearWordHighlights();
        return;
    }

    const sptr_t wordStart = Sci(SCI_WORDSTARTPOSITION, static_cast<uptr_t>(selectionStart), TRUE);
    const sptr_t wordEnd = Sci(SCI_WORDENDPOSITION, static_cast<uptr_t>(selectionStart), TRUE);
    if (wordStart != selectionStart || wordEnd != selectionEnd)
    {
        ClearWordHighlights();
        return;
    }

    const std::string word = GetEditorRangeText(selectionStart, selectionEnd);
    if (!IsAsciiWordText(word))
    {
        ClearWordHighlights();
        return;
    }

    HighlightWordOccurrences(word);
}

bool SelectCurrentTarget(bool focusEditor)
{
    sptr_t targetStart = Sci(SCI_GETTARGETSTART);
    sptr_t targetEnd = Sci(SCI_GETTARGETEND);
    if (targetStart < 0 || targetEnd < 0)
        return false;

    const sptr_t selectionStart = (std::min)(targetStart, targetEnd);
    const sptr_t selectionEnd = (std::max)(targetStart, targetEnd);
    Sci(SCI_SETSEL, static_cast<uptr_t>(selectionStart), selectionEnd);
    Sci(SCI_SCROLLCARET);
    if (focusEditor)
        SetFocus(g_hSci);
    return true;
}

bool FindTextInEditor(const wchar_t* findText, DWORD findOptions, bool searchDown, bool wrap, bool focusEditor)
{
    const std::string needle = WideToUtf8(findText ? findText : L"");
    if (needle.empty())
        return false;

    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);
    const sptr_t selectionStart = Sci(SCI_GETSELECTIONSTART);
    const sptr_t selectionEnd = Sci(SCI_GETSELECTIONEND);
    sptr_t result = -1;

    if (searchDown)
    {
        result = SearchTargetRange(selectionEnd, documentLength, needle, findOptions);
        if (result < 0 && wrap)
            result = SearchTargetRange(0, selectionStart, needle, findOptions);
    }
    else
    {
        result = SearchTargetRange(selectionStart, 0, needle, findOptions);
        if (result < 0 && wrap)
            result = SearchTargetRange(documentLength, selectionEnd, needle, findOptions);
    }

    if (result < 0)
    {
        MessageBeep(MB_ICONINFORMATION);
        return false;
    }

    return SelectCurrentTarget(focusEditor);
}

bool SelectionMatchesSearch(const std::string& needle, DWORD findOptions)
{
    const sptr_t selectionStart = Sci(SCI_GETSELECTIONSTART);
    const sptr_t selectionEnd = Sci(SCI_GETSELECTIONEND);
    if (selectionStart == selectionEnd)
        return false;

    const sptr_t start = (std::min)(selectionStart, selectionEnd);
    const sptr_t end = (std::max)(selectionStart, selectionEnd);
    const sptr_t result = SearchTargetRange(start, end, needle, findOptions);
    return result == start && Sci(SCI_GETTARGETSTART) == start && Sci(SCI_GETTARGETEND) == end;
}

bool ReplaceCurrentSelection(const wchar_t* findText, const wchar_t* replaceText, DWORD findOptions)
{
    const std::string needle = WideToUtf8(findText ? findText : L"");
    if (needle.empty() || !SelectionMatchesSearch(needle, findOptions))
        return false;

    const std::string replacement = WideToUtf8(replaceText ? replaceText : L"");
    const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
    const unsigned int replaceMessage = (findOptions & kFindOptionRegex) ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
    const sptr_t replacementLength = Sci(replaceMessage,
        static_cast<uptr_t>(replacement.size()), reinterpret_cast<sptr_t>(replacement.c_str()));
    if (replacementLength >= 0)
        Sci(SCI_SETSEL, static_cast<uptr_t>(targetStart), targetStart + replacementLength);
    SetActiveTabModified(true);
    return true;
}

sptr_t PositionAfter(sptr_t position)
{
    const sptr_t next = Sci(SCI_POSITIONAFTER, static_cast<uptr_t>(position), 0);
    return next > position ? next : position + 1;
}

int ReplaceAllMatches(const wchar_t* findText, const wchar_t* replaceText, DWORD findOptions)
{
    const std::string needle = WideToUtf8(findText ? findText : L"");
    if (needle.empty())
        return 0;

    const std::string replacement = WideToUtf8(replaceText ? replaceText : L"");
    int replacements = 0;
    sptr_t start = 0;
    sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);

    Sci(SCI_BEGINUNDOACTION);
    while (start <= documentLength)
    {
        const sptr_t result = SearchTargetRange(start, documentLength, needle, findOptions);
        if (result < 0)
            break;

        const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
        const sptr_t targetEnd = Sci(SCI_GETTARGETEND);
        const unsigned int replaceMessage = (findOptions & kFindOptionRegex) ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
        const sptr_t replacementLength = Sci(replaceMessage,
            static_cast<uptr_t>(replacement.size()), reinterpret_cast<sptr_t>(replacement.c_str()));
        ++replacements;

        documentLength = Sci(SCI_GETTEXTLENGTH);
        start = targetStart + (replacementLength > 0 ? replacementLength : 0);
        if (start <= targetStart || targetEnd == targetStart)
            start = PositionAfter(targetStart);
    }
    Sci(SCI_ENDUNDOACTION);

    if (replacements > 0)
        SetActiveTabModified(true);
    else
        MessageBeep(MB_ICONINFORMATION);

    return replacements;
}

int CountMatches(const wchar_t* findText, DWORD findOptions)
{
    const std::string needle = WideToUtf8(findText ? findText : L"");
    if (needle.empty())
        return 0;

    int count = 0;
    sptr_t start = 0;
    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);

    while (start <= documentLength)
    {
        const sptr_t result = SearchTargetRange(start, documentLength, needle, findOptions);
        if (result < 0)
            break;

        ++count;
        const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
        const sptr_t targetEnd = Sci(SCI_GETTARGETEND);
        start = targetEnd > targetStart ? targetEnd : PositionAfter(targetStart);
    }

    return count;
}

void CopySearchText(wchar_t* destination, const std::wstring& source)
{
    if (!destination)
        return;

    wcsncpy_s(destination, kSearchTextBufferLength, source.c_str(), _TRUNCATE);
}

DWORD FindOptionsFromRequest(const OpenEditFindRequest& request, bool searchDown)
{
    DWORD options = 0;
    if (request.matchCase)
        options |= FR_MATCHCASE;
    if (request.regex)
        options |= kFindOptionRegex;
    if (searchDown)
        options |= FR_DOWN;
    return options;
}

void RememberFindRequest(const OpenEditFindRequest& request, bool searchDown)
{
    CopySearchText(g_findText, request.findText);
    CopySearchText(g_replaceText, request.replaceText);
    g_lastFindOptions = FindOptionsFromRequest(request, searchDown);
}

bool FindWindowFind(void*, const OpenEditFindRequest& request, bool previous)
{
    const bool searchDown = previous ? request.reverse : !request.reverse;
    RememberFindRequest(request, searchDown);
    return FindTextInEditor(g_findText, g_lastFindOptions, searchDown, request.wrap, false);
}

int FindWindowCount(void*, const OpenEditFindRequest& request)
{
    RememberFindRequest(request, true);
    return CountMatches(g_findText, g_lastFindOptions);
}

bool FindWindowReplace(void*, const OpenEditFindRequest& request)
{
    const bool searchDown = !request.reverse;
    RememberFindRequest(request, searchDown);
    const bool replaced = ReplaceCurrentSelection(g_findText, g_replaceText, g_lastFindOptions);
    if (replaced)
        FindTextInEditor(g_findText, g_lastFindOptions, searchDown, request.wrap, false);
    return replaced;
}

int FindWindowReplaceAll(void*, const OpenEditFindRequest& request)
{
    RememberFindRequest(request, true);
    return ReplaceAllMatches(g_findText, g_replaceText, g_lastFindOptions);
}

std::wstring GetSelectedTextForFind()
{
    if (!g_hSci)
        return L"";

    const sptr_t byteLength = Sci(SCI_GETSELTEXT, 0, 0);
    if (byteLength <= 1 || byteLength > kSearchTextBufferLength)
        return L"";

    std::vector<char> selectedText(static_cast<size_t>(byteLength), '\0');
    Sci(SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(selectedText.data()));
    return Utf8ToWide(std::string(selectedText.data()));
}

void OpenFindReplaceDialog(bool replaceDialog)
{
    if (!g_findWindow)
        g_findWindow = std::make_unique<OpenEditFindWindow>(hInst);

    if (g_findText[0] == L'\0')
    {
        const std::wstring selectedText = GetSelectedTextForFind();
        if (!selectedText.empty())
            CopySearchText(g_findText, selectedText);
    }

    OpenEditFindWindow::Callbacks callbacks{};
    callbacks.find = FindWindowFind;
    callbacks.count = FindWindowCount;
    callbacks.replace = FindWindowReplace;
    callbacks.replaceAll = FindWindowReplaceAll;

    g_findWindow->Show(hWnd, replaceDialog, g_findText, g_replaceText,
        IsDarkTheme(), g_appLanguage == AppLanguage::Chinese, callbacks);
}

void FindNextCommand(bool searchDown)
{
    if (g_findText[0] == L'\0')
    {
        OpenFindReplaceDialog(false);
        return;
    }

    DWORD options = g_lastFindOptions & (FR_MATCHCASE | FR_WHOLEWORD | kFindOptionRegex);
    if (searchDown)
        options |= FR_DOWN;
    FindTextInEditor(g_findText, options, searchDown, true);
}

LRESULT HandleFindReplaceMessage(LPARAM lParam)
{
    FINDREPLACEW* findReplace = reinterpret_cast<FINDREPLACEW*>(lParam);
    if (!findReplace)
        return 0;

    if (findReplace->Flags & FR_DIALOGTERM)
    {
        g_hFindReplaceDialog = nullptr;
        return 0;
    }

    g_lastFindOptions = findReplace->Flags & kFindOptionMask;
    const bool searchDown = (findReplace->Flags & FR_DOWN) != 0;

    if (findReplace->Flags & FR_FINDNEXT)
    {
        FindTextInEditor(findReplace->lpstrFindWhat, g_lastFindOptions, searchDown, true);
    }
    else if (findReplace->Flags & FR_REPLACE)
    {
        ReplaceCurrentSelection(findReplace->lpstrFindWhat, findReplace->lpstrReplaceWith, g_lastFindOptions);
        FindTextInEditor(findReplace->lpstrFindWhat, g_lastFindOptions, searchDown, true);
    }
    else if (findReplace->Flags & FR_REPLACEALL)
    {
        ReplaceAllMatches(findReplace->lpstrFindWhat, findReplace->lpstrReplaceWith, g_lastFindOptions);
    }

    return 0;
}

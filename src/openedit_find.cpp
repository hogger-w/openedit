#include "openedit_internal.h"

#include <regex>

namespace
{
struct FindWindowEditorState
{
    bool active = false;
    bool caretLineVisibleAlways = false;
};

FindWindowEditorState g_findWindowEditorState;

void BeginFindWindowEditorState()
{
    if (!g_hSci || g_findWindowEditorState.active)
        return;

    g_findWindowEditorState.active = true;
    g_findWindowEditorState.caretLineVisibleAlways = Sci(SCI_GETCARETLINEVISIBLEALWAYS) != 0;
    Sci(SCI_SETCARETLINEVISIBLEALWAYS, TRUE, 0);
}

void EndFindWindowEditorState()
{
    if (!g_findWindowEditorState.active)
        return;

    const FindWindowEditorState state = g_findWindowEditorState;
    g_findWindowEditorState = {};

    if (!g_hSci)
        return;

    Sci(SCI_SETCARETLINEVISIBLEALWAYS, state.caretLineVisibleAlways ? TRUE : FALSE, 0);
    SetFocus(g_hSci);
}
}

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

std::string DecodeRegexLineEndingEscapes(const std::string& pattern)
{
    std::string decoded;
    decoded.reserve(pattern.size());

    for (size_t i = 0; i < pattern.size(); ++i)
    {
        if (pattern[i] != '\\' || i + 1 >= pattern.size())
        {
            decoded.push_back(pattern[i]);
            continue;
        }

        const char next = pattern[i + 1];
        if (next == 'r')
        {
            decoded.push_back('\r');
            ++i;
        }
        else if (next == 'n')
        {
            decoded.push_back('\n');
            ++i;
        }
        else
        {
            decoded.push_back(pattern[i]);
            decoded.push_back(next);
            ++i;
        }
    }

    return decoded;
}

std::string PrepareFindText(const wchar_t* findText, DWORD findOptions)
{
    std::string needle = WideToUtf8(findText ? findText : L"");
    if (findOptions & kFindOptionRegex)
        needle = DecodeRegexLineEndingEscapes(needle);
    return needle;
}

bool IsRegexLineEndingLiteral(const std::string& needle, DWORD findOptions)
{
    if (!(findOptions & kFindOptionRegex))
        return false;

    return needle == "\r\n" || needle == "\n" || needle == "\r";
}

bool UseScintillaRegexReplacement(const std::string& needle, DWORD findOptions)
{
    return (findOptions & kFindOptionRegex) && !IsRegexLineEndingLiteral(needle, findOptions);
}

sptr_t SearchTargetRange(sptr_t start, sptr_t end, const std::string& needle, DWORD findOptions)
{
    DWORD effectiveFindOptions = findOptions;
    if (IsRegexLineEndingLiteral(needle, findOptions))
        effectiveFindOptions &= ~kFindOptionRegex;

    Sci(SCI_SETSEARCHFLAGS, GetScintillaSearchFlags(effectiveFindOptions), 0);
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

bool UseFullDocumentRegex(DWORD findOptions)
{
    return (findOptions & kFindOptionRegex) && (findOptions & kFindOptionFullDocument);
}

size_t ClampRegexOffset(sptr_t position, size_t length)
{
    if (position <= 0)
        return 0;

    const size_t offset = static_cast<size_t>(position);
    return (std::min)(offset, length);
}

bool BuildFullDocumentRegex(const std::string& pattern, DWORD findOptions, std::regex& regex)
{
    try
    {
        std::regex_constants::syntax_option_type flags = std::regex_constants::ECMAScript;
        if (!(findOptions & FR_MATCHCASE))
            flags |= std::regex_constants::icase;
        regex = std::regex(pattern, flags);
        return true;
    }
    catch (const std::regex_error&)
    {
        MessageBeep(MB_ICONWARNING);
        return false;
    }
}

std::string PrepareFullDocumentRegexReplacement(const wchar_t* replaceText)
{
    return DecodeRegexLineEndingEscapes(WideToUtf8(replaceText ? replaceText : L""));
}

bool SelectEditorRange(sptr_t start, sptr_t end, bool focusEditor)
{
    if (start < 0 || end < start)
        return false;

    Sci(SCI_SETSEL, static_cast<uptr_t>(start), end);
    Sci(SCI_SCROLLCARET);
    if (focusEditor)
        SetFocus(g_hSci);
    return true;
}

bool FindFullDocumentRegexInRange(const std::string& text, const std::regex& regex,
    size_t rangeStart, size_t rangeEnd, bool searchDown, sptr_t& matchStart, sptr_t& matchEnd)
{
    if (rangeStart > rangeEnd || rangeEnd > text.size())
        return false;

    using Iterator = std::string::const_iterator;
    std::match_results<Iterator> match;
    const Iterator begin = text.cbegin();
    const Iterator last = begin + rangeEnd;

    if (searchDown)
    {
        const Iterator first = begin + rangeStart;
        if (!std::regex_search(first, last, match, regex))
            return false;

        matchStart = static_cast<sptr_t>(rangeStart + static_cast<size_t>(match.position(0)));
        matchEnd = matchStart + static_cast<sptr_t>(match.length(0));
        return true;
    }

    bool found = false;
    size_t searchOffset = rangeStart;
    while (searchOffset <= rangeEnd)
    {
        const Iterator first = begin + searchOffset;
        if (!std::regex_search(first, last, match, regex))
            break;

        const size_t currentStart = searchOffset + static_cast<size_t>(match.position(0));
        const size_t currentEnd = currentStart + static_cast<size_t>(match.length(0));
        if (currentStart > rangeEnd || currentEnd > rangeEnd)
            break;

        matchStart = static_cast<sptr_t>(currentStart);
        matchEnd = static_cast<sptr_t>(currentEnd);
        found = true;

        searchOffset = currentEnd > currentStart ? currentEnd : currentStart + 1;
        if (searchOffset > rangeEnd)
            break;
    }

    return found;
}

bool FindFullDocumentRegexTextInEditor(const std::string& pattern, DWORD findOptions,
    bool searchDown, bool wrap, bool focusEditor)
{
    std::regex regex;
    if (!BuildFullDocumentRegex(pattern, findOptions, regex))
        return false;

    const std::string text = GetEditorText();
    const size_t selectionStart = ClampRegexOffset(Sci(SCI_GETSELECTIONSTART), text.size());
    const size_t selectionEnd = ClampRegexOffset(Sci(SCI_GETSELECTIONEND), text.size());
    const bool hasSelection = selectionStart != selectionEnd;
    sptr_t matchStart = -1;
    sptr_t matchEnd = -1;
    bool found = false;

    for (std::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
    {
        const size_t currentStart = static_cast<size_t>(it->position(0));
        const size_t currentEnd = currentStart + static_cast<size_t>(it->length(0));
        const bool usable = searchDown ?
            (hasSelection ? currentStart >= selectionEnd : currentEnd > selectionEnd) :
            (hasSelection ? currentEnd <= selectionStart : currentStart < selectionStart);
        if (!usable)
            continue;

        matchStart = static_cast<sptr_t>(currentStart);
        matchEnd = static_cast<sptr_t>(currentEnd);
        found = true;
        if (searchDown)
            break;
    }

    if (!found && wrap)
    {
        for (std::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
        {
            const size_t currentStart = static_cast<size_t>(it->position(0));
            const size_t currentEnd = currentStart + static_cast<size_t>(it->length(0));
            const bool usable = searchDown ?
                (hasSelection ? currentEnd <= selectionStart : currentEnd <= selectionEnd) :
                (hasSelection ? currentStart >= selectionEnd : currentStart >= selectionStart);
            if (!usable)
                continue;

            matchStart = static_cast<sptr_t>(currentStart);
            matchEnd = static_cast<sptr_t>(currentEnd);
            found = true;
            if (searchDown)
                break;
        }
    }

    if (!found)
    {
        MessageBeep(MB_ICONINFORMATION);
        return false;
    }

    return SelectEditorRange(matchStart, matchEnd, focusEditor);
}

int CountFullDocumentRegexMatches(const std::string& pattern, DWORD findOptions)
{
    std::regex regex;
    if (!BuildFullDocumentRegex(pattern, findOptions, regex))
        return 0;

    const std::string text = GetEditorText();
    int count = 0;
    for (std::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
        ++count;
    return count;
}

int MarkFullDocumentRegexMatches(const std::string& pattern, DWORD findOptions)
{
    std::regex regex;
    if (!BuildFullDocumentRegex(pattern, findOptions, regex))
        return 0;

    const std::string text = GetEditorText();
    int count = 0;
    Sci(SCI_SETINDICATORCURRENT, kSearchMarkIndicator, 0);
    Sci(SCI_SETINDICATORVALUE, 1, 0);

    for (std::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
    {
        const sptr_t matchStart = static_cast<sptr_t>(it->position(0));
        const sptr_t matchLength = static_cast<sptr_t>(it->length(0));
        ++count;
        if (matchLength > 0)
            Sci(SCI_INDICATORFILLRANGE, static_cast<uptr_t>(matchStart), matchLength);
    }

    return count;
}

bool ReplaceCurrentSelectionFullDocumentRegex(const std::string& pattern,
    const wchar_t* replaceText, DWORD findOptions)
{
    const sptr_t selectionStart = Sci(SCI_GETSELECTIONSTART);
    const sptr_t selectionEnd = Sci(SCI_GETSELECTIONEND);
    if (selectionStart == selectionEnd)
        return false;

    std::regex regex;
    if (!BuildFullDocumentRegex(pattern, findOptions, regex))
        return false;

    const std::string selectedText = GetEditorRangeText(
        (std::min)(selectionStart, selectionEnd), (std::max)(selectionStart, selectionEnd));
    std::smatch match;
    if (!std::regex_search(selectedText, match, regex) ||
        match.position(0) != 0 ||
        static_cast<size_t>(match.length(0)) != selectedText.size())
    {
        return false;
    }

    const std::string replacement = PrepareFullDocumentRegexReplacement(replaceText);
    const std::string replacedText = std::regex_replace(selectedText, regex, replacement,
        std::regex_constants::format_first_only);
    const sptr_t targetStart = (std::min)(selectionStart, selectionEnd);
    const sptr_t targetEnd = (std::max)(selectionStart, selectionEnd);

    Sci(SCI_SETTARGETRANGE, static_cast<uptr_t>(targetStart), targetEnd);
    const sptr_t replacementLength = Sci(SCI_REPLACETARGET,
        static_cast<uptr_t>(replacedText.size()), reinterpret_cast<sptr_t>(replacedText.c_str()));
    if (replacementLength >= 0)
        Sci(SCI_SETSEL, static_cast<uptr_t>(targetStart), targetStart + replacementLength);
    SetActiveTabModified(true);
    return true;
}

int ReplaceAllFullDocumentRegexMatches(const std::string& pattern,
    const wchar_t* replaceText, DWORD findOptions)
{
    std::regex regex;
    if (!BuildFullDocumentRegex(pattern, findOptions, regex))
        return 0;

    const std::string text = GetEditorText();
    int count = 0;
    for (std::sregex_iterator it(text.begin(), text.end(), regex), end; it != end; ++it)
        ++count;

    if (count == 0)
    {
        MessageBeep(MB_ICONINFORMATION);
        return 0;
    }

    const std::string replacement = PrepareFullDocumentRegexReplacement(replaceText);
    const std::string replacedText = std::regex_replace(text, regex, replacement);

    Sci(SCI_BEGINUNDOACTION);
    Sci(SCI_SETTARGETRANGE, 0, static_cast<sptr_t>(text.size()));
    Sci(SCI_REPLACETARGET,
        static_cast<uptr_t>(replacedText.size()), reinterpret_cast<sptr_t>(replacedText.c_str()));
    Sci(SCI_ENDUNDOACTION);
    SetActiveTabModified(true);
    return count;
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

void ClearSearchMarks()
{
    if (!g_hSci)
        return;

    Sci(SCI_SETINDICATORCURRENT, kSearchMarkIndicator, 0);
    Sci(SCI_INDICATORCLEARRANGE, 0, Sci(SCI_GETTEXTLENGTH));
}

int FillSearchMarks(const std::string& needle, DWORD findOptions)
{
    ClearSearchMarks();
    if (needle.empty())
        return 0;

    if (UseFullDocumentRegex(findOptions))
        return MarkFullDocumentRegexMatches(needle, findOptions);

    int count = 0;
    sptr_t start = 0;
    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);

    Sci(SCI_SETINDICATORCURRENT, kSearchMarkIndicator, 0);
    Sci(SCI_SETINDICATORVALUE, 1, 0);

    while (start <= documentLength)
    {
        const sptr_t result = SearchTargetRange(start, documentLength, needle, findOptions);
        if (result < 0)
            break;

        ++count;
        const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
        const sptr_t targetEnd = Sci(SCI_GETTARGETEND);
        if (targetEnd > targetStart)
            Sci(SCI_INDICATORFILLRANGE, static_cast<uptr_t>(targetStart), targetEnd - targetStart);

        start = targetEnd > targetStart ? targetEnd : PositionAfter(targetStart);
    }

    return count;
}

void ClearActiveTabSearchMarkState()
{
    if (!IsActiveTabValid())
        return;

    DocumentTab& tab = g_tabs[g_activeTabIndex];
    tab.hasSearchMarks = false;
    tab.searchMarkText.clear();
    tab.searchMarkOptions = 0;
}

void StoreActiveTabSearchMarkState(const wchar_t* findText, DWORD findOptions)
{
    if (!IsActiveTabValid())
        return;

    DocumentTab& tab = g_tabs[g_activeTabIndex];
    tab.hasSearchMarks = true;
    tab.searchMarkText = findText ? findText : L"";
    tab.searchMarkOptions = findOptions & kFindOptionMask;
}

void RestoreSearchMarksForActiveTab()
{
    if (!IsActiveTabValid())
    {
        ClearSearchMarks();
        return;
    }

    const DocumentTab& tab = g_tabs[g_activeTabIndex];
    if (!tab.hasSearchMarks)
    {
        ClearSearchMarks();
        return;
    }

    FillSearchMarks(PrepareFindText(tab.searchMarkText.c_str(), tab.searchMarkOptions), tab.searchMarkOptions);
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
    const std::string needle = PrepareFindText(findText, findOptions);
    if (needle.empty())
        return false;

    if (UseFullDocumentRegex(findOptions))
        return FindFullDocumentRegexTextInEditor(needle, findOptions, searchDown, wrap, focusEditor);

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

std::string ConvertDollarCapturesForScintilla(const std::string& replacement)
{
    std::string converted;
    converted.reserve(replacement.size());

    for (size_t i = 0; i < replacement.size(); ++i)
    {
        if (replacement[i] == '$' && i + 1 < replacement.size())
        {
            const char next = replacement[i + 1];
            if (next >= '0' && next <= '9')
            {
                converted.push_back('\\');
                converted.push_back(next);
                ++i;
                continue;
            }
            if (next == '$')
            {
                converted.push_back('$');
                ++i;
                continue;
            }
        }

        converted.push_back(replacement[i]);
    }

    return converted;
}

std::string PrepareReplacementText(const wchar_t* replaceText, bool regexReplacement)
{
    std::string replacement = WideToUtf8(replaceText ? replaceText : L"");
    if (regexReplacement)
        replacement = ConvertDollarCapturesForScintilla(replacement);
    return replacement;
}

bool ReplaceCurrentSelection(const wchar_t* findText, const wchar_t* replaceText, DWORD findOptions)
{
    const std::string needle = PrepareFindText(findText, findOptions);
    if (needle.empty())
        return false;

    if (UseFullDocumentRegex(findOptions))
        return ReplaceCurrentSelectionFullDocumentRegex(needle, replaceText, findOptions);

    if (!SelectionMatchesSearch(needle, findOptions))
        return false;

    const bool regexReplacement = UseScintillaRegexReplacement(needle, findOptions);
    const std::string replacement = PrepareReplacementText(replaceText, regexReplacement);
    const sptr_t targetStart = Sci(SCI_GETTARGETSTART);
    const unsigned int replaceMessage = regexReplacement ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
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
    const std::string needle = PrepareFindText(findText, findOptions);
    if (needle.empty())
        return 0;

    if (UseFullDocumentRegex(findOptions))
        return ReplaceAllFullDocumentRegexMatches(needle, replaceText, findOptions);

    const bool regexReplacement = UseScintillaRegexReplacement(needle, findOptions);
    const std::string replacement = PrepareReplacementText(replaceText, regexReplacement);
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
        const unsigned int replaceMessage = regexReplacement ? SCI_REPLACETARGETRE : SCI_REPLACETARGET;
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
    const std::string needle = PrepareFindText(findText, findOptions);
    if (needle.empty())
        return 0;

    if (UseFullDocumentRegex(findOptions))
        return CountFullDocumentRegexMatches(needle, findOptions);

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

int MarkMatches(const wchar_t* findText, DWORD findOptions)
{
    const std::string needle = PrepareFindText(findText, findOptions);
    const int count = FillSearchMarks(needle, findOptions);
    if (count > 0)
    {
        StoreActiveTabSearchMarkState(findText, findOptions);
        return count;
    }

    ClearActiveTabSearchMarkState();
    if (!needle.empty())
        MessageBeep(MB_ICONINFORMATION);

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
    if (request.fullDocument)
        options |= kFindOptionFullDocument;
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

int FindWindowMark(void*, const OpenEditFindRequest& request)
{
    RememberFindRequest(request, true);
    return MarkMatches(g_findText, g_lastFindOptions);
}

bool FindWindowReplace(void*, const OpenEditFindRequest& request)
{
    const bool searchDown = !request.reverse;
    RememberFindRequest(request, searchDown);
    bool replaced = ReplaceCurrentSelection(g_findText, g_replaceText, g_lastFindOptions);
    if (!replaced && FindTextInEditor(g_findText, g_lastFindOptions, searchDown, request.wrap, false))
        replaced = ReplaceCurrentSelection(g_findText, g_replaceText, g_lastFindOptions);
    if (replaced)
        FindTextInEditor(g_findText, g_lastFindOptions, searchDown, request.wrap, false);
    return replaced;
}

int FindWindowReplaceAll(void*, const OpenEditFindRequest& request)
{
    RememberFindRequest(request, true);
    return ReplaceAllMatches(g_findText, g_replaceText, g_lastFindOptions);
}

void FindWindowClosed(void*)
{
    EndFindWindowEditorState();
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
    callbacks.mark = FindWindowMark;
    callbacks.replace = FindWindowReplace;
    callbacks.replaceAll = FindWindowReplaceAll;
    callbacks.closed = FindWindowClosed;

    BeginFindWindowEditorState();
    if (!g_findWindow->Show(hWnd, replaceDialog, g_findText, g_replaceText,
        IsDarkTheme(), g_appLanguage == AppLanguage::Chinese, callbacks))
    {
        EndFindWindowEditorState();
    }
}

void FindNextCommand(bool searchDown)
{
    if (g_findText[0] == L'\0')
    {
        OpenFindReplaceDialog(false);
        return;
    }

    DWORD options = g_lastFindOptions & (FR_MATCHCASE | FR_WHOLEWORD |
        kFindOptionRegex | kFindOptionFullDocument);
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

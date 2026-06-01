#include "openedit_internal.h"

const wchar_t* ColumnEditorTitle()
{
    return UiText(L"\u5217\u7F16\u8F91\u5668", L"Column Editor");
}

void ShowColumnEditorMessage(HWND owner, const std::wstring& message)
{
    MessageBoxW(owner ? owner : hWnd, message.c_str(), ColumnEditorTitle(), MB_OK | MB_ICONWARNING);
}

void ShowColumnEditorMessage(HWND owner, const wchar_t* chinese, const wchar_t* english)
{
    ShowColumnEditorMessage(owner, UiText(chinese, english));
}

std::wstring TrimWhitespace(const std::wstring& text)
{
    size_t first = 0;
    while (first < text.size() && iswspace(text[first]))
        ++first;

    size_t last = text.size();
    while (last > first && iswspace(text[last - 1]))
        --last;

    return text.substr(first, last - first);
}

std::wstring GetControlText(HWND parent, int controlId)
{
    HWND control = GetDlgItem(parent, controlId);
    if (!control)
        return {};

    const int length = GetWindowTextLengthW(control);
    if (length <= 0)
        return {};

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<size_t>(length));
    return text;
}

bool TryParseInt64(const std::wstring& text, long long& value)
{
    const std::wstring trimmed = TrimWhitespace(text);
    if (trimmed.empty())
        return false;

    size_t parsed = 0;
    try
    {
        value = std::stoll(trimmed, &parsed, 10);
    }
    catch (...)
    {
        return false;
    }
    return parsed == trimmed.size();
}

bool TryParsePositiveInt(const std::wstring& text, int& value)
{
    long long parsed = 0;
    if (!TryParseInt64(text, parsed) || parsed <= 0 || parsed > (std::numeric_limits<int>::max)())
        return false;

    value = static_cast<int>(parsed);
    return true;
}

size_t MinimumNumberWidthFromInitialText(const std::wstring& text)
{
    std::wstring trimmed = TrimWhitespace(text);
    if (!trimmed.empty() && trimmed[0] == L'+')
        trimmed.erase(trimmed.begin());
    return trimmed.size();
}

bool CheckedAddInt64(long long left, long long right, long long& result)
{
    if (right > 0 && left > (std::numeric_limits<long long>::max)() - right)
        return false;
    if (right < 0 && left < (std::numeric_limits<long long>::min)() - right)
        return false;

    result = left + right;
    return true;
}

std::wstring ApplyNumberPadding(const std::wstring& value, size_t width, ColumnPaddingMode padding)
{
    if (padding == ColumnPaddingMode::None || value.size() >= width)
        return value;

    const size_t fillCount = width - value.size();
    if (padding == ColumnPaddingMode::Space)
        return std::wstring(fillCount, L' ') + value;

    if (!value.empty() && value[0] == L'-')
        return std::wstring(L"-") + std::wstring(fillCount, L'0') + value.substr(1);
    return std::wstring(fillCount, L'0') + value;
}

bool IsRectangularSelectionMode(int selectionMode)
{
    return selectionMode == SC_SEL_RECTANGLE || selectionMode == SC_SEL_THIN;
}

bool GetActiveColumnSelection(ColumnEditSelection& selection, std::wstring& error)
{
    if (!g_hSci)
    {
        error = UiText(L"\u7F16\u8F91\u5668\u5C1A\u672A\u521D\u59CB\u5316\u3002", L"The editor is not initialized.");
        return false;
    }

    const int selectionMode = static_cast<int>(Sci(SCI_GETSELECTIONMODE));
    const int selectionCount = static_cast<int>(Sci(SCI_GETSELECTIONS));
    const bool rectangularSelection = Sci(SCI_SELECTIONISRECTANGLE) != 0 || IsRectangularSelectionMode(selectionMode);
    if (!rectangularSelection)
    {
        error = selectionCount > 1 ?
            UiText(L"\u5217\u7F16\u8F91\u5668\u4EC5\u652F\u6301\u5355\u4E2A\u77E9\u5F62\u5217\u9009\u533A\uFF1B\u8BF7\u5148\u53D6\u6D88\u591A\u5149\u6807/\u591A\u9009\u533A\u3002",
                L"Column Editor supports one rectangular selection only; clear multiple selections first.") :
            UiText(L"\u8BF7\u5148\u4F7F\u7528 Alt+\u62D6\u52A8\u6216 Alt+Shift+\u65B9\u5411\u952E\u521B\u5EFA\u77E9\u5F62\u5217\u9009\u533A\u3002",
                L"Create a rectangular column selection first with Alt+drag or Alt+Shift+arrow keys.");
        return false;
    }

    const sptr_t anchor = Sci(SCI_GETRECTANGULARSELECTIONANCHOR);
    const sptr_t caret = Sci(SCI_GETRECTANGULARSELECTIONCARET);
    if (anchor == INVALID_POSITION || caret == INVALID_POSITION)
    {
        error = UiText(L"\u77E9\u5F62\u5217\u9009\u533A\u65E0\u6548\u3002", L"The rectangular column selection is invalid.");
        return false;
    }

    const sptr_t anchorLine = Sci(SCI_LINEFROMPOSITION, static_cast<uptr_t>(anchor), 0);
    const sptr_t caretLine = Sci(SCI_LINEFROMPOSITION, static_cast<uptr_t>(caret), 0);
    const sptr_t lineCount = Sci(SCI_GETLINECOUNT);
    if (anchorLine < 0 || caretLine < 0 || anchorLine >= lineCount || caretLine >= lineCount)
    {
        error = UiText(L"\u77E9\u5F62\u5217\u9009\u533A\u8D85\u51FA\u6587\u6863\u8303\u56F4\u3002", L"The rectangular column selection is outside the document.");
        return false;
    }

    const sptr_t anchorColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(anchor), 0) +
        Sci(SCI_GETRECTANGULARSELECTIONANCHORVIRTUALSPACE);
    const sptr_t caretColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(caret), 0) +
        Sci(SCI_GETRECTANGULARSELECTIONCARETVIRTUALSPACE);

    selection.firstLine = (std::min)(anchorLine, caretLine);
    selection.lastLine = (std::max)(anchorLine, caretLine);
    selection.insertColumn = (std::min)(anchorColumn, caretColumn);
    selection.lineCount = selection.lastLine - selection.firstLine + 1;

    if (selection.lineCount <= 0 || selection.lineCount > kColumnEditMaxAffectedLines)
    {
        error = UiText(L"\u5217\u5757\u5305\u542B\u7684\u884C\u6570\u8FC7\u591A\uFF0C\u5217\u7F16\u8F91\u5DF2\u4E2D\u6B62\u3002",
            L"The column block has too many lines, so the edit was stopped.");
        return false;
    }

    if (selection.insertColumn < 0 || selection.insertColumn > kColumnEditMaxVisualColumn)
    {
        error = UiText(L"\u76EE\u6807\u5217\u8FC7\u5927\uFF0C\u53EF\u80FD\u9700\u8981\u586B\u5145\u8FC7\u591A\u7A7A\u683C\u3002",
            L"The target column is too large and would require too much space padding.");
        return false;
    }

    return true;
}

bool GetInsertionPointForVisualColumn(sptr_t line, sptr_t targetColumn, ColumnInsertPoint& point, std::wstring& error)
{
    const sptr_t lineStart = Sci(SCI_POSITIONFROMLINE, static_cast<uptr_t>(line), 0);
    const sptr_t lineEnd = Sci(SCI_GETLINEENDPOSITION, static_cast<uptr_t>(line), 0);
    if (lineStart < 0 || lineEnd < lineStart)
    {
        error = UiText(L"\u65E0\u6CD5\u8BFB\u53D6\u5217\u5757\u4E2D\u7684\u884C\u3002", L"Could not read a line in the column block.");
        return false;
    }

    if (lineEnd - lineStart > kColumnEditMaxLineBytes)
    {
        error = UiText(L"\u5217\u5757\u4E2D\u5B58\u5728\u8FC7\u957F\u884C\uFF0C\u5217\u7F16\u8F91\u5DF2\u4E2D\u6B62\u3002",
            L"The column block contains an excessively long line, so the edit was stopped.");
        return false;
    }

    sptr_t position = Sci(SCI_FINDCOLUMN, static_cast<uptr_t>(line), targetColumn);
    if (position < lineStart)
        position = lineStart;
    if (position > lineEnd)
        position = lineEnd;

    sptr_t actualColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(position), 0);
    if (actualColumn > targetColumn)
    {
        sptr_t candidate = position;
        while (candidate > lineStart)
        {
            const sptr_t previous = Sci(SCI_POSITIONBEFORE, static_cast<uptr_t>(candidate), 0);
            if (previous < lineStart || previous >= candidate)
                break;

            const sptr_t previousColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(previous), 0);
            if (previousColumn <= targetColumn)
            {
                candidate = previous;
                actualColumn = previousColumn;
                break;
            }
            candidate = previous;
        }

        position = candidate;
        if (actualColumn > targetColumn)
            actualColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(position), 0);
    }

    if (actualColumn > targetColumn)
    {
        error = UiText(L"\u65E0\u6CD5\u5C06\u76EE\u6807\u89C6\u89C9\u5217\u6620\u5C04\u5230\u6587\u672C\u4F4D\u7F6E\u3002",
            L"Could not map the target visual column to a text position.");
        return false;
    }

    point.position = position;
    point.spacesBeforeValue = targetColumn - actualColumn;
    return true;
}

bool BuildColumnValueTexts(const ColumnEditSelection& selection, const ColumnEditRequest& request,
    std::vector<std::string>& values, bool& sameValueWidth, std::wstring& error)
{
    const size_t rows = static_cast<size_t>(selection.lineCount);
    values.clear();
    values.reserve(rows);
    sameValueWidth = true;

    if (request.mode == ColumnEditorMode::Text)
    {
        const std::string value = WideToUtf8(request.text);
        if (value.empty())
        {
            error = UiText(L"\u8BF7\u8F93\u5165\u8981\u63D2\u5165\u7684\u6587\u672C\u3002", L"Enter the text to insert.");
            return false;
        }

        for (size_t index = 0; index < rows; ++index)
            values.push_back(value);
        return true;
    }

    std::vector<std::wstring> baseValues;
    baseValues.reserve(rows);
    size_t fieldWidth = request.padding == ColumnPaddingMode::None ? 0 : request.minimumNumberWidth;
    long long value = request.initialValue;
    for (size_t index = 0; index < rows; ++index)
    {
        std::wstring base = std::to_wstring(value);
        fieldWidth = (std::max)(fieldWidth, base.size());
        baseValues.push_back(std::move(base));

        if (index + 1 < rows && ((index + 1) % static_cast<size_t>(request.repeatCount)) == 0)
        {
            long long next = 0;
            if (!CheckedAddInt64(value, request.increment, next))
            {
                error = UiText(L"\u6570\u5B57\u5E8F\u5217\u8D85\u51FA 64 \u4F4D\u6574\u6570\u8303\u56F4\u3002",
                    L"The numeric sequence exceeds the 64-bit integer range.");
                return false;
            }
            value = next;
        }
    }

    size_t firstWidth = 0;
    for (size_t index = 0; index < baseValues.size(); ++index)
    {
        const std::wstring formatted = ApplyNumberPadding(baseValues[index], fieldWidth, request.padding);
        std::string utf8Value = WideToUtf8(formatted);
        if (index == 0)
            firstWidth = utf8Value.size();
        else if (utf8Value.size() != firstWidth)
            sameValueWidth = false;
        values.push_back(std::move(utf8Value));
    }

    return true;
}

bool BuildColumnEditInsertions(const ColumnEditSelection& selection, const ColumnEditRequest& request,
    std::vector<ColumnEditInsertion>& insertions, bool& sameValueWidth, std::wstring& error)
{
    std::vector<std::string> values;
    if (!BuildColumnValueTexts(selection, request, values, sameValueWidth, error))
        return false;

    insertions.clear();
    insertions.reserve(values.size());
    size_t totalBytes = 0;
    for (size_t index = 0; index < values.size(); ++index)
    {
        const sptr_t line = selection.firstLine + static_cast<sptr_t>(index);
        ColumnInsertPoint insertPoint{};
        if (!GetInsertionPointForVisualColumn(line, selection.insertColumn, insertPoint, error))
            return false;

        if (insertPoint.spacesBeforeValue < 0 ||
            insertPoint.spacesBeforeValue > static_cast<sptr_t>(kColumnEditMaxInsertBytes))
        {
            error = UiText(L"\u9700\u8981\u586B\u5145\u7684\u7A7A\u683C\u8FC7\u591A\uFF0C\u5217\u7F16\u8F91\u5DF2\u4E2D\u6B62\u3002",
                L"Too much space padding would be required, so the column edit was stopped.");
            return false;
        }

        const size_t spaces = static_cast<size_t>(insertPoint.spacesBeforeValue);
        const size_t valueBytes = values[index].size();
        if (totalBytes > kColumnEditMaxInsertBytes - spaces ||
            totalBytes + spaces > kColumnEditMaxInsertBytes - valueBytes)
        {
            error = UiText(L"\u672C\u6B21\u63D2\u5165\u5185\u5BB9\u8FC7\u5927\uFF0C\u5217\u7F16\u8F91\u5DF2\u4E2D\u6B62\u3002",
                L"This insertion is too large, so the column edit was stopped.");
            return false;
        }
        totalBytes += spaces + valueBytes;

        std::string insertionText(spaces, ' ');
        insertionText += values[index];
        insertions.push_back(ColumnEditInsertion{
            line,
            insertPoint.position,
            spaces,
            valueBytes,
            std::move(insertionText)
        });
    }

    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);
    if (documentLength > kColumnEditMaxDocumentBytes)
    {
        error = UiText(L"\u6587\u4EF6\u8FC7\u5927\uFF0C\u5217\u7F16\u8F91\u5DF2\u4E2D\u6B62\u3002",
            L"The file is too large, so the column edit was stopped.");
        return false;
    }

    if (totalBytes > static_cast<size_t>((std::numeric_limits<sptr_t>::max)() - documentLength))
    {
        error = UiText(L"\u672C\u6B21\u63D2\u5165\u540E\u6587\u6863\u5927\u5C0F\u8D85\u51FA\u652F\u6301\u8303\u56F4\u3002",
            L"The document would exceed the supported size after this insertion.");
        return false;
    }

    return true;
}

sptr_t PositionForSelectionColumn(sptr_t line, sptr_t column, sptr_t& virtualSpace)
{
    sptr_t position = Sci(SCI_FINDCOLUMN, static_cast<uptr_t>(line), column);
    const sptr_t actualColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(position), 0);
    virtualSpace = actualColumn < column ? column - actualColumn : 0;
    return position;
}

void SetRectangularSelectionByColumns(sptr_t firstLine, sptr_t lastLine, sptr_t startColumn, sptr_t endColumn)
{
    sptr_t anchorVirtual = 0;
    sptr_t caretVirtual = 0;
    const sptr_t anchor = PositionForSelectionColumn(firstLine, startColumn, anchorVirtual);
    const sptr_t caret = PositionForSelectionColumn(lastLine, endColumn, caretVirtual);

    Sci(SCI_SETSELECTIONMODE, endColumn == startColumn ? SC_SEL_THIN : SC_SEL_RECTANGLE, 0);
    Sci(SCI_SETRECTANGULARSELECTIONANCHOR, static_cast<uptr_t>(anchor), 0);
    Sci(SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE, static_cast<uptr_t>(anchorVirtual), 0);
    Sci(SCI_SETRECTANGULARSELECTIONCARET, static_cast<uptr_t>(caret), 0);
    Sci(SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE, static_cast<uptr_t>(caretVirtual), 0);
    Sci(SCI_SCROLLCARET);
}

void RestoreColumnEditSelection(const ColumnEditSelection& selection,
    const std::vector<ColumnEditInsertion>& insertions, bool sameValueWidth)
{
    if (insertions.empty())
        return;

    if (sameValueWidth)
    {
        const sptr_t valueStart = Sci(SCI_FINDCOLUMN, static_cast<uptr_t>(selection.firstLine), selection.insertColumn);
        const sptr_t valueEnd = valueStart + static_cast<sptr_t>(insertions.front().valueBytes);
        const sptr_t endColumn = Sci(SCI_GETCOLUMN, static_cast<uptr_t>(valueEnd), 0);
        SetRectangularSelectionByColumns(selection.firstLine, selection.lastLine, selection.insertColumn, endColumn);
        return;
    }

    const sptr_t caretColumn = selection.insertColumn + static_cast<sptr_t>(insertions.back().valueBytes);
    SetRectangularSelectionByColumns(selection.lastLine, selection.lastLine, caretColumn, caretColumn);
}

bool ApplyColumnEditRequest(HWND owner, const ColumnEditRequest& request)
{
    if (Sci(SCI_GETREADONLY))
    {
        ShowColumnEditorMessage(owner, L"\u5F53\u524D\u6587\u6863\u662F\u53EA\u8BFB\u7684\uFF0C\u65E0\u6CD5\u6267\u884C\u5217\u7F16\u8F91\u3002",
            L"The current document is read-only, so column editing cannot be applied.");
        return false;
    }

    ColumnEditSelection selection{};
    std::wstring error;
    if (!GetActiveColumnSelection(selection, error))
    {
        ShowColumnEditorMessage(owner, error);
        return false;
    }

    bool sameValueWidth = true;
    std::vector<ColumnEditInsertion> insertions;
    if (!BuildColumnEditInsertions(selection, request, insertions, sameValueWidth, error))
    {
        ShowColumnEditorMessage(owner, error);
        return false;
    }

    size_t totalBytes = 0;
    for (const ColumnEditInsertion& insertion : insertions)
        totalBytes += insertion.text.size();

    const sptr_t documentLength = Sci(SCI_GETTEXTLENGTH);
    Sci(SCI_ALLOCATE, static_cast<uptr_t>(documentLength + static_cast<sptr_t>(totalBytes)), 0);

    {
        ScopedRedrawPause pause(g_hSci);
        Sci(SCI_BEGINUNDOACTION);
        for (auto it = insertions.rbegin(); it != insertions.rend(); ++it)
        {
            Sci(SCI_INSERTTEXT, static_cast<uptr_t>(it->position), reinterpret_cast<sptr_t>(it->text.c_str()));
        }
        Sci(SCI_ENDUNDOACTION);
    }

    RestoreColumnEditSelection(selection, insertions, sameValueWidth);
    SetActiveTabModified(true);
    return true;
}

#include "openedit_internal.h"

std::wstring PickFolderPath(HWND owner)
{
    IFileDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
    if (FAILED(hr))
    {
        ShowErrorMessage(L"无法打开文件夹选择器。");
        return L"";
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
    dialog->SetTitle(L"打开文件夹");

    std::wstring folderPath;
    if (SUCCEEDED(dialog->Show(owner)))
    {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item)))
        {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
            {
                folderPath = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }

    dialog->Release();
    return folderPath;
}

int AddThemedFolderIconToImageList(HIMAGELIST imageList, int iconWidth, int iconHeight, bool open)
{
    if (!imageList || iconWidth <= 0 || iconHeight <= 0)
        return -1;

    HDC screenDc = GetDC(nullptr);
    if (!screenDc)
        return -1;

    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = memoryDc ? CreateCompatibleBitmap(screenDc, iconWidth, iconHeight) : nullptr;
    ReleaseDC(nullptr, screenDc);

    if (!memoryDc || !bitmap)
    {
        if (bitmap)
            DeleteObject(bitmap);
        if (memoryDc)
            DeleteDC(memoryDc);
        return -1;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    RECT fullRect{ 0, 0, iconWidth, iconHeight };
    HBRUSH maskBrush = CreateSolidBrush(kFolderTreeIconMaskColor);
    FillRect(memoryDc, &fullRect, maskBrush);
    DeleteObject(maskBrush);

    const int left = (std::max)(1, iconWidth / 16);
    const int top = (std::max)(2, iconHeight / 5);
    const int right = iconWidth - (std::max)(1, iconWidth / 16);
    const int bottom = iconHeight - (std::max)(2, iconHeight / 8);
    const int tabWidth = (std::max)(5, iconWidth / 3);
    const int tabHeight = (std::max)(3, iconHeight / 5);
    const COLORREF borderColor = IsDarkTheme() ? RGB(216, 172, 83) : RGB(172, 116, 32);
    const COLORREF tabFillColor = IsDarkTheme() ? RGB(190, 139, 52) : RGB(237, 176, 70);
    const COLORREF bodyFillColor = IsDarkTheme() ? RGB(205, 153, 65) : RGB(248, 194, 91);
    const COLORREF openFillColor = IsDarkTheme() ? RGB(224, 171, 77) : RGB(255, 207, 104);

    HBRUSH tabBrush = CreateSolidBrush(tabFillColor);
    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBrush = SelectObject(memoryDc, tabBrush);
    HGDIOBJ oldPen = SelectObject(memoryDc, borderPen);

    POINT tabShape[] = {
        { left + 1, top },
        { left + tabWidth, top },
        { left + tabWidth + 2, top + tabHeight },
        { right - 1, top + tabHeight },
        { right - 1, top + tabHeight + 3 },
        { left + 1, top + tabHeight + 3 },
        { left + 1, top },
    };
    Polygon(memoryDc, tabShape, static_cast<int>(sizeof(tabShape) / sizeof(tabShape[0])));

    HBRUSH bodyBrush = CreateSolidBrush(open ? openFillColor : bodyFillColor);
    SelectObject(memoryDc, bodyBrush);
    if (open)
    {
        POINT openShape[] = {
            { left, top + tabHeight + 3 },
            { right, top + tabHeight + 3 },
            { right - 2, bottom },
            { left + 2, bottom },
            { left, top + tabHeight + 3 },
        };
        Polygon(memoryDc, openShape, static_cast<int>(sizeof(openShape) / sizeof(openShape[0])));
    }
    else
    {
        RoundRect(memoryDc, left, top + tabHeight + 2, right, bottom, 2, 2);
    }

    SelectObject(memoryDc, oldBrush);
    SelectObject(memoryDc, oldPen);
    DeleteObject(bodyBrush);
    DeleteObject(tabBrush);
    DeleteObject(borderPen);
    SelectObject(memoryDc, oldBitmap);

    const int iconIndex = ImageList_AddMasked(imageList, bitmap, kFolderTreeIconMaskColor);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    return iconIndex;
}

int AddThemedFileIconToImageList(HIMAGELIST imageList, int iconWidth, int iconHeight)
{
    if (!imageList || iconWidth <= 0 || iconHeight <= 0)
        return -1;

    HDC screenDc = GetDC(nullptr);
    if (!screenDc)
        return -1;

    HDC memoryDc = CreateCompatibleDC(screenDc);
    HBITMAP bitmap = memoryDc ? CreateCompatibleBitmap(screenDc, iconWidth, iconHeight) : nullptr;
    ReleaseDC(nullptr, screenDc);

    if (!memoryDc || !bitmap)
    {
        if (bitmap)
            DeleteObject(bitmap);
        if (memoryDc)
            DeleteDC(memoryDc);
        return -1;
    }

    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    RECT fullRect{ 0, 0, iconWidth, iconHeight };
    HBRUSH maskBrush = CreateSolidBrush(kFolderTreeIconMaskColor);
    FillRect(memoryDc, &fullRect, maskBrush);
    DeleteObject(maskBrush);

    const int left = (std::max)(2, iconWidth / 8);
    const int top = (std::max)(1, iconHeight / 8);
    const int right = iconWidth - left;
    const int bottom = iconHeight - top;
    const int fold = (std::max)(4, iconWidth / 4);
    const COLORREF lineColor = IsDarkTheme() ? RGB(190, 190, 190) : RGB(82, 92, 104);

    HPEN linePen = CreatePen(PS_SOLID, 1, lineColor);
    HGDIOBJ oldPen = SelectObject(memoryDc, linePen);
    HGDIOBJ oldBrush = SelectObject(memoryDc, GetStockObject(NULL_BRUSH));

    POINT outline[] = {
        { left, top },
        { right - fold, top },
        { right, top + fold },
        { right, bottom },
        { left, bottom },
        { left, top },
    };
    Polyline(memoryDc, outline, static_cast<int>(sizeof(outline) / sizeof(outline[0])));
    MoveToEx(memoryDc, right - fold, top, nullptr);
    LineTo(memoryDc, right - fold, top + fold);
    LineTo(memoryDc, right, top + fold);

    const int lineLeft = left + 3;
    const int lineRight = right - 3;
    const int firstLineY = top + ((bottom - top) / 2);
    MoveToEx(memoryDc, lineLeft, firstLineY, nullptr);
    LineTo(memoryDc, lineRight, firstLineY);
    MoveToEx(memoryDc, lineLeft, (std::min)(bottom - 2, firstLineY + 3), nullptr);
    LineTo(memoryDc, lineRight, (std::min)(bottom - 2, firstLineY + 3));

    SelectObject(memoryDc, oldBrush);
    SelectObject(memoryDc, oldPen);
    DeleteObject(linePen);
    SelectObject(memoryDc, oldBitmap);

    const int iconIndex = ImageList_AddMasked(imageList, bitmap, kFolderTreeIconMaskColor);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    return iconIndex;
}

void InitializeFolderTreeImageList()
{
    if (!g_hFolderTree && !g_hOpenFilesTree)
        return;

    const int iconWidth = GetSystemMetrics(SM_CXSMICON);
    const int iconHeight = GetSystemMetrics(SM_CYSMICON);
    HIMAGELIST imageList = ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 3, 0);
    if (!imageList)
        return;

    ImageList_SetBkColor(imageList, CLR_NONE);

    const int folderIconIndex = AddThemedFolderIconToImageList(imageList, iconWidth, iconHeight, false);
    const int folderOpenIconIndex = AddThemedFolderIconToImageList(imageList, iconWidth, iconHeight, true);
    const int textFileIconIndex = AddThemedFileIconToImageList(imageList, iconWidth, iconHeight);

    if (folderIconIndex < 0 || folderOpenIconIndex < 0 || textFileIconIndex < 0)
    {
        ImageList_Destroy(imageList);
        return;
    }

    if (g_hFolderTree)
        TreeView_SetImageList(g_hFolderTree, imageList, TVSIL_NORMAL);
    if (g_hOpenFilesTree)
        TreeView_SetImageList(g_hOpenFilesTree, imageList, TVSIL_NORMAL);

    if (g_hFolderTreeImageList)
        ImageList_Destroy(g_hFolderTreeImageList);

    g_hFolderTreeImageList = imageList;
    g_folderIconIndex = folderIconIndex;
    g_folderOpenIconIndex = folderOpenIconIndex;
    g_textFileIconIndex = textFileIconIndex;
}

LRESULT HandleFolderTreeCustomDraw(LPARAM lParam)
{
    NMTVCUSTOMDRAW* customDraw = reinterpret_cast<NMTVCUSTOMDRAW*>(lParam);
    switch (customDraw->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
        if (customDraw->nmcd.uItemState & CDIS_SELECTED)
        {
            customDraw->clrText = ThemeFolderTreeSelectionText();
            customDraw->clrTextBk = ThemeFolderTreeSelectionBack();
        }
        else
        {
            customDraw->clrText = ThemeFolderPaneText();
            customDraw->clrTextBk = ThemeFolderPaneBack();
        }
        return CDRF_DODEFAULT;
    }

    return CDRF_DODEFAULT;
}

LRESULT CALLBACK FolderTreeWndProc(HWND treeWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_LBUTTONDBLCLK:
    {
        POINT clientPoint{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HandleFolderTreeItemClickAt(clientPoint, true);
        return 0;
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT clientRect{};
        GetClientRect(treeWindow, &clientRect);
        HBRUSH brush = CreateSolidBrush(ThemeFolderPaneBack());
        FillRect(hdc, &clientRect, brush);
        DeleteObject(brush);
        return 1;
    }

    case WM_THEMECHANGED:
        ApplyFolderTreeTheme();
        InvalidateFolderTree();
        break;
    }

    return g_originalFolderTreeProc ?
        CallWindowProcW(g_originalFolderTreeProc, treeWindow, message, wParam, lParam) :
        DefWindowProcW(treeWindow, message, wParam, lParam);
}

LRESULT CALLBACK OpenFilesTreeWndProc(HWND treeWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_LBUTTONDBLCLK:
    {
        POINT clientPoint{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        HandleOpenFilesTreeItemClickAt(clientPoint);
        return 0;
    }

    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT clientRect{};
        GetClientRect(treeWindow, &clientRect);
        HBRUSH brush = CreateSolidBrush(ThemeFolderPaneBack());
        FillRect(hdc, &clientRect, brush);
        DeleteObject(brush);
        return 1;
    }

    case WM_THEMECHANGED:
        ApplyFolderTreeTheme();
        InvalidateOpenFilesTree();
        break;
    }

    return g_originalOpenFilesTreeProc ?
        CallWindowProcW(g_originalOpenFilesTreeProc, treeWindow, message, wParam, lParam) :
        DefWindowProcW(treeWindow, message, wParam, lParam);
}

LRESULT CALLBACK EditorWndProc(HWND editorWindow, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DROPFILES:
        HandleDroppedFiles(reinterpret_cast<HDROP>(wParam));
        return 0;

    case WM_MOUSEWHEEL:
        if (g_hTabBar && IsWindow(g_hTabBar))
        {
            POINT screenPoint{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT tabRect{};
            if (GetWindowRect(g_hTabBar, &tabRect) && PtInRect(&tabRect, screenPoint))
            {
                SendMessageW(g_hTabBar, message, wParam, lParam);
                return 0;
            }
        }
        break;
    }

    return g_originalEditorProc ?
        CallWindowProcW(g_originalEditorProc, editorWindow, message, wParam, lParam) :
        DefWindowProcW(editorWindow, message, wParam, lParam);
}

std::vector<FolderItem> EnumerateFolderChildren(const std::wstring& folderPath)
{
    namespace fs = std::filesystem;

    std::vector<FolderItem> children;
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(fs::path(folderPath), fs::directory_options::skip_permission_denied, ec))
    {
        std::error_code itemError;
        FolderItem item;
        item.name = entry.path().filename().wstring();
        item.path = entry.path().wstring();
        item.isDirectory = entry.is_directory(itemError);
        if (!itemError)
            children.push_back(std::move(item));
    }

    std::sort(children.begin(), children.end(), [](const FolderItem& left, const FolderItem& right) {
        if (left.isDirectory != right.isDirectory)
            return left.isDirectory;
        return CompareStringOrdinal(left.name.c_str(), -1, right.name.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    });

    return children;
}

FolderItem* StoreFolderItem(FolderItem item)
{
    g_folderItems.push_back(std::make_unique<FolderItem>(std::move(item)));
    return g_folderItems.back().get();
}

void DeleteTreeChildren(HTREEITEM parentItem)
{
    HTREEITEM child = TreeView_GetChild(g_hFolderTree, parentItem);
    while (child)
    {
        TreeView_DeleteItem(g_hFolderTree, child);
        child = TreeView_GetChild(g_hFolderTree, parentItem);
    }
}

HTREEITEM InsertFolderTreeItem(HTREEITEM parentItem, const FolderItem& source)
{
    FolderItem* itemData = StoreFolderItem(source);
    const int iconIndex = source.isDirectory ? g_folderIconIndex : g_textFileIconIndex;
    const int selectedIconIndex = source.isDirectory ? g_folderOpenIconIndex : g_textFileIconIndex;

    TVINSERTSTRUCTW insert{};
    insert.hParent = parentItem;
    insert.hInsertAfter = TVI_LAST;
    insert.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
    insert.item.pszText = const_cast<LPWSTR>(itemData->name.c_str());
    insert.item.iImage = iconIndex;
    insert.item.iSelectedImage = selectedIconIndex;
    insert.item.lParam = reinterpret_cast<LPARAM>(itemData);
    insert.item.cChildren = source.isDirectory ? 1 : 0;

    return TreeView_InsertItem(g_hFolderTree, &insert);
}

void UpdateFolderNodeIcon(HTREEITEM treeItem, bool expanded)
{
    TVITEMW item{};
    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.hItem = treeItem;
    item.iImage = expanded ? g_folderOpenIconIndex : g_folderIconIndex;
    item.iSelectedImage = expanded ? g_folderOpenIconIndex : g_folderIconIndex;
    TreeView_SetItem(g_hFolderTree, &item);
}

FolderItem* GetFolderItemData(HTREEITEM treeItem)
{
    TVITEMW item{};
    item.mask = TVIF_PARAM;
    item.hItem = treeItem;
    if (!TreeView_GetItem(g_hFolderTree, &item))
        return nullptr;
    return reinterpret_cast<FolderItem*>(item.lParam);
}

void LoadFolderNodeChildren(HTREEITEM treeItem, FolderItem* folderItem)
{
    if (!folderItem || !folderItem->isDirectory || folderItem->childrenLoaded)
        return;

    ScopedRedrawPause redrawPause(g_hFolderTree);
    DeleteTreeChildren(treeItem);

    const std::vector<FolderItem> children = EnumerateFolderChildren(folderItem->path);
    for (const FolderItem& child : children)
        InsertFolderTreeItem(treeItem, child);

    folderItem->childrenLoaded = true;

    TVITEMW item{};
    item.mask = TVIF_CHILDREN;
    item.hItem = treeItem;
    item.cChildren = children.empty() ? 0 : 1;
    TreeView_SetItem(g_hFolderTree, &item);
}

void PrepareFolderTreeItemForExpand(HTREEITEM treeItem)
{
    if (!g_hFolderTree || !treeItem)
        return;

    FolderItem* folderItem = GetFolderItemData(treeItem);
    if (!folderItem || !folderItem->isDirectory)
        return;

    LoadFolderNodeChildren(treeItem, folderItem);
    if (TreeView_GetChild(g_hFolderTree, treeItem))
    {
        TVITEMW item{};
        item.mask = TVIF_CHILDREN;
        item.hItem = treeItem;
        item.cChildren = 1;
        TreeView_SetItem(g_hFolderTree, &item);
    }
}

void ExpandFolderTreeItem(HTREEITEM treeItem)
{
    if (!g_hFolderTree || !treeItem)
        return;

    ScopedRedrawPause redrawPause(g_hFolderTree);
    PrepareFolderTreeItemForExpand(treeItem);
    TreeView_Expand(g_hFolderTree, treeItem, TVE_EXPAND);
}

void ToggleFolderTreeItem(HTREEITEM treeItem)
{
    if (!g_hFolderTree || !treeItem)
        return;

    ScopedRedrawPause redrawPause(g_hFolderTree);
    const UINT state = TreeView_GetItemState(g_hFolderTree, treeItem, TVIS_EXPANDED);
    if ((state & TVIS_EXPANDED) == 0)
        PrepareFolderTreeItemForExpand(treeItem);

    TreeView_Expand(g_hFolderTree, treeItem, (state & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND);
}

void PopulateFolderTree(const std::wstring& folderPath)
{
    if (!g_hFolderTree)
        return;

    namespace fs = std::filesystem;

    ScopedRedrawPause redrawPause(g_hFolderTree);
    g_currentFolderPath = folderPath;
    g_restoreFolderInSession = true;
    g_folderItems.clear();
    TreeView_DeleteAllItems(g_hFolderTree);

    FolderItem rootItem;
    rootItem.path = folderPath;
    rootItem.name = FileNameFromPath(folderPath);
    rootItem.isDirectory = true;

    HTREEITEM rootTreeItem = InsertFolderTreeItem(TVI_ROOT, rootItem);
    LoadFolderNodeChildren(rootTreeItem, GetFolderItemData(rootTreeItem));
    TreeView_Expand(g_hFolderTree, rootTreeItem, TVE_EXPAND);
    TreeView_SelectItem(g_hFolderTree, rootTreeItem);
    UpdateFolderNodeIcon(rootTreeItem, true);
    InvalidateFolderTree();

    SetFolderPaneVisible(true);
    UpdateWindowTitle();
}

void OpenFolderCommand()
{
    const std::wstring folderPath = PickFolderPath(hWnd);
    if (!folderPath.empty())
        PopulateFolderTree(folderPath);
}

bool HandleFolderTreeItemClickAt(POINT clientPoint, bool doubleClick)
{
    if (!g_hFolderTree)
        return false;

    TVHITTESTINFO hitTest{};
    hitTest.pt = clientPoint;
    HTREEITEM clicked = TreeView_HitTest(g_hFolderTree, &hitTest);
    if (!clicked)
        return false;

    if (hitTest.flags & TVHT_ONITEMBUTTON)
    {
        return doubleClick;
    }

    if ((hitTest.flags & (TVHT_ONITEMICON | TVHT_ONITEMLABEL)) == 0)
        return false;

    ScopedRedrawPause redrawPause(g_hFolderTree);
    TreeView_SelectItem(g_hFolderTree, clicked);

    FolderItem* item = GetFolderItemData(clicked);
    if (!item)
        return true;

    if (item->isDirectory)
    {
        if (doubleClick)
            ToggleFolderTreeItem(clicked);
        return true;
    }

    if (doubleClick)
        LoadFileIntoEditor(item->path, true);
    return true;
}

bool HandleFolderTreeSingleClick()
{
    DWORD cursorPosition = GetMessagePos();
    POINT clientPoint{ GET_X_LPARAM(cursorPosition), GET_Y_LPARAM(cursorPosition) };
    ScreenToClient(g_hFolderTree, &clientPoint);
    return HandleFolderTreeItemClickAt(clientPoint, false);
}

HTREEITEM GetFolderTreeSelection()
{
    return g_hFolderTree ? TreeView_GetSelection(g_hFolderTree) : nullptr;
}

void RefreshFolderTreeItem(HTREEITEM treeItem)
{
    if (!g_hFolderTree)
        return;

    if (!treeItem)
    {
        if (!g_currentFolderPath.empty())
            PopulateFolderTree(g_currentFolderPath);
        return;
    }

    FolderItem* item = GetFolderItemData(treeItem);
    if (!item)
        return;

    if (!item->isDirectory)
    {
        HTREEITEM parent = TreeView_GetParent(g_hFolderTree, treeItem);
        if (parent)
            RefreshFolderTreeItem(parent);
        return;
    }

    ScopedRedrawPause redrawPause(g_hFolderTree);
    item->childrenLoaded = false;
    LoadFolderNodeChildren(treeItem, item);
    TreeView_Expand(g_hFolderTree, treeItem, TVE_EXPAND);
    TreeView_SelectItem(g_hFolderTree, treeItem);
    UpdateFolderNodeIcon(treeItem, true);
    InvalidateFolderTree();
}

void CopyTextToClipboard(const std::wstring& text)
{
    if (text.empty() || !OpenClipboard(hWnd))
        return;

    EmptyClipboard();
    const size_t byteCount = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if (memory)
    {
        void* data = GlobalLock(memory);
        if (data)
        {
            memcpy(data, text.c_str(), byteCount);
            GlobalUnlock(memory);
            if (SetClipboardData(CF_UNICODETEXT, memory))
                memory = nullptr;
        }
    }
    if (memory)
        GlobalFree(memory);
    CloseClipboard();
}

void CopySelectedFolderItemName()
{
    FolderItem* item = GetFolderItemData(GetFolderTreeSelection());
    if (item)
        CopyTextToClipboard(item->name);
}

void OpenSelectedFolderItemInExplorer()
{
    FolderItem* item = GetFolderItemData(GetFolderTreeSelection());
    if (!item)
        return;

    if (item->isDirectory)
    {
        ShellExecuteW(hWnd, L"open", item->path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }

    const std::wstring parameters = L"/select,\"" + item->path + L"\"";
    ShellExecuteW(hWnd, L"open", L"explorer.exe", parameters.c_str(), nullptr, SW_SHOWNORMAL);
}

void ShowFolderTreeContextMenu(POINT screenPoint)
{
    if (!g_hFolderTree)
        return;

    POINT clientPoint = screenPoint;
    ScreenToClient(g_hFolderTree, &clientPoint);

    TVHITTESTINFO hitTest{};
    hitTest.pt = clientPoint;
    HTREEITEM hitItem = TreeView_HitTest(g_hFolderTree, &hitTest);
    if (hitItem)
        TreeView_SelectItem(g_hFolderTree, hitItem);

    HTREEITEM selected = GetFolderTreeSelection();
    if (!selected)
        return;

    HMENU menu = CreatePopupMenu();
    if (!menu)
        return;

    AppendMenuW(menu, MF_STRING, IDM_FOLDER_REFRESH, UiText(L"\u5237\u65B0\u6587\u4EF6\u5939", L"Refresh Folder"));
    AppendMenuW(menu, MF_STRING, IDM_FOLDER_COPY_NAME, UiText(L"\u590D\u5236\u6587\u4EF6\u540D", L"Copy Name"));
    AppendMenuW(menu, MF_STRING, IDM_FOLDER_OPEN_EXPLORER, UiText(L"\u6587\u4EF6\u7BA1\u7406\u5668\u6253\u5F00", L"Open in Explorer"));
    std::vector<std::unique_ptr<ThemedMenuItem>> menuItems;
    ApplyPopupMenuTheme(menu, menuItems);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, screenPoint.x, screenPoint.y, 0, hWnd, nullptr);
    DestroyMenu(menu);
}

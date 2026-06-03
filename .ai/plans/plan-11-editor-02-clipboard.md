# phase-11-editor-02-clipboard — 剪贴板操作与全局快捷键

## ① 核心问题
有选区时无法复制/剪切，也无法粘贴文本。Delete 键在有选区时应删除选区内容。这些操作只能通过鼠标右键菜单完成，但右键菜单尚未实现。

## ② 目标
- **Ctrl+C**：有选区时复制选中文本到系统剪贴板
- **Ctrl+X**：有选区时剪切（复制到剪贴板 + 删除选区）
- **Ctrl+V**：粘贴系统剪贴板内容到光标位置
- **Delete 键**：有选区时删除选区内容
- 所有快捷键在窗口拥有焦点时全局生效

## ③ 方案设计
在 `App_OnKeyDown` 中增加 Ctrl/C/X/V 和 Delete 的判断。
> 注意：Ctrl+C/V/X 是字符键，无法放在 `switch(VK_*)` 中。应在 switch 之前用 `if (GetKeyState(VK_CONTROL) < 0)` 统一判断。

### 子步拆分
| 子步 | 内容 | 涉及层 | 预计行数 |
|------|------|--------|---------|
| ① | Ctrl+C/V + 系统剪贴板 API | app 层 | ~40 |
| ② | Ctrl+X + Delete 键处理 | app + editor 层 | ~20 |
| ③ | Delete 无选区时向后删 | editor 层 | ~10 |

## ④ 实现细节

### 子步① — 复制/粘贴

```c
case 'C': // Ctrl+C
    if (GetKeyState(VK_CONTROL) < 0 && Editor_HasSelection(&g_app.editor))
    {
        int start = Editor_GetSelectionAnchor(&g_app.editor);
        int end   = Editor_GetSelectionActive(&g_app.editor);
        if (start > end) { int t = start; start = end; end = t; }
        int len = end - start;
        if (len > 0)
        {
            const wchar_t* text = Document_GetText(Editor_GetDocument(&g_app.editor));
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
            if (hMem)
            {
                wchar_t* dst = GlobalLock(hMem);
                wcsncpy(dst, text + start, len);
                dst[len] = L'\0';
                GlobalUnlock(hMem);
                if (!OpenClipboard(g_app.hwnd))
                {
                    GlobalFree(hMem);
                    break;
                }
                EmptyClipboard();
                SetClipboardData(CF_UNICODETEXT, hMem);
                CloseClipboard();
            }
        }
    }
    break;
```

`case 'V'` 类似，用 `GetClipboardData(CF_UNICODETEXT)` 读取后调用新增的 `Editor_InsertText` 批量插入。
> `Editor_InsertText` 内部先调用 `Editor_ClearSelection` 清除选区，再逐个调 `Document_InsertChar` 插入文本。
> 注意：当前 `Document` 层没有批量插入函数，逐个插入在小文档中可接受。后续可优化为 `Document_InsertText` 批量插入。

### 子步② — 剪切和 Delete
- Ctrl+X：复制 + Delete 选区
- Delete 键：`VK_DELETE`，有选区时清除选区（同 Backspace），无选区时删除光标后一个字符

## ⑤ 测试计划

### 正常路径
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-1 | Ctrl+C 复制 | 选中文本 → Ctrl+C | 文本进入剪贴板 |
| 2-2 | Ctrl+V 粘贴 | 剪贴板有内容 → Ctrl+V | 文本出现在光标位置 |
| 2-3 | Ctrl+X 剪切 | 选中文本 → Ctrl+X | 文本移到剪贴板，内容被删除 |
| 2-4 | Delete 清除选区 | 有选区 → Delete | 选区内容删除 |

### 边界
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-5 | 无选区时 Ctrl+C | 无选区 → Ctrl+C | 无操作 |
| 2-6 | 无选区时 Delete | 光标在中间 → Delete | 光标后删一个字符 |
| 2-7 | 剪贴板为空时 Ctrl+V | 剪贴板空 → Ctrl+V | 无操作 |

### 回归
| 编号 | 用例 | 操作 | 预期 |
|------|------|------|------|
| 2-8 | 正常输入不受影响 | 打字 | 输入正常 |
| 2-9 | Ctrl+Z 不受影响 | Ctrl+Z | 撤销正常 |

## ⑥ 修改文件清单

| 文件 | 改动 |
|------|------|
| `src/app/app.c` | `App_OnKeyDown` 增加 Ctrl+C/V/X + Delete 处理 |
| `src/editor/editor.c` | 新增 `Editor_InsertText` 批量插入 + `HandleKey(DELETE)` 处理 |
| `src/editor/editor.h` | 新增 `EDITOR_KEY_DELETE` 枚举 + 声明 `Editor_InsertText` |

## ⑦ 推进步骤
1. 在 `editor.h` 增加 `EDITOR_KEY_DELETE` 枚举 + 声明 `Editor_InsertText`
2. 在 `editor.c` 实现 `Editor_InsertText`（批量插入）+ 在 `HandleKey` 中处理 DELETE 情况
3. 在 `app.c` 的 `OnKeyDown` 中处理 Ctrl+C/V/X
4. 编译 + 手动验证

## ⑧ 分层核实
- app 层调用 `OpenClipboard`/`SetClipboardData`（Win32 API）
- editor 层只处理 `EDITOR_KEY_DELETE` 等编辑操作
- `platform/*`、`render/*`、`storage/*` 零改动

#ifndef DESKNOTE_EDITOR_VIEW_H
#define DESKNOTE_EDITOR_VIEW_H

#include "../core/document.h"
#include "../render/render.h"

/*
 * 绘制最小编辑区视图。
 *
 * 当前阶段只负责：
 * - 绘制编辑区背景
 * - 绘制占位文字
 *
 * 不负责：
 * - 文本编辑逻辑
 * - 光标 / 选区状态
 * - 文件读写
 */
void EditorView_Draw(RenderContext* ctx,
                     const Document* document,
                     int cursor,
                     int window_width,
                     int window_height);

/*
 * 计算编辑区当前光标的像素矩形。
 *
 * 该函数只负责把当前文档和光标索引映射为视图中的客户区坐标，
 * 供 app 层同步系统光标和输入法位置使用。
 */
int EditorView_GetCaretRect(RenderContext* ctx,
                            const Document* document,
                            int cursor,
                            int window_width,
                            int window_height,
                            RenderRect* out_rect);

#endif
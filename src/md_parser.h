// md_parser.h - Markdown 解析器封装
#ifndef MD_PARSER_H
#define MD_PARSER_H

#include <windows.h>

// ============================================================
// 格式类型
// ============================================================
#define MD_FMT_BOLD          1
#define MD_FMT_ITALIC        2
#define MD_FMT_CODE          3
#define MD_FMT_STRIKETHROUGH 4
#define MD_FMT_H1            5
#define MD_FMT_H2            6
#define MD_FMT_H3            7
#define MD_FMT_H4            8
#define MD_FMT_H5            9
#define MD_FMT_H6            10
#define MD_FMT_DATE          11   // @today @2025-01-20
#define MD_FMT_PRIORITY      12   // !high !medium !low
#define MD_FMT_TAG           13   // #tag

// ============================================================
// 一条格式指令
// ============================================================
typedef struct {
    int type;       // MD_FMT_*
    int beg;        // 起始字符位置（UTF-16 索引）
    int end;        // 结束字符位置（不包含）
} MDFormat;

// ============================================================
// 解析结果
// ============================================================
typedef struct {
    MDFormat* fmts;     // 格式指令数组
    int n_fmts;         // 有效指令数量
} MDResult;

// ============================================================
// 接口
// ============================================================

// 解析 Markdown 文本
// text: 原始 Markdown 源码（UTF-16，以 \0 结尾）
// 返回值：格式指令数组（调用后需通过 MDFree 释放）
MDResult MDParse(const wchar_t* text);

// 释放解析结果
void MDFree(MDResult* result);

#endif

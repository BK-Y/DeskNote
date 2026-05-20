// md_parser.c - Markdown 解析器封装实现
#define MD4C_USE_UTF16
#include "md4c.h"
#include "md_parser.h"
#include <stdlib.h>
#include <wchar.h>

// ============================================================
// 解析上下文
// ============================================================
typedef struct {
    const wchar_t* base;    // 原始文本起始地址，用于计算偏移
    MDFormat* fmts;         // 收集到的格式指令
    int n_fmts;
    int cap_fmts;
    int span_type;          // 当前所在 span 类型（0=不在任何 span 内）
    int span_beg;           // 当前 span 的起始位置
    int span_end;           // 当前 span 的结束位置
} ParseCtx;

// ============================================================
// 辅助函数
// ============================================================

static void add_fmt(ParseCtx* ctx, int type, int beg, int end)
{
    if (ctx->n_fmts >= ctx->cap_fmts) {
        ctx->cap_fmts = ctx->cap_fmts ? ctx->cap_fmts * 2 : 32;
        MDFormat* new_fmts = realloc(ctx->fmts, ctx->cap_fmts * sizeof(MDFormat));
        if (!new_fmts) return;
        ctx->fmts = new_fmts;
    }
    ctx->fmts[ctx->n_fmts].type = type;
    ctx->fmts[ctx->n_fmts].beg = beg;
    ctx->fmts[ctx->n_fmts].end = end;
    ctx->n_fmts++;
}

// 通过文本指针计算它在原文中的偏移
static int text_off(ParseCtx* ctx, const MD_CHAR* text)
{
    return (int)(text - ctx->base);
}

// 在文本块中扫描 @ 标签
static int scan_at_tag(const wchar_t* text, int pos, int size)
{
    // @today, @tomorrow, @2025-01-20 等
    int i = pos + 1;
    while (i < pos + size && text[i] != L' ' && text[i] != L'\t' &&
           text[i] != L'\n' && text[i] != L'\r' && text[i] != L'\0')
        i++;
    return (i > pos + 1) ? i : 0;
}

// 在文本块中扫描 ! 标签
static int scan_bang_tag(const wchar_t* text, int pos, int size)
{
    int i = pos + 1;
    while (i < pos + size && text[i] != L' ' && text[i] != L'\t' &&
           text[i] != L'\n' && text[i] != L'\r' && text[i] != L'\0')
        i++;
    return (i > pos + 1) ? i : 0;
}

// 在文本块中扫描 # 标签（行内、# 后无空格）
static int scan_hash_tag(const wchar_t* text, int pos, int size)
{
    int i = pos + 1;
    while (i < pos + size && text[i] != L' ' && text[i] != L'\t' &&
           text[i] != L'\n' && text[i] != L'\r' && text[i] != L'\0')
        i++;
    return (i > pos + 1) ? i : 0;
}

// ============================================================
// md4c 回调
// ============================================================

static int enter_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    (void)detail;
    (void)userdata;
    return 0;
}

static int leave_block(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    (void)type;
    (void)detail;
    (void)userdata;
    return 0;
}

static int enter_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    ParseCtx* ctx = (ParseCtx*)userdata;
    (void)detail;
    ctx->span_type = (int)type;
    ctx->span_beg = -1;
    ctx->span_end = -1;
    return 0;
}

static int leave_span(MD_SPANTYPE type, void* detail, void* userdata)
{
    ParseCtx* ctx = (ParseCtx*)userdata;
    (void)detail;
    if (ctx->span_beg >= 0 && ctx->span_end > ctx->span_beg)
    {
        int fmt = 0;
        switch (type) {
            case MD_SPAN_EM:     fmt = MD_FMT_ITALIC; break;
            case MD_SPAN_STRONG: fmt = MD_FMT_BOLD; break;
            case MD_SPAN_CODE:   fmt = MD_FMT_CODE; break;
            case MD_SPAN_DEL:    fmt = MD_FMT_STRIKETHROUGH; break;
            default: break;
        }
        if (fmt)
            add_fmt(ctx, fmt, ctx->span_beg, ctx->span_end);
    }

    ctx->span_type = 0;
    return 0;
}

static int text_cb(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    ParseCtx* ctx = (ParseCtx*)userdata;
    (void)type;

    int off = text_off(ctx, text);
    int end = off + (int)size;

    // 追踪 span 边界
    if (ctx->span_type > 0) {
        if (ctx->span_beg < 0 || off < ctx->span_beg)
            ctx->span_beg = off;
        if (ctx->span_end < 0 || end > ctx->span_end)
            ctx->span_end = end;
    }

    // 扫描当前文本块中的 @ ! # 标签
    for (int i = 0; i < (int)size; i++) {
        if (text[i] == L'@') {
            int tag_end = scan_at_tag(text, i, (int)size);
            if (tag_end > 0)
                add_fmt(ctx, MD_FMT_DATE, off + i, off + tag_end);
        }
        else if (text[i] == L'!') {
            int tag_end = scan_bang_tag(text, i, (int)size);
            if (tag_end > 0)
                add_fmt(ctx, MD_FMT_PRIORITY, off + i, off + tag_end);
        }
        else if (text[i] == L'#' && (i == 0 || text[i-1] == L' ')) {
            int tag_end = scan_hash_tag(text, i, (int)size);
            if (tag_end > 0)
                add_fmt(ctx, MD_FMT_TAG, off + i, off + tag_end);
        }
    }

    return 0;
}

// ============================================================
// 公开接口
// ============================================================

MDResult MDParse(const wchar_t* text)
{
    MDResult result = { NULL, 0 };
    ParseCtx ctx = { 0 };

    if (!text || text[0] == L'\0')
        return result;

    ctx.base = text;

    MD_PARSER parser;
    memset(&parser, 0, sizeof(parser));
    parser.abi_version = 0;
    parser.flags = MD_FLAG_TASKLISTS | MD_FLAG_STRIKETHROUGH;
    parser.enter_block = enter_block;
    parser.leave_block = leave_block;
    parser.enter_span = enter_span;
    parser.leave_span = leave_span;
    parser.text = text_cb;

    int len = (int)wcslen(text);
    md_parse(text, len, &parser, &ctx);

    result.fmts = ctx.fmts;
    result.n_fmts = ctx.n_fmts;
    return result;
}

void MDFree(MDResult* result)
{
    if (result) {
        free(result->fmts);
        result->fmts = NULL;
        result->n_fmts = 0;
    }
}

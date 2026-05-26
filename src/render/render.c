#include "render.h"

#include <d2d1.h>
#include <dwrite.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/*
 * MinGW 当前环境下没有直接给出可链接的 IID_IDWriteFactory 定义，
 * 因此在模块内显式提供一份 IID 常量，避免把问题扩散到上层。
 */
static const IID Render_IID_IDWriteFactory =
    { 0xb859ee5a, 0xd838, 0x4b5b, { 0xa2, 0xe8, 0x1a, 0xdc, 0x7d, 0x93, 0xdb, 0x48 } };

/* render.h 只暴露不透明类型；真正的 Direct 资源只在 render.c 内可见。 */
struct RenderContext {
    HWND hwnd;
    ID2D1Factory* d2d_factory;
    ID2D1HwndRenderTarget* render_target;
    ID2D1SolidColorBrush* brush;
    IDWriteFactory* dwrite_factory;
    IDWriteTextFormat* text_format;
    int frame_active;
};

typedef struct {
    const wchar_t* text;
    UINT32 length;
    UINT32 mapped_position;
    wchar_t* owned_text;
} RenderPreparedText;

RenderContext* Render_Create(void)
{
    return (RenderContext*)calloc(1, sizeof(RenderContext));
}

void Render_Destroy(RenderContext* ctx)
{
    if (ctx == NULL)
        return;

    Render_Shutdown(ctx);
    free(ctx);
}

/* 所有 COM 接口最终都继承自 IUnknown，这里统一做 Release 和置空。 */
static void Render_ReleaseUnknown(void** unknown)
{
    if (unknown != NULL && *unknown != NULL)
    {
        ((IUnknown*)(*unknown))->lpVtbl->Release((IUnknown*)(*unknown));
        *unknown = NULL;
    }
}

/* render target 和 brush 都依赖窗口设备状态，丢失后需要成组重建。 */
static void Render_ReleaseDeviceResources(RenderContext* ctx)
{
    if (ctx == NULL)
        return;

    Render_ReleaseUnknown((void**)&ctx->brush);
    Render_ReleaseUnknown((void**)&ctx->render_target);
}

/* 将项目里的 0-255 颜色表示转换成 Direct2D 使用的 0-1 浮点表示。 */
static D2D1_COLOR_F Render_ToD2DColor(RenderColor color)
{
    D2D1_COLOR_F d2d_color;

    d2d_color.r = (FLOAT)color.r / 255.0f;
    d2d_color.g = (FLOAT)color.g / 255.0f;
    d2d_color.b = (FLOAT)color.b / 255.0f;
    d2d_color.a = (FLOAT)color.a / 255.0f;

    return d2d_color;
}

/* 上层统一传 RenderRect，render 层内部再转换成 Direct2D 的矩形类型。 */
static D2D1_RECT_F Render_ToD2DRect(RenderRect rect)
{
    D2D1_RECT_F d2d_rect;

    d2d_rect.left = (FLOAT)rect.x;
    d2d_rect.top = (FLOAT)rect.y;
    d2d_rect.right = (FLOAT)(rect.x + rect.width);
    d2d_rect.bottom = (FLOAT)(rect.y + rect.height);

    return d2d_rect;
}

static IDWriteTextLayout* Render_CreateTextLayout(RenderContext* ctx,
                                                  const wchar_t* text,
                                                  UINT32 length,
                                                  RenderRect rect)
{
    IDWriteTextLayout* text_layout;
    FLOAT max_width;
    FLOAT max_height;
    HRESULT hr;

    if (ctx == NULL || ctx->dwrite_factory == NULL || ctx->text_format == NULL || text == NULL)
        return NULL;

    text_layout = NULL;
    max_width = rect.width > 0 ? (FLOAT)rect.width : 1.0f;
    max_height = rect.height > 0 ? (FLOAT)rect.height : 1.0f;

    hr = ctx->dwrite_factory->lpVtbl->CreateTextLayout(
        ctx->dwrite_factory,
        text,
        length,
        ctx->text_format,
        max_width,
        max_height,
        &text_layout);
    if (FAILED(hr))
        return NULL;

    hr = text_layout->lpVtbl->SetWordWrapping(
        text_layout,
        DWRITE_WORD_WRAPPING_WRAP);
    if (FAILED(hr))
    {
        Render_ReleaseUnknown((void**)&text_layout);
        return NULL;
    }

    hr = text_layout->lpVtbl->SetLineSpacing(
        text_layout,
        DWRITE_LINE_SPACING_METHOD_UNIFORM,
        20.0f,
        12.0f);
    if (FAILED(hr))
    {
        Render_ReleaseUnknown((void**)&text_layout);
        return NULL;
    }

    return text_layout;
}

static int Render_PrepareText(const wchar_t* text,
                              UINT32 length,
                              UINT32 text_position,
                              RenderPreparedText* prepared)
{
    UINT32 newline_count;
    UINT32 i;
    UINT32 j;

    if (text == NULL || prepared == NULL)
        return 1;

    memset(prepared, 0, sizeof(*prepared));
    newline_count = 0;
    for (i = 0; i < length; ++i)
    {
        if (text[i] == L'\n')
            newline_count += 1;
    }

    if (newline_count == 0)
    {
        prepared->text = text;
        prepared->length = length;
        prepared->mapped_position = text_position;
        return 0;
    }

    prepared->owned_text = (wchar_t*)malloc((size_t)(length + newline_count + 1) * sizeof(wchar_t));
    if (prepared->owned_text == NULL)
        return 1;

    j = 0;
    prepared->mapped_position = 0;
    for (i = 0; i < length; ++i)
    {
        if (i == text_position)
            prepared->mapped_position = j;

        if (text[i] == L'\n')
        {
            prepared->owned_text[j++] = L'\r';
            prepared->owned_text[j++] = L'\n';
        }
        else
        {
            prepared->owned_text[j++] = text[i];
        }
    }

    if (text_position == length)
        prepared->mapped_position = j;

    prepared->owned_text[j] = L'\0';
    prepared->text = prepared->owned_text;
    prepared->length = j;
    return 0;
}

static void Render_FreePreparedText(RenderPreparedText* prepared)
{
    if (prepared == NULL)
        return;

    free(prepared->owned_text);
    prepared->owned_text = NULL;
}

/*
 * 创建与窗口绑定的设备资源。
 *
 * 工厂和文本格式属于长期资源；
 * render target 和 brush 更接近设备资源，窗口 resize 或 target 丢失时可能需要重建。
 */
static HRESULT Render_CreateTarget(RenderContext* ctx)
{
    RECT client_rect;
    D2D1_RENDER_TARGET_PROPERTIES render_target_props;
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_target_props;
    ID2D1RenderTarget* base_target;
    D2D1_COLOR_F default_color;
    HRESULT hr;

    if (ctx == NULL || ctx->hwnd == NULL || ctx->d2d_factory == NULL)
        return E_INVALIDARG;

    if (!GetClientRect(ctx->hwnd, &client_rect))
        return HRESULT_FROM_WIN32(GetLastError());

    render_target_props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    render_target_props.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
    render_target_props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
    render_target_props.dpiX = 0.0f;
    render_target_props.dpiY = 0.0f;
    render_target_props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
    render_target_props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

    hwnd_target_props.hwnd = ctx->hwnd;
    hwnd_target_props.pixelSize.width = (UINT32)(client_rect.right - client_rect.left);
    hwnd_target_props.pixelSize.height = (UINT32)(client_rect.bottom - client_rect.top);
    hwnd_target_props.presentOptions = D2D1_PRESENT_OPTIONS_NONE;

    hr = ctx->d2d_factory->lpVtbl->CreateHwndRenderTarget(
        ctx->d2d_factory,
        &render_target_props,
        &hwnd_target_props,
        &ctx->render_target);
    if (FAILED(hr))
        return hr;

    /* 大部分绘制方法定义在 ID2D1RenderTarget 基接口上，而不是 hwnd target 自身。 */
    base_target = (ID2D1RenderTarget*)ctx->render_target;
    default_color = Render_ToD2DColor((RenderColor){ 0, 0, 0, 255 });
    hr = base_target->lpVtbl->CreateSolidColorBrush(
        base_target,
        &default_color,
        NULL,
        &ctx->brush);
    if (FAILED(hr))
    {
        Render_ReleaseDeviceResources(ctx);
        return hr;
    }

    return S_OK;
}

int Render_Init(RenderContext* ctx, HWND hwnd)
{
    D2D1_FACTORY_OPTIONS factory_options;
    HRESULT hr;

    if (ctx == NULL || hwnd == NULL)
        return 1;

    memset(ctx, 0, sizeof(*ctx));
    ctx->hwnd = hwnd;

    memset(&factory_options, 0, sizeof(factory_options));

    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &IID_ID2D1Factory,
        &factory_options,
        (void**)&ctx->d2d_factory);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        &Render_IID_IDWriteFactory,
        (IUnknown**)&ctx->dwrite_factory);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    /* 当前阶段先固定一个默认文本格式，后续再拆成主题或字体配置。 */
    hr = ctx->dwrite_factory->lpVtbl->CreateTextFormat(
        ctx->dwrite_factory,
        L"Segoe UI",
        NULL,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        16.0f,
        L"",
        &ctx->text_format);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    hr = ctx->text_format->lpVtbl->SetTextAlignment(
        ctx->text_format,
        DWRITE_TEXT_ALIGNMENT_LEADING);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    hr = ctx->text_format->lpVtbl->SetParagraphAlignment(
        ctx->text_format,
        DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    hr = Render_CreateTarget(ctx);
    if (FAILED(hr))
    {
        Render_Shutdown(ctx);
        return 1;
    }

    return 0;
}

void Render_Shutdown(RenderContext* ctx)
{
    if (ctx == NULL)
        return;

    ctx->frame_active = 0;

    Render_ReleaseDeviceResources(ctx);
    Render_ReleaseUnknown((void**)&ctx->text_format);
    Render_ReleaseUnknown((void**)&ctx->dwrite_factory);
    Render_ReleaseUnknown((void**)&ctx->d2d_factory);
    ctx->hwnd = NULL;
}

int Render_Resize(RenderContext* ctx, unsigned int width, unsigned int height)
{
    HRESULT hr;
    D2D1_SIZE_U size;

    if (ctx == NULL)
        return 1;

    if (ctx->render_target == NULL)
    {
        hr = Render_CreateTarget(ctx);
        return FAILED(hr) ? 1 : 0;
    }

    size.width = width;
    size.height = height;

    hr = ctx->render_target->lpVtbl->Resize(
        ctx->render_target,
        &size);

    /* 设备丢失后，Direct2D 要求释放旧 target 再重新创建。 */
    if (hr == D2DERR_RECREATE_TARGET)
    {
        Render_ReleaseDeviceResources(ctx);
        hr = Render_CreateTarget(ctx);
    }

    return FAILED(hr) ? 1 : 0;
}

int Render_BeginFrame(RenderContext* ctx)
{
    ID2D1RenderTarget* base_target;
    HRESULT hr;

    if (ctx == NULL)
        return 1;

    if (ctx->render_target == NULL)
    {
        hr = Render_CreateTarget(ctx);
        if (FAILED(hr))
            return 1;
    }

    /* BeginDraw / EndDraw 同样属于基接口方法。 */
    base_target = (ID2D1RenderTarget*)ctx->render_target;
    base_target->lpVtbl->BeginDraw(base_target);
    ctx->frame_active = 1;

    return 0;
}

int Render_EndFrame(RenderContext* ctx)
{
    ID2D1RenderTarget* base_target;
    HRESULT hr;

    if (ctx == NULL || ctx->render_target == NULL)
        return 1;

    if (!ctx->frame_active)
        return 0;

    base_target = (ID2D1RenderTarget*)ctx->render_target;
    hr = base_target->lpVtbl->EndDraw(base_target, NULL, NULL);
    ctx->frame_active = 0;

    if (hr == D2DERR_RECREATE_TARGET)
    {
        Render_ReleaseDeviceResources(ctx);
        return 1;
    }

    return FAILED(hr) ? 1 : 0;
}

void Render_Clear(RenderContext* ctx, RenderColor color)
{
    ID2D1RenderTarget* base_target;
    D2D1_COLOR_F d2d_color;

    if (ctx == NULL || ctx->render_target == NULL)
        return;

    base_target = (ID2D1RenderTarget*)ctx->render_target;
    d2d_color = Render_ToD2DColor(color);
    base_target->lpVtbl->Clear(base_target, &d2d_color);
}

void Render_FillRect(RenderContext* ctx, RenderRect rect, RenderColor color)
{
    ID2D1RenderTarget* base_target;
    D2D1_RECT_F d2d_rect;
    D2D1_COLOR_F d2d_color;

    if (ctx == NULL || ctx->render_target == NULL || ctx->brush == NULL)
        return;

    base_target = (ID2D1RenderTarget*)ctx->render_target;
    d2d_rect = Render_ToD2DRect(rect);
    d2d_color = Render_ToD2DColor(color);

    ctx->brush->lpVtbl->SetColor(ctx->brush, &d2d_color);
    base_target->lpVtbl->FillRectangle(
        base_target,
        &d2d_rect,
        (ID2D1Brush*)ctx->brush);
}

void Render_DrawText(RenderContext* ctx,
                     const wchar_t* text,
                     RenderRect rect,
                     RenderColor color)
{
    ID2D1RenderTarget* base_target;
    IDWriteTextLayout* text_layout;
    RenderPreparedText prepared;
    D2D1_COLOR_F d2d_color;
    D2D1_POINT_2F origin;
    UINT32 length;

    if (ctx == NULL || ctx->render_target == NULL || ctx->brush == NULL ||
        ctx->text_format == NULL || text == NULL)
        return;

    base_target = (ID2D1RenderTarget*)ctx->render_target;
    d2d_color = Render_ToD2DColor(color);
    length = (UINT32)wcslen(text);
    if (Render_PrepareText(text, length, 0, &prepared) != 0)
        return;
    text_layout = Render_CreateTextLayout(ctx, prepared.text, prepared.length, rect);
    if (text_layout == NULL)
    {
        Render_FreePreparedText(&prepared);
        return;
    }
    origin.x = (FLOAT)rect.x;
    origin.y = (FLOAT)rect.y;

    ctx->brush->lpVtbl->SetColor(ctx->brush, &d2d_color);
    base_target->lpVtbl->DrawTextLayout(
        base_target,
        origin,
        text_layout,
        (ID2D1Brush*)ctx->brush,
        D2D1_DRAW_TEXT_OPTIONS_NONE);
    Render_ReleaseUnknown((void**)&text_layout);
    Render_FreePreparedText(&prepared);
}

int Render_HitTestTextPosition(RenderContext* ctx,
                               const wchar_t* text,
                               int text_length,
                               RenderRect rect,
                               int text_position,
                               RenderTextPosition* out_position)
{
    IDWriteTextLayout* text_layout;
    RenderPreparedText prepared;
    DWRITE_HIT_TEST_METRICS metrics;
    FLOAT x;
    FLOAT y;
    HRESULT hr;

    if (ctx == NULL || text == NULL || out_position == NULL)
        return 1;
    if (text_length < 0)
        return 1;

    if (text_position < 0)
        text_position = 0;
    if (text_position > text_length)
        text_position = text_length;

    if (Render_PrepareText(text, (UINT32)text_length, (UINT32)text_position, &prepared) != 0)
        return 1;
    text_layout = Render_CreateTextLayout(ctx, prepared.text, prepared.length, rect);
    if (text_layout == NULL)
    {
        Render_FreePreparedText(&prepared);
        return 1;
    }

    x = 0.0f;
    y = 0.0f;
    memset(&metrics, 0, sizeof(metrics));

    hr = text_layout->lpVtbl->HitTestTextPosition(
        text_layout,
        prepared.mapped_position,
        FALSE,
        &x,
        &y,
        &metrics);
    if (FAILED(hr))
    {
        Render_ReleaseUnknown((void**)&text_layout);
        Render_FreePreparedText(&prepared);
        return 1;
    }

    out_position->x = rect.x + (int)x;
    out_position->y = rect.y + (int)y;
    out_position->height = metrics.height > 0.0f ? (int)metrics.height : 18;

    Render_ReleaseUnknown((void**)&text_layout);
    Render_FreePreparedText(&prepared);
    return 0;
}

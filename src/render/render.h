#ifndef DESKNOTE_RENDER_H
#define DESKNOTE_RENDER_H

#include <windows.h>

/*
 * RGBA 颜色描述。
 *
 * render 层当前统一使用 8-bit 通道颜色，便于上层组件直接表达：
 * - 背景色
 * - 边框色
 * - 文字色
 * - 高亮色
 *
 * 其中 a 通道在当前阶段主要作为接口保留位存在。
 * 后续如果底层渲染实现接入透明混合，可以在不修改接口形状的前提下继续使用。
 */
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} RenderColor;

/*
 * 轴对齐矩形区域。
 *
 * render 层只关心“在某个矩形区域里画东西”，
 * 不关心这个区域到底代表：
 * - 编辑区
 * - 标题栏
 * - 状态栏
 * - 选区
 * - 高亮块
 *
 * 坐标系约定：
 * - 使用窗口客户区坐标
 * - x / y 为左上角
 * - width / height 为区域尺寸
 */
typedef struct {
    int x;
    int y;
    int width;
    int height;
} RenderRect;

/*
 * 文本位置命中结果。
 *
 * 用途：
 * - 让上层根据文本中的某个字符位置拿到像素坐标
 * - 为光标、输入法组合窗口、命中测试等功能提供基础坐标
 *
 * 约束：
 * - render 只负责提供“文本位置 -> 像素位置”的测量结果
 * - 这个位置最终在业务上代表什么，由上层决定
 */
typedef struct {
    int x;
    int y;
    int height;
} RenderTextPosition;

/*
 * 渲染上下文。
 *
 * 这是 render 模块对外暴露的核心句柄类型，用于承载一个窗口对应的渲染资源。
 *
 * 当前 Windows 路径直接采用 Direct2D + DirectWrite，
 * 但上层不应直接知道这些底层 COM 对象的细节，因此这里采用不透明结构体声明：
 *
 * - `render.c` 负责真正定义结构体内容
 * - `ui/`、`app/` 只持有 `RenderContext*`
 *
 * 这样可以保持边界：
 * - 上层知道“有一个渲染上下文”
 * - 上层不知道“内部到底持有哪些 Direct 资源”
 */
typedef struct RenderContext RenderContext;

/*
 * 创建一个新的渲染上下文实例。
 *
 * 用途：
 * - 为上层提供一个不需要知道结构体内部字段的创建入口
 * - 让 `RenderContext` 可以继续保持不透明类型
 *
 * 返回值：
 * - 成功时返回新分配的上下文指针
 * - 失败时返回 `NULL`
 *
 * 注意：
 * - 该函数只分配内存，不初始化 Direct 资源
 * - 真实资源初始化仍由 `Render_Init` 负责
 */
RenderContext* Render_Create(void);

/*
 * 销毁一个渲染上下文实例。
 *
 * 用途：
 * - 释放 `Render_Create` 分配的上下文内存
 * - 保证在销毁实例前先正确执行 `Render_Shutdown`
 *
 * 注意：
 * - 调用前允许上下文已经处于 shutdown 状态
 * - 调用后该指针不再可用
 */
void Render_Destroy(RenderContext* ctx);

/*
 * 初始化渲染上下文。
 *
 * 调用时机：
 * - 通常在窗口创建成功后尽早调用
 * - 一个窗口对应一个 RenderContext
 *
 * 作用：
 * - 建立 render 层的长期资源
 * - 绑定窗口句柄
 * - 为后续 `Render_BeginFrame` / `Render_DrawText` 等调用做准备
 *
 * 当前阶段内部预期会初始化：
 * - Direct2D factory
 * - DirectWrite factory
 * - hwnd render target
 * - 默认画刷
 * - 默认文本格式
 *
 * 参数：
 * - ctx  : 需要初始化的渲染上下文指针
 * - hwnd : 目标窗口句柄
 *
 * 返回值：
 * - `0` 表示成功
 * - 非 `0` 表示失败
 *
 * 边界约束：
 * - 该函数只负责 render 层资源初始化
 * - 不负责创建窗口
 * - 不负责决定界面内容
 */
int Render_Init(RenderContext* ctx, HWND hwnd);

/*
 * 释放渲染上下文持有的资源。
 *
 * 调用时机：
 * - 通常在窗口销毁前或应用退出阶段调用
 * - 必须与 `Render_Init` 成对出现
 *
 * 作用：
 * - 释放 render target
 * - 释放 Direct2D / DirectWrite 相关资源
 * - 将上下文恢复到未初始化状态
 *
 * 注意：
 * - 该函数不负责销毁窗口本身
 * - 调用后该 `RenderContext` 不能继续用于绘制，除非再次 `Render_Init`
 */
void Render_Shutdown(RenderContext* ctx);

/*
 * 响应窗口尺寸变化，调整渲染目标大小。
 *
 * 调用时机：
 * - 通常在 `WM_SIZE` 后调用
 * - 当窗口客户区宽高发生变化时调用
 *
 * 作用：
 * - 让底层渲染目标与当前窗口客户区大小保持一致
 * - 为后续绘制提供正确的目标区域
 *
 * 参数：
 * - ctx    : 已初始化的渲染上下文
 * - width  : 新的客户区宽度
 * - height : 新的客户区高度
 *
 * 返回值：
 * - `0` 表示成功
 * - 非 `0` 表示失败
 *
 * 注意：
 * - 这里只处理 render target 尺寸
 * - 不负责重排 UI 布局
 * - UI 布局仍由 `ui/` 或 `app/` 决定
 */
int Render_Resize(RenderContext* ctx, unsigned int width, unsigned int height);

/*
 * 开始一帧绘制。
 *
 * 调用时机：
 * - 通常在 `WM_PAINT` 流程中
 * - 在 app 进入本帧所有绘制调用之前执行
 *
 * 作用：
 * - 告诉底层渲染后端“本帧开始”
 * - 准备本帧绘制所需状态
 *
 * 返回值：
 * - `0` 表示成功，可以继续当前帧绘制
 * - 非 `0` 表示失败，调用方应跳过本帧后续绘制
 *
 * 边界约束：
 * - 该函数不决定要画什么
 * - 它只负责为“画”这件事做底层准备
 */
int Render_BeginFrame(RenderContext* ctx);

/*
 * 结束一帧绘制。
 *
 * 调用时机：
 * - 在本帧所有绘制命令发出后调用
 * - 与 `Render_BeginFrame` 成对出现
 *
 * 作用：
 * - 提交本帧绘制结果
 * - 执行本帧收尾逻辑
 * - 为下一帧绘制恢复到稳定状态
 *
 * 返回值：
 * - `0` 表示成功
 * - 非 `0` 表示失败
 *
 * 注意：
 * - 该函数不负责调用 `BeginPaint` / `EndPaint`
 * - 系统消息级别的绘制生命周期仍由 platform 层控制
 */
int Render_EndFrame(RenderContext* ctx);

/*
 * 清空整个窗口客户区背景。
 *
 * 用途：
 * - 为整帧铺底色
 * - 在正式绘制 UI 组件前建立统一背景
 *
 * 参数：
 * - ctx   : 当前帧的渲染上下文
 * - color : 整个客户区需要使用的背景色
 *
 * 约束：
 * - render 只负责“怎么清”
 * - 是否要清背景、为什么用这个颜色，由上层决定
 */
void Render_Clear(RenderContext* ctx, RenderColor color);

/*
 * 填充一个矩形区域。
 *
 * 当前阶段主要用于：
 * - 绘制编辑区背景
 * - 绘制面板底色
 * - 绘制高亮区域
 * - 绘制提示块背景
 *
 * 参数：
 * - ctx   : 当前帧的渲染上下文
 * - rect  : 目标矩形区域
 * - color : 填充色
 *
 * 边界约束：
 * - render 只负责把这个矩形画出来
 * - 这个矩形在业务上代表什么，由 `ui/` 或其他上层模块定义
 */
void Render_FillRect(RenderContext* ctx, RenderRect rect, RenderColor color);

/*
 * 在指定矩形区域中绘制文本。
 *
 * 当前阶段这是最小文本输出接口，主要用于：
 * - 绘制占位文字
 * - 绘制调试信息
 * - 为后续正文显示建立最小通路
 *
 * 参数：
 * - ctx   : 当前帧的渲染上下文
 * - text  : UTF-16 字符串，以 `L'\0'` 结尾
 * - rect  : 文本目标区域
 * - color : 文本颜色
 *
 * 当前不负责：
 * - 富文本 run 拆分
 * - 精细排版
 * - 命中测试
 * - 文本测量接口
 * - 光标与选区语义
 *
 * 后续扩展方向：
 * - 文本对齐
 * - 字体 / 字号
 * - 文本测量
 * - 裁剪区域
 * - 富文本样式绘制
 */
void Render_DrawText(RenderContext* ctx,
                     const wchar_t* text,
                     RenderRect rect,
                     RenderColor color);

/*
 * 命中文本中的某个逻辑位置，并返回对应像素坐标。
 *
 * 参数：
 * - ctx           : 已初始化的渲染上下文
 * - text          : UTF-16 文本内容
 * - text_length   : 文本长度（不含 `L'\0'`）
 * - rect          : 文本布局区域
 * - text_position : 需要命中的逻辑位置，范围应在 [0, text_length]
 * - out_position  : 输出的像素位置与行高
 *
 * 返回值：
 * - `0` 表示成功
 * - 非 `0` 表示失败
 */
int Render_HitTestTextPosition(RenderContext* ctx,
                               const wchar_t* text,
                               int text_length,
                               RenderRect rect,
                               int text_position,
                               RenderTextPosition* out_position);

#endif

#ifndef JADEVIEW_H
#define JADEVIEW_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * WebView设置选项结构体，C-compatible
 * 用于设置webview的高级选项
 */
typedef struct WebViewSettings {
  /**
   * 是否允许自动播放媒体 (0=false, 1=true)
   */
  int32_t autoplay;
  /**
   * 背景限速策略 (0=默认, 1=禁用背景限速)
   */
  int32_t background_throttling;
  /**
   * 是否允许页面右键菜单 (0=禁用, 1=允许)
   */
  int32_t allow_right_click;
  /**
   * 自定义User-Agent字符串指针，NULL表示使用默认UA
   */
  const char *ua;
  /**
   * 预载JavaScript代码字符串指针，NULL表示不预载JS
   */
  const char *preload_js;
  /**
   * 是否允许页面全屏 (0=false, 1=true)
   */
  int32_t allow_fullscreen;
  /**
   * PostMessage白名单，单个域名字符串
   */
  const char *postmessage_whitelist;
  /**
   * CORS来源白名单，逗号分隔多个域名
   */
  const char *cors_whitelist;
  /**
   * 是否启用账号/密码自动填充 (0=禁用, 1=启用)
   */
  int32_t autofill;
  /**
   * 是否启用通用表单自动填充 (0=禁用, 1=启用)
   */
  int32_t general_autofill_enabled;
  /**
   * 无痕模式 (0=否, 1=是)
   */
  int32_t incognito;
  /**
   * 禁用剪贴板读写权限 (0=启用, 1=禁用)
   */
  int32_t disable_clipboard;
  /**
   * 代理URL字符串指针(NULL=不使用代理)，格式如 "http://host:port" 或 "socks5://host:port"
   */
  const char *proxy_url;
  /**
   * 初始是否自动获取焦点 (0=否, 1=是)
   */
  int32_t focused;
} WebViewSettings;

/**
 * WebView窗口选项结构体，C-compatible
 * 字段顺序必须与create_webview_window函数中的解构赋值顺序完全匹配
 * 字段数量和类型必须与易语言的JadeView窗口设置结构体完全匹配
 */
typedef struct WebViewWindowOptions {
  /**
   * 标题
   */
  const char *title;
  /**
   * 宽度
   */
  int32_t width;
  /**
   * 高度
   */
  int32_t height;
  /**
   * 可调整大小边框
   */
  int32_t resizable;
  /**
   * 窗口边框样式：normal / no-titlebar / borderless
   */
  const char *frame_style;
  /**
   * 透明背景
   */
  int32_t transparent;
  /**
   * 背景颜色："#RRGGBBAA" 十六进制字符串，例如 "#ffffff00"
   */
  const char *background_color;
  /**
   * 置顶窗口
   */
  int32_t always_on_top;
  /**
   * 窗口主题
   */
  const char *theme;
  /**
   * 最大化
   */
  int32_t maximized;
  /**
   * 最大化按钮
   */
  int32_t maximizable;
  /**
   * 最小化按钮
   */
  int32_t minimizable;
  /**
   * X坐标（-1 表示居中）
   */
  int32_t x;
  /**
   * Y坐标（-1 表示居中）
   */
  int32_t y;
  /**
   * 最小宽度
   */
  int32_t min_width;
  /**
   * 最小高度
   */
  int32_t min_height;
  /**
   * 最大宽度
   */
  int32_t max_width;
  /**
   * 最大高度
   */
  int32_t max_height;
  /**
   * 全屏
   */
  int32_t fullscreen;
  /**
   * 焦点
   */
  int32_t focus;
  /**
   * 隐藏窗口
   */
  int32_t hide_window;
  /**
   * 使用页面图标
   */
  int32_t use_page_icon;
  /**
   * 内容保护
   */
  int32_t content_protection;
  /**
   * 自动保存窗口状态（0=关闭，1=开启）
   */
  int32_t auto_save_state;
  /**
   * 不进任务栏/Alt-Tab（0=否，1=是）。追加在末尾以保持结构体字段顺序兼容（易语言/火山 SDK）
   */
  int32_t skip_taskbar;
  /**
   * 不抢焦点：点击/显示不激活窗口（0=否，1=是）。追加在末尾以保持兼容
   */
  int32_t no_activate;
} WebViewWindowOptions;

/**
 * 通知参数结构体
 */
typedef struct NotificationParams {
  /**
   * 通知摘要/标题
   */
  const char *summary;
  /**
   * 通知正文
   */
  const char *body;
  /**
   * 通知图标路径
   */
  const char *icon;
  /**
   * 通知超时时间（毫秒，-1=系统默认）
   */
  int32_t timeout;
  /**
   * 按钮1文本
   */
  const char *button1;
  /**
   * 按钮2文本
   */
  const char *button2;
  /**
   * 按钮3文本
   */
  const char *text3;
  /**
   * 默认操作动作
   */
  const char *action;
} NotificationParams;

/**
 * 文件对话框参数结构体
 */
typedef struct FileDialogParams {
  /**
   * 关联窗口ID
   */
  uint32_t window_id;
  /**
   * 对话框标题
   */
  const char *title;
  /**
   * 默认路径
   */
  const char *default_path;
  /**
   * 确认按钮文本
   */
  const char *button_label;
  /**
   * 文件过滤器，JSON格式
   */
  const char *filters;
  /**
   * 对话框属性，JSON格式
   */
  const char *properties;
} FileDialogParams;

/**
 * 消息框参数结构体
 */
typedef struct MessageBoxParams {
  /**
   * 关联窗口ID
   */
  uint32_t window_id;
  /**
   * 消息框标题
   */
  const char *title;
  /**
   * 消息正文
   */
  const char *message;
  /**
   * 详细信息
   */
  const char *detail;
  /**
   * 按钮列表，JSON格式
   */
  const char *buttons;
  /**
   * 默认按钮索引
   */
  int32_t default_id;
  /**
   * 取消按钮索引
   */
  int32_t cancel_id;
  /**
   * 消息框类型：none / info / warning / error
   */
  const char *type_;
} MessageBoxParams;

/**
 * C ABI：与 `JadeView.h` 中 `TrayMenuItemDesc` 一致（用 `parent_key` 指向父项的 `key`，无数组下标）。
 *
 * `item_type`：`0` NORMAL、`1` SUBMENU、`2` DIVIDER、`3` GROUP。
 * `key`：全表唯一，非空 UTF-8（含分隔线也需唯一 key，供内部引用）。
 * `parent_key`：NULL 或空字符串表示根下子项；否则须等于某行的 `key`，且该行须为 SUBMENU 或 GROUP。
 */
typedef struct TrayMenuItemDesc {
  int32_t item_type;
  const char *key;
  const char *label;
  const char *parent_key;
  int32_t disabled;
  int32_t dangerous;
} TrayMenuItemDesc;


// -----------------------
// 事件名常量（自动从源码提取）
// -----------------------
// 应用初始化完成
#define JADEVIEW_EVENT_APP_READY "app-ready"
// 右键菜单
#define JADEVIEW_EVENT_CONTEXT_MENU "context-menu"
// 程序崩溃
#define JADEVIEW_EVENT_CRASH "crash"
// 拖拽事件（enter/over/drop/leave）
#define JADEVIEW_EVENT_DRAG_DROP "drag-drop"
// 全局热键触发
#define JADEVIEW_EVENT_GLOBAL_HOTKEY "global-hotkey"
// JAPK 资源包加载失败
#define JADEVIEW_EVENT_JAPK_LOAD_FAILED "japk-load-failed"
// JavaScript 执行结果
#define JADEVIEW_EVENT_JAVASCRIPT_RESULT "javascript-result"
// 菜单项点击
#define JADEVIEW_EVENT_MENU_ITEM_CLICKED "menu-item-clicked"
// 通知操作被点击
#define JADEVIEW_EVENT_NOTIFICATION_ACTION "notification-action"
// 通知已关闭
#define JADEVIEW_EVENT_NOTIFICATION_DISMISSED "notification-dismissed"
// 通知显示失败
#define JADEVIEW_EVENT_NOTIFICATION_FAILED "notification-failed"
// 通知已显示
#define JADEVIEW_EVENT_NOTIFICATION_SHOWN "notification-shown"
// 收到前端 postMessage
#define JADEVIEW_EVENT_POSTMESSAGE_RECEIVED "postmessage-received"
// 第二个实例启动
#define JADEVIEW_EVENT_SECOND_INSTANCE "second-instance"
// 系统主题变化
#define JADEVIEW_EVENT_THEME_CHANGED "theme-changed"
// 托盘图标事件
#define JADEVIEW_EVENT_TRAY_EVENT "tray-event"
// 托盘菜单命令
#define JADEVIEW_EVENT_TRAY_MENU_COMMAND "tray-menu-command"
// 更新窗口图标
#define JADEVIEW_EVENT_UPDATE_WINDOW_ICON "update-window-icon"
// WebView 加载完成
#define JADEVIEW_EVENT_WEBVIEW_DID_FINISH_LOAD "webview-did-finish-load"
// WebView 开始加载
#define JADEVIEW_EVENT_WEBVIEW_DID_START_LOADING "webview-did-start-loading"
#define JADEVIEW_EVENT_WEBVIEW_DOWNLOAD_COMPLETED "webview-download-completed"
// WebView 页面图标更新
#define JADEVIEW_EVENT_WEBVIEW_PAGE_FAVICON_UPDATED "webview-page-favicon-updated"
// WebView 页面标题更新
#define JADEVIEW_EVENT_WEBVIEW_PAGE_TITLE_UPDATED "webview-page-title-updated"
// 所有窗口已关闭
#define JADEVIEW_EVENT_WINDOW_ALL_CLOSED "window-all-closed"
// 窗口失去焦点
#define JADEVIEW_EVENT_WINDOW_BLURRED "window-blurred"
#define JADEVIEW_EVENT_WINDOW_BOUNDS "window-bounds"
// 窗口关闭
#define JADEVIEW_EVENT_WINDOW_CLOSED "window-closed"
// 窗口创建完成
#define JADEVIEW_EVENT_WINDOW_CREATED "window-created"
// 窗口销毁
#define JADEVIEW_EVENT_WINDOW_DESTROYED "window-destroyed"
// 窗口获得焦点
#define JADEVIEW_EVENT_WINDOW_FOCUSED "window-focused"
// 窗口全屏状态变化
#define JADEVIEW_EVENT_WINDOW_FULLSCREEN "window-fullscreen"
// 窗口移动
#define JADEVIEW_EVENT_WINDOW_MOVED "window-moved"
// 窗口大小改变
#define JADEVIEW_EVENT_WINDOW_RESIZED "window-resized"
// 窗口状态变化（最大化/还原等）
#define JADEVIEW_EVENT_WINDOW_STATE_CHANGED "window-state-changed"


// -----------------------
// 崩溃错误代码（crash 事件 event_data）
// -----------------------
// SEH 异常：内存访问违规
#define JADEVIEW_CRASH_SEH_ACCESS_VIOLATION "SEH_ACCESS_VIOLATION"
// SEH 异常：栈溢出
#define JADEVIEW_CRASH_SEH_STACK_OVERFLOW "SEH_STACK_OVERFLOW"
// SEH 异常：非法指令
#define JADEVIEW_CRASH_SEH_ILLEGAL_INSTRUCTION "SEH_ILLEGAL_INSTRUCTION"
// SEH 异常：无效句柄
#define JADEVIEW_CRASH_SEH_INVALID_HANDLE "SEH_INVALID_HANDLE"
// SEH 异常：未知
#define JADEVIEW_CRASH_SEH_UNKNOWN "SEH_UNKNOWN"
// 运行时异常：Panic
#define JADEVIEW_CRASH_RUNTIME_PANIC "RUNTIME_PANIC"
// WebView2：浏览器进程已退出
#define JADEVIEW_CRASH_WV2_BROWSER_EXITED "WV2_BROWSER_EXITED"
// WebView2：渲染进程已退出
#define JADEVIEW_CRASH_WV2_RENDER_EXITED "WV2_RENDER_EXITED"
// WebView2：渲染进程无响应
#define JADEVIEW_CRASH_WV2_RENDER_UNRESPONSIVE "WV2_RENDER_UNRESPONSIVE"
// WebView2：框架渲染进程已退出
#define JADEVIEW_CRASH_WV2_FRAME_RENDER_EXITED "WV2_FRAME_RENDER_EXITED"
// WebView2：工具进程已退出
#define JADEVIEW_CRASH_WV2_UTILITY_EXITED "WV2_UTILITY_EXITED"
// WebView2：沙箱辅助进程已退出
#define JADEVIEW_CRASH_WV2_SANDBOX_HELPER_EXITED "WV2_SANDBOX_HELPER_EXITED"
// WebView2：GPU进程已退出
#define JADEVIEW_CRASH_WV2_GPU_EXITED "WV2_GPU_EXITED"
// WebView2：PPAPI插件进程已退出
#define JADEVIEW_CRASH_WV2_PPAPI_PLUGIN_EXITED "WV2_PPAPI_PLUGIN_EXITED"
// WebView2：PPAPI代理进程已退出
#define JADEVIEW_CRASH_WV2_PPAPI_BROKER_EXITED "WV2_PPAPI_BROKER_EXITED"
// WebView2：未知进程已退出
#define JADEVIEW_CRASH_WV2_UNKNOWN_EXITED "WV2_UNKNOWN_EXITED"



// -----------------------
// C API 函数声明（自动从 src/ffi/ 生成）
// -----------------------

#ifndef JADEVIEW_CALL
#define JADEVIEW_CALL __stdcall
#endif

// 回调函数类型定义
typedef const char* (JADEVIEW_CALL *IpcCallback)(uint32_t, const char*);

#ifdef __cplusplus
extern "C" {
#endif

// --- from japk_api.rs ---
// 设置公钥 (必须在加载 JAPK 之前调用)
// # 参数
// - `public_key`: Base64 编码的 Ed25519 公钥 (44 字符)
// # 返回
// - 0=成功, 负数=错误码
int32_t JADEVIEW_CALL JadeView_set_public_key(const char* public_key);
// 从内存加载 JAPK 文件（支持签名包和混淆包）
// # 参数
// - `japk_data`: JAPK 文件数据指针
// - `data_size`: 数据大小
// # 返回
// - 0=成功, 负数=错误码
// # 加载逻辑
// | 公钥设置 | 数据格式 | 结果 |
// |----------|----------|------|
// | 已设置 | JAPK v2 签名包 | 验证签名后加载 |
// | 已设置 | 其他格式 | 返回错误 |
// | 未设置 | JAPK v2 签名包 | 返回错误（需要公钥）|
// | 未设置 | 混淆包 (JPKBIN02) | 解混淆后加载 |
// | 未设置 | 其他格式 | 返回错误 |
// # 说明
// - 如果设置了公钥，必须是签名包，不会回退到混淆包逻辑
// - app_name 和 app_signature 必须与 JadeView_init 时设置的一致
// - 错误信息通过 jade_on 事件异步通知
int32_t JADEVIEW_CALL JadeView_load_from_bytes(const uint8_t* japk_data, size_t data_size);
// 获取加载状态
// # 返回
// - true=已加载, false=未加载
int32_t JADEVIEW_CALL JadeView_is_loaded(void);
// 获取当前 app_signature
// # 返回
// - app_signature 字符串指针 (调用方需使用 jade_text_free 释放)
char* JADEVIEW_CALL JadeView_get_app_signature(void);
// 获取签名信息 JSON
// # 返回
// - 签名信息 JSON 字符串指针 (调用方需使用 jade_text_free 释放)
char* JADEVIEW_CALL JadeView_get_signature_info(void);
// 清除 JAPK 加载状态
// # 返回
// - 0=成功
int32_t JADEVIEW_CALL JadeView_unload(void);

// --- from lifecycle.rs ---
// DLL初始化
// 备注 · app_signature 长度限制：
// `app_signature` 必填，且 **trim 后至少 `MIN_APP_SIGNATURE_CHARS`（=6）个 Unicode 字符**；
// `app_name` 必填、非纯空白。过短/缺失会被拒绝（例如 `"jvh"` 仅 3 字符 → 拒绝）。
// 返回值：成功返回 `1`；失败返回 `0`（参数校验未过，或启用单实例时本进程为第二实例）。
// 失败时**不启动 GUI 线程**，但仍通过 `app-ready` 事件回报宿主：
// `window_id == 1` 成功、`window_id == 0` 失败（event_data 为错误描述）。
// ⚠️ 宿主在 `jade_on("app-ready", ...)` 里必须判断 `window_id`，不要无条件继续建窗——
// 否则签名过短时会在未初始化状态下误建窗、协议名回退为默认。
int32_t JADEVIEW_CALL JadeView_init(int32_t enable_devmod, const char* log_path, const char* data_directory, const char* app_name, const char* app_signature, int32_t single_instance);
// 运行消息循环
int32_t JADEVIEW_CALL run_message_loop(void);
// 清理所有窗口并结束消息循环
int32_t JADEVIEW_CALL jadeview_exit(void);
// [已废弃] 请使用 jadeview_exit() 代替
int32_t JADEVIEW_CALL cleanup_all_windows(void);

// --- from window.rs ---
// 创建WebView窗口
uint32_t JADEVIEW_CALL create_webview_window(const char* url, uint32_t parent_window_id, const struct WebViewWindowOptions* options, const struct WebViewSettings* webview_settings);
// 独立无边框 WebView 窗口：内部为普通承载窗口 + WebView，返回 `window_id`。
// 仅此类窗口可通过 `get_window_hwnd` 获取原生句柄。
uint32_t JADEVIEW_CALL create_borderless_webview_window(const char* url, const struct WebViewSettings* webview_settings);
// 仅对 `create_borderless_webview_window` 创建的窗口返回 HWND；标准窗口始终返回 0。
size_t JADEVIEW_CALL get_window_hwnd(uint32_t window_id);
// 导航到URL
int32_t JADEVIEW_CALL navigate_to_url(uint32_t window_id, const char* url, const char* headers_json);
// 刷新webview页面
int32_t JADEVIEW_CALL reload_webview_window(uint32_t window_id);
// 执行JavaScript，返回唯一id，通过javascript-result事件返回结果
int32_t JADEVIEW_CALL execute_javascript(uint32_t window_id, const char* script);
// 设置窗口标题
int32_t JADEVIEW_CALL set_window_title(uint32_t window_id, const char* title);
// 设置窗口大小
int32_t JADEVIEW_CALL set_window_size(uint32_t window_id, int32_t width, int32_t height);
// 设置窗口位置
int32_t JADEVIEW_CALL set_window_position(uint32_t window_id, int32_t x, int32_t y);
// 设置窗口可见性
int32_t JADEVIEW_CALL set_window_visible(uint32_t window_id, int32_t visible);
// 设置窗口焦点
int32_t JADEVIEW_CALL set_window_focus(uint32_t window_id);
// 设置窗口是否置顶
int32_t JADEVIEW_CALL set_window_always_on_top(uint32_t window_id, int32_t always_on_top);
// 不进任务栏/Alt-Tab（跨平台：Windows WS_EX_TOOLWINDOW、Linux GTK skip-taskbar-hint）
int32_t JADEVIEW_CALL set_window_skip_taskbar(uint32_t window_id, int32_t skip);
// 不抢焦点：点击/显示不激活窗口（Windows WS_EX_NOACTIVATE、Linux gtk accept-focus=false）
int32_t JADEVIEW_CALL set_window_no_activate(uint32_t window_id, int32_t no_activate);
// 设置窗口层级：topmost | normal | bottom | desktop
// topmost/normal/bottom 跨平台（tao set_always_on_top/bottom）；
// desktop=贴桌面壁纸层（Windows 寄生 WorkerW、Linux GTK desktop type hint）。
int32_t JADEVIEW_CALL set_window_level(uint32_t window_id, const char* level);
// 关闭窗口
int32_t JADEVIEW_CALL close_window(uint32_t window_id);
// 最小化窗口
int32_t JADEVIEW_CALL minimize_window(uint32_t window_id);
// 切换窗口最大化/还原状态
int32_t JADEVIEW_CALL toggle_maximize_window(uint32_t window_id);
// 获取窗口是否最大化
int32_t JADEVIEW_CALL is_window_maximized(uint32_t window_id);
// 设置内容保护
int32_t JADEVIEW_CALL set_content_protection(uint32_t window_id, int32_t content_protection);
// 设置WebView缩放级别
int32_t JADEVIEW_CALL set_webview_zoom(uint32_t window_id, double level);
// 设置窗口边框样式
int32_t JADEVIEW_CALL set_window_frame_style(uint32_t window_id, const char* frame_style);
// 获取窗口数量
uint32_t JADEVIEW_CALL get_window_count(void);
// 注册事件处理器
uint32_t JADEVIEW_CALL jade_on(const char* event_name, IpcCallback callback);
// 注销事件处理器
int32_t JADEVIEW_CALL jade_off(const char* event_name, uint32_t callback_id);
// 订阅前端发送的IPC消息
int32_t JADEVIEW_CALL register_ipc_handler(const char* channel, IpcCallback ipc_cb);
// 设置窗口主题
int32_t JADEVIEW_CALL set_window_theme(uint32_t window_id, const char* theme);
// 设置标题栏覆盖层样式（Windows only）
// 参数:
// window_id: 目标窗口ID
// height: 覆盖层高度（<=0 不修改）
// icon_color_hex: 图标颜色十六进制（如 #FFFFFF）
// hover_bg_hex: 悬浮背景色十六进制（不含关闭按钮）
// 返回: 1 成功，0 失败
int32_t JADEVIEW_CALL set_titlebar_overlay_style(uint32_t window_id, int32_t height, const char* icon_color_hex, const char* hover_bg_hex);
// 启用/禁用窗口
int32_t JADEVIEW_CALL set_window_enabled(uint32_t window_id, int32_t enabled);
// 获取窗口主题
int32_t JADEVIEW_CALL get_window_theme(uint32_t window_id);
// 请求窗口重绘
int32_t JADEVIEW_CALL request_redraw(uint32_t window_id);
// 发送IPC消息
int32_t JADEVIEW_CALL send_ipc_message(uint32_t window_id, const char* message_type, const char* message_content);
// 设置窗口背景材料
int32_t JADEVIEW_CALL set_window_backdrop(uint32_t window_id, const char* backdrop_type);
// 设置窗口背景色（纯色底）：#RRGGBBAA
int32_t JADEVIEW_CALL set_window_background_color(uint32_t window_id, const char* background_color_hex);
// 设置窗口全屏
int32_t JADEVIEW_CALL set_window_fullscreen(uint32_t window_id, int32_t fullscreen);
int32_t JADEVIEW_CALL get_window_bounds(uint32_t window_id, char* buffer, int32_t buffer_size);
int32_t JADEVIEW_CALL is_window_minimized(uint32_t window_id);
int32_t JADEVIEW_CALL is_window_visible(uint32_t window_id);
int32_t JADEVIEW_CALL is_window_focused(uint32_t window_id);
int32_t JADEVIEW_CALL is_window_fullscreen(uint32_t window_id);
int32_t JADEVIEW_CALL set_window_min_size(uint32_t window_id, int32_t width, int32_t height);
int32_t JADEVIEW_CALL set_window_max_size(uint32_t window_id, int32_t width, int32_t height);
int32_t JADEVIEW_CALL set_window_resizable(uint32_t window_id, int32_t resizable);
int32_t JADEVIEW_CALL set_window_ignore_cursor_events(uint32_t window_id, int32_t ignore);
int32_t JADEVIEW_CALL get_webview_url(uint32_t window_id, char* buffer, int32_t buffer_size);
int32_t JADEVIEW_CALL open_devtools(uint32_t window_id);
int32_t JADEVIEW_CALL close_devtools(uint32_t window_id);
int32_t JADEVIEW_CALL is_devtools_open(uint32_t window_id);
int32_t JADEVIEW_CALL clear_browsing_data(uint32_t window_id);
int32_t JADEVIEW_CALL set_window_progress(uint32_t window_id, int32_t progress, int32_t state);
int32_t JADEVIEW_CALL flash_window(uint32_t window_id, uint32_t count);
int32_t JADEVIEW_CALL show_about_dialog(uint32_t window_id);

// --- from tray.rs ---
uint32_t JADEVIEW_CALL tray_create(void);
int32_t JADEVIEW_CALL tray_destroy(uint32_t tray_id);
int32_t JADEVIEW_CALL tray_set_visible(uint32_t tray_id, int32_t visible);
int32_t JADEVIEW_CALL tray_set_tooltip(uint32_t tray_id, const char* tooltip_utf8);
int32_t JADEVIEW_CALL tray_set_icon_from_file(uint32_t tray_id, const char* icon_path_utf8);
// 用扁平表设置托盘根菜单（库内深拷贝字符串）。`item_count==0` 表示清除菜单。成功 `1`，失败 `0`。
int32_t JADEVIEW_CALL tray_set_menu_items(uint32_t tray_id, const struct TrayMenuItemDesc* items, uint32_t item_count);
int32_t JADEVIEW_CALL set_tray_icon_from_data(uint32_t tray_id, const uint8_t* icon_data, uint32_t data_len);

// --- from dialog.rs ---
// 显示通知
int32_t JADEVIEW_CALL show_notification(const struct NotificationParams* params);
// 显示打开文件对话框
// 参数:
// - params: 对话框参数结构体指针
// 返回:
// - 1: 成功
// - 0: 失败
char* JADEVIEW_CALL jade_dialog_show_open_dialog(const struct FileDialogParams* params);
// 显示保存文件对话框
// 参数:
// - params: 对话框参数结构体指针
// 返回:
// - 1: 成功
// - 0: 失败
char* JADEVIEW_CALL jade_dialog_show_save_dialog(const struct FileDialogParams* params);
// 显示消息框
// 参数:
// - params: 消息框参数结构体指针
// 返回:
// - 1: 成功
// - 0: 失败
char* JADEVIEW_CALL jade_dialog_show_message_box(const struct MessageBoxParams* params);
// 显示错误框
// 参数:
// - window_id: 窗口ID
// - title: 错误标题
// - content: 错误内容
// 返回:
// - 1: 成功
// - 0: 失败
int32_t JADEVIEW_CALL jade_dialog_show_error_box(uint32_t window_id, const char* title, const char* content);
int32_t JADEVIEW_CALL jade_dialog_show_open_dialog_async(const struct FileDialogParams* params, void (*callback)(const char*));
int32_t JADEVIEW_CALL jade_dialog_show_save_dialog_async(const struct FileDialogParams* params, void (*callback)(const char*));
int32_t JADEVIEW_CALL jade_dialog_show_message_box_async(const struct MessageBoxParams* params, void (*callback)(const char*));
// 创建菜单项，返回 menu_id（0=失败）
// label: 显示文本, kind: 0=普通命令, 1=分隔线, 2=复选框, 3=单选, 4=子菜单
// parent_menu_id: 0=顶级菜单, >0=添加到指定子菜单
// item_id: 用户自定义ID，点击时通过 menu-item-clicked 事件返回
uint32_t JADEVIEW_CALL jade_menu_item_create(const char* label, int32_t kind, uint32_t parent_menu_id, int32_t item_id);
// 设置菜单项启用/禁用
int32_t JADEVIEW_CALL jade_menu_item_set_enabled(uint32_t menu_id, int32_t enabled);
// 设置复选框/单选的选中状态
int32_t JADEVIEW_CALL jade_menu_item_set_checked(uint32_t menu_id, int32_t checked);
// 设置当前右键菜单要显示的菜单项（在 context-menu 事件回调中调用）
// 传入顶级菜单项 ID 数组，按顺序显示
int32_t JADEVIEW_CALL jade_set_context_menu_items(uint32_t window_id, const uint32_t* menu_ids, int32_t count);
// 销毁菜单项（及其子项）
int32_t JADEVIEW_CALL jade_menu_item_destroy(uint32_t menu_id);

// --- from yaml_store.rs ---
// YAML: set value by key path (a.b.c). `value` 可为 JSON、YAML 片段或任意纯文本（见 parse_yaml_set_payload）。
// 返回值：1=成功，0=路径不存在/空操作，-1=IO错误，-2=类型不匹配，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_set(const char* file_name, const char* key_path, const char* value);
// YAML: get value by key path; returns JSON string in buffer.
// 返回值：1=成功，≥2=两阶段查询所需字节数(含NUL)，0=路径不存在，-1=IO错误，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_get(const char* file_name, const char* key_path, char* buffer, size_t buffer_size);
// YAML: 强制字符串存储（不尝试 JSON/YAML 解析）
// 返回值：1=成功，0=空操作，-1=IO错误，-2=类型不匹配，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_set_str(const char* file_name, const char* key_path, const char* value);
// YAML: 内部 malloc 返回字符串指针，调用方 CoTaskMemFree 释放
// 返回值：非空指针=成功，空指针=失败
char* JADEVIEW_CALL yaml_get_str(const char* file_name, const char* key_path);
// YAML: 读取整个文件，返回 JSON 字符串到 buffer
// 返回值：1=成功，≥2=所需字节数(含NUL)，0=文件不存在，-1=IO错误，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_get_all(const char* file_name, char* buffer, size_t buffer_size);
// YAML: 检查路径是否存在
// 返回值：1=存在，0=不存在，-1=IO错误，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_has(const char* file_name, const char* key_path);
// YAML: 删除指定路径
// 返回值：1=成功，0=路径不存在，-1=IO错误，-2=类型不匹配，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_delete(const char* file_name, const char* key_path);
// YAML: 清空文件为 {}
// 返回值：1=成功，-1=IO错误
int32_t JADEVIEW_CALL yaml_clear(const char* file_name);
// YAML: 删除文件
// 返回值：1=成功，0=文件不存在，-1=IO错误
int32_t JADEVIEW_CALL yaml_delete_file(const char* file_name);
// YAML: 列出路径下的所有 key
// 返回值：1=成功，≥2=所需字节数(含NUL)，0=路径不存在/非映射非序列，-1=IO错误，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_keys(const char* file_name, const char* key_path, char* buffer, size_t buffer_size);
// YAML: 返回数组长度/对象 key 数
// 返回值：≥0=长度，-1=IO错误，-2=类型不匹配(非映射非序列)，-4=格式解析失败
int32_t JADEVIEW_CALL yaml_len(const char* file_name, const char* key_path);

// --- from system.rs ---
// 打开打印对话框（WebView2 内置）
int32_t JADEVIEW_CALL jade_print(uint32_t window_id);
// 使用系统关联程序打印文件（Windows=ShellExecute "print"、Linux=CUPS lp），调用点无 cfg。
int32_t JADEVIEW_CALL jade_print_dialog(const char* file_path);
// 获取系统打印机列表（Windows=EnumPrintersW、Linux=CUPS lpstat -e）。
// 写入 JSON 字符串数组到 buffer，返回打印机数量。
int32_t JADEVIEW_CALL jade_get_printer_list(char* buffer, int32_t buffer_size);
// 智能转码：自动检测输入文本编码，转换为目标编码
// # 参数
// - `input_data`: 输入字节流
// - `input_len`: 输入字节长度
// - `target_encoding`: 目标编码名称，大小写不敏感，支持 WhatWG 标准名称和别名：
// - UTF-8: `"utf-8"`, `"utf8"`, `"unicode-1-1-utf-8"`
// - GBK: `"gbk"`, `"gb2312"`, `"gb18030"`, `"chinese"`, `"csgb2312"`
// - Shift_JIS: `"shift_jis"`, `"sjis"`, `"shift-jis"`, `"ms_kanji"`
// - Big5: `"big5"`, `"big5-hkscs"`, `"cn-big5"`, `"csbig5"`
// - EUC-KR: `"euc-kr"`, `"cseuckr"`, `"korean"`
// - windows-1252: `"windows-1252"`, `"ascii"`, `"latin1"`, `"iso-8859-1"`
// - 完整列表见 https://encoding.spec.whatwg.org/
// - `output_buffer`: 输出缓冲区（NUL 结尾）
// - `buffer_size`: 输出缓冲区大小（字节）
// - `detected_encoding`: [输出] 检测到的来源编码名称（可为 NULL）
// - `detected_encoding_size`: detected_encoding 缓冲区大小
// # 返回值
// - `>0`: 成功，写入输出缓冲区的字节数（不含 NUL）
// - `0`: 失败（参数无效、编码检测失败、目标编码不支持等）
// - `<0`: 缓冲区不足，绝对值为所需缓冲区大小
// # 检测逻辑
// 1. BOM 优先（UTF-8 BOM / UTF-16 LE BOM / UTF-16 BE BOM）
// 2. 无 BOM 时使用 chardetng 智能检测
// 3. 源编码与目标编码相同时直接复制
int32_t JADEVIEW_CALL smart_convert_encoding(const uint8_t* input_data, int32_t input_len, const char* target_encoding, char* output_buffer, int32_t buffer_size, char* detected_encoding, int32_t detected_encoding_size);
// 设置自定义协议服务路径（原 create_local_server）
// hot_reload: 是否启用热载模式 (0=禁用, 1=启用)，仅文件系统模式有效
int32_t JADEVIEW_CALL set_protocol_service_path(const char* root_path, char* url_buffer, size_t buffer_size, int32_t hot_reload);
int32_t JADEVIEW_CALL getPath(const char* name, char* buffer, size_t buffer_size);
// 显示器信息 JSON 数组写入 `buffer`（UTF-8 NUL 结尾）。每项含 bounds、work_area、scale_factor、dpi_x/y、is_primary。
// `buffer` 过小则返回 0。
int32_t JADEVIEW_CALL get_displays_info(char* buffer, size_t buffer_size);
// 获取系统语言（BCP 47，如 zh-CN）
int32_t JADEVIEW_CALL getLocale(char* buffer, size_t buffer_size);
// 清空数据目录（安全确认）
// confirm_token 必须等于 "I_UNDERSTAND_CLEAR_DATA" 才会执行
int32_t JADEVIEW_CALL clear_data_directory(const char* confirm_token);
// JadeView 版本字符串：`Cargo` 语义化版本 + `.` + 构建号（见 `build.rs`）。
int32_t JADEVIEW_CALL jadeview_version(char* buffer, size_t buffer_size);
int32_t JADEVIEW_CALL register_url_scheme(const char* scheme);
int32_t JADEVIEW_CALL unregister_url_scheme(const char* scheme);
int32_t JADEVIEW_CALL register_file_association(const char* extension, const char* friendly_name);
int32_t JADEVIEW_CALL unregister_file_association(const char* extension);
uint32_t JADEVIEW_CALL register_global_hotkey(uint32_t modifiers, uint32_t vk);
int32_t JADEVIEW_CALL unregister_global_hotkey(uint32_t hotkey_id);
// 启用/取消开机自启。
// 参数：enable（0=取消，1=启用）、args（追加到可执行路径后的启动参数，NULL/空=无）。
// 返回：1=成功，0=失败。
int32_t JADEVIEW_CALL set_login_autostart(int32_t enable, const char* args);
// 查询是否已设置开机自启。返回：1=已设置，0=未设置。
int32_t JADEVIEW_CALL get_login_autostart(void);
// 提取任意路径（exe/lnk/普通文件/文件夹）的图标为 PNG，注册为 jade:// 资源，URL 写入 url_buffer。
// 参数：size 目标边长（16/32/48/64/128/256，<=0 取 48）；window_id 资源归属窗口（0=全局）；
// ttl_seconds 资源过期秒（0=默认，u32::MAX=永不过期）。
// 返回：1=成功，0=失败。Windows 用 SHGetFileInfo 取系统图标；Linux 用 gio 查询 + gtk IconTheme。
int32_t JADEVIEW_CALL get_file_icon(const char* path, int32_t size, uint32_t window_id, uint32_t ttl_seconds, char* url_buffer, size_t buffer_size);
// 获取WebView版本号
int32_t JADEVIEW_CALL get_webview_version(char* buffer, size_t buffer_size);
// 检查当前系统是否为Windows 11
int32_t JADEVIEW_CALL is_windows_11(void);
void JADEVIEW_CALL jade_text_free(char* ptr);
char* JADEVIEW_CALL jade_text_create(const char* text);
// 注册本地文件为安全资源，返回 jade:// URL
// path: 本地文件绝对路径
// window_id: 所属窗口ID (0=全局)
// ttl_seconds: 过期时间秒数 (0=永不过期)
// url_buffer: 输出URL字符串的缓冲区
// buffer_size: 缓冲区大小
// 返回: 1=成功, 0=失败
int32_t JADEVIEW_CALL register_resource(const char* path, uint32_t window_id, uint32_t ttl_seconds, char* url_buffer, size_t buffer_size);
// 注销已注册的安全资源
// token_or_url: token字符串或完整jade://---jade---resource--?token=xxx URL
// 返回: 1=成功, 0=未找到
int32_t JADEVIEW_CALL unregister_resource(const char* token_or_url);
// 清理指定窗口的所有已注册资源
// window_id: 窗口ID
// 返回: 清理的资源数量
int32_t JADEVIEW_CALL clear_window_resources(uint32_t window_id);
int32_t JADEVIEW_CALL clipboard_read_text(char* buffer, int32_t buffer_size);
int32_t JADEVIEW_CALL clipboard_write_text(const char* text);
int32_t JADEVIEW_CALL get_cursor_position(char* buffer, int32_t buffer_size);
// 获取当前网络时间戳（毫秒）
// # 参数
// - `ntp_server`: NTP 服务器地址（如 `"ntp.aliyun.com"`）。传 `NULL` 或空字符串时，
// 使用内置的 NTP 服务器列表逐个尝试。
// # 返回值
// - `>= 0`：Unix 时间戳（毫秒）
// - `-1`：获取失败
// # Safety
// `ntp_server` 可为 `NULL`；若非空必须指向以 NUL 结尾的有效 C 字符串。
int64_t JADEVIEW_CALL jade_ntp_now(const char* ntp_server);  /* PATCH: 上游 v2.3.0-beta.6 头 cbindgen 漏映射, 原为 `i64`(Rust型) → C++ 不识别, 改为 int64_t */

#ifdef __cplusplus
}
#endif

#endif  /* JADEVIEW_H */

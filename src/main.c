#include <stdio.h>
#include "cutils.h"
#include "quickjs-libc.h"
#include "quickjs.h"
#include "module_hello.c"
#include "module_quickjs_ffi.h"
#include "module_fs.h"
#include "uv.h"
#include "list.h"
#include "./bundles/quickjs-ffi.c"

typedef struct JSRuntimeAddinfo
{
    struct list_head idle_handlers;
    struct list_head timer_handlers;
    int64_t next_timer_id;
} JSRuntimeAddinfo;

typedef struct JSIdleHandler
{
    struct list_head link;
    uv_idle_t idle_handle;
} JSIdleHandler;

typedef struct
{
    struct list_head link;
    uv_timer_t handle;
    int64_t timer_id;
    JSValue fn;
    JSContext *ctx;
} JSTimerInfo;

static void check_cb(uv_check_t *handle)
{
    printf("check_cb11122------\n");
    JSContext *ctx1;
    int err;
    JSRuntime *rt = handle->data;
    while (1)
    {
        err = JS_ExecutePendingJob(rt, &ctx1);
        if (err <= 0)
        {
            // if err < 0, an exception occurred
            break;
        }
    }

    JSRuntimeAddinfo *customData = JS_GetRuntimeOpaque(rt);
    struct list_head *el;
    JSIdleHandler *idle_handler = NULL;
    list_for_each(el, &customData->idle_handlers)
    {
        idle_handler = list_entry(el, JSIdleHandler, link);
    }
    if (idle_handler != NULL)
    {
        uv_idle_stop(&idle_handler->idle_handle);
        list_del(&idle_handler->link);
        js_free_rt(rt, idle_handler);
    }
}

static void js_free_libuv_timer(JSRuntime *rt, JSTimerInfo *info)
{
    JS_FreeValue(info->ctx, info->fn);
    list_del(&info->link);
    js_free_rt(rt, info);
}

static void timer_cb(uv_timer_t *timer)
{
    JSTimerInfo *info = timer->data;
    // 执行 js 函数
    JSValue ret = JS_Call(info->ctx, info->fn, JS_UNDEFINED, 0, NULL);
    printf("timer_cb------%s\n", JS_ToCString(info->ctx, ret));
    JS_FreeValue(info->ctx, ret);
    js_free_libuv_timer(JS_GetRuntime(info->ctx), info);
}

static void idle_cb(uv_idle_t *handle)
{
    printf("idle_cb------\n");
}

static JSValue js_setTimeout(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv)
{
    printf("js_setTimeout------\n");
    //
    if (argc < 2)
        return JS_EXCEPTION;
    // setTimeout(fn, delay)
    // 第一个参数是一个js 函数
    JSValue fn = argv[0];
    if (!JS_IsFunction(ctx, fn))
        return JS_ThrowTypeError(ctx, "not a function");
    // 第二个参数是一个数字
    int delay = JS_VALUE_GET_INT(argv[1]);

    JSTimerInfo *info = js_mallocz(ctx, sizeof(*info));
    info->ctx = ctx;
    // 这里需要 dup 一下，参数会被quickjs 自动释放
    info->fn = JS_DupValue(ctx, argv[0]);

    // 初始化一个 uv_timer_t
    uv_timer_init(uv_default_loop(), &info->handle);
    // 因为我们需要在 timer_cb 中使用这个参数，所以我们把这个参数放到 handle 的 data 中
    info->handle.data = info;
    uv_timer_start(&info->handle, timer_cb, delay, 0);
    // 把这个定时器放到一个链表中
    JSRuntime *rt = JS_GetRuntime(ctx);
    JSRuntimeAddinfo *customData = JS_GetRuntimeOpaque(rt);
    info->timer_id = customData->next_timer_id++;
    list_add_tail(&info->link, &customData->timer_handlers);
    return JS_NewInt32(ctx, info->timer_id);
}

static JSValue js_clearTimeout(JSContext *ctx, JSValueConst this_val,
                               int argc, JSValueConst *argv)
{
    if (argc < 1)
        return JS_EXCEPTION;
    // 第一个参数是一个数字
    int timer_id = JS_VALUE_GET_INT(argv[0]);
    // 遍历链表，找到这个定时器
    JSRuntime *rt = JS_GetRuntime(ctx);
    JSRuntimeAddinfo *customData = JS_GetRuntimeOpaque(rt);
    struct list_head *el;
    JSTimerInfo *info = NULL;
    list_for_each(el, &customData->timer_handlers)
    {
        info = list_entry(el, JSTimerInfo, link);
        if (info->timer_id == timer_id)
        {
            break;
        }
    }
    if (info != NULL)
    {
        uv_timer_stop(&info->handle);
        js_free_libuv_timer(rt, info);
    }
    return JS_UNDEFINED;
}

static void js_add_custom_helpers(JSContext *ctx)
{
    JSValue global_obj;
    // 获取全局对象
    global_obj = JS_GetGlobalObject(ctx);
    // 我们定义一个 JS function in C。这样我们就可以在JS中调用这个函数。
    JSValue setTimeout = JS_NewCFunction(ctx, js_setTimeout, "setTimeout", 1);
    // 把这个函数添加到全局对象中
    JS_SetPropertyStr(ctx, global_obj, "setTimeout", setTimeout);

    JSValue clearTimeout = JS_NewCFunction(ctx, js_clearTimeout, "clearTimeout", 1);
    // 把这个函数添加到全局对象中
    JS_SetPropertyStr(ctx, global_obj, "clearTimeout", clearTimeout);

    JS_FreeValue(ctx, global_obj);
}

typedef struct
{
    const char *name;
    const uint8_t *data;
    uint32_t data_size;
} builtin_js_t;

// 定义一个内置模块的数组，都是用 quickjs 编译的 js 文件
static builtin_js_t builtins[] = {
    {"toyjsruntime:jsffi", qjsc_quickjs_ffi, qjsc_quickjs_ffi_size}};

JSModuleDef *js_custom_module_loader(JSContext *ctx,
                                     const char *module_name, void *opaque)
{
    builtin_js_t *item = NULL;
    for (int i = 0; i < sizeof(builtins) / sizeof(builtin_js_t); i++)
    {
        if (strcmp(module_name, builtins[i].name) == 0)
        {
            item = &builtins[i];
            break;
        }
    }
    if (item != NULL)
    {
        // 加载 btyecode
        JSValue obj = JS_ReadObject(ctx, item->data, item->data_size, JS_READ_OBJ_BYTECODE);
        if (JS_IsException(obj))
        {
            JS_ThrowReferenceError(ctx, "Error loading module %s\n", module_name);
            JS_FreeValue(ctx, obj);
            return NULL;
        }

        if (JS_VALUE_GET_TAG(obj) != JS_TAG_MODULE)
        {
            JS_ThrowReferenceError(ctx, "loaded %s, butis not a modules\n", module_name);
            JS_FreeValue(ctx, obj);
            return NULL;
        }
        // 得到模块
        JSModuleDef *m = JS_VALUE_GET_PTR(obj);
        JS_FreeValue(ctx, obj);
        return m;
    }

    return js_module_loader(ctx, module_name, opaque);
}

int main(int argc, char **argv)
{
    // 初始化 libuv 的 check 和 idle handle
    static uv_check_t check_handle;

    int r = 0;
    JSRuntime *rt;
    JSContext *ctx;
    JSValue ret = JS_UNDEFINED;
    size_t script_len;
    char *filename = argv[1];
#pragma region runtime
    rt = JS_NewRuntime();
    JSRuntimeAddinfo *customData = js_mallocz_rt(rt, sizeof(JSRuntimeAddinfo));
    init_list_head(&customData->idle_handlers);
    init_list_head(&customData->timer_handlers);
    JS_SetRuntimeOpaque(rt, customData);
    ctx = JS_NewContext(rt);
#pragma endregion

    // Initialize standard handlers
    // js_std_init_handlers(rt);

    // // Initialize standard handlers, settimeout etc
    JS_SetModuleLoaderFunc(rt, NULL, js_custom_module_loader, NULL);
    // add console
    js_std_add_helpers(ctx, argc, argv);
    // add custom helpers
    js_add_custom_helpers(ctx);

    // custom module
    js_init_module_test(ctx, "toyjsruntime:test");
    js_init_module_ffi(ctx, "toyjsruntime:ffi");
    js_init_module_fs(ctx, "toyjsruntime:fs");

    // libuv
    uv_check_init(uv_default_loop(), &check_handle);
    check_handle.data = rt;
    uv_check_start(&check_handle, check_cb);
    uv_unref((uv_handle_t *)&check_handle);

    JSIdleHandler *idle_handler = js_mallocz_rt(rt, sizeof(JSIdleHandler));
    uv_idle_init(uv_default_loop(), &idle_handler->idle_handle);
    uv_idle_start(&idle_handler->idle_handle, idle_cb);
    list_add_tail(&idle_handler->link, &customData->idle_handlers);

    uint8_t *script_str;
    // 利用 C 语言的文件读取函数读取文件内容
    script_str = js_load_file(ctx, &script_len, filename);
    if (!script_str)
    {
        printf("failed to load file %s\n", filename);
        r = -1;
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return r;
    }
    // 运行读取的 js 脚本
    ret = JS_Eval(ctx, script_str, script_len, "main", JS_EVAL_TYPE_MODULE);
    if (JS_IsException(ret))
    {
        printf("JS exception occurred\n");
        js_std_dump_error(ctx);
        r = -1;
    } 

    // // Run the event loop until all jobs are done
    // for(;;) {
    //     JSContext *pctx;
    //     int err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &pctx);
    //     if (err <= 0) {
    //         if (err < 0)
    //             js_std_dump_error(pctx);
    //         break;
    //     }
    // }

    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    // Run event loop to process any pending jobs (promises, etc)
    // js_std_loop(ctx);
    js_free(ctx, script_str);
    JS_FreeValue(ctx, ret);
    // free addinfo
    js_free_rt(rt, customData);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return r;
}

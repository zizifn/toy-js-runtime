#include <stdio.h>
#include "cutils.h"
#include "quickjs-libc.h"
#include "quickjs.h"
#include "module_hello.c"
#include "uv.h"
#include "list.h"

typedef struct JSRuntimeAddinfo
{
    struct list_head idle_handlers;
} JSRuntimeAddinfo;

typedef struct JSIdleHandler
{
    struct list_head link;
    uv_idle_t idle_handle;
} JSIdleHandler;

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
    JSIdleHandler *idle_handler;
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

static void idle_cb(uv_idle_t *handle)
{
    printf("idle_cb------\n");
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
    JS_SetRuntimeOpaque(rt, customData);
#pragma endregion

    ctx = JS_NewContext(rt);

    // // Initialize standard handlers, settimeout etc
    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    // add console
    js_std_add_helpers(ctx, argc, argv);

    // custom module
    js_init_module_test(ctx, "toyjsruntime:test");

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
    // 运行读取的 js 脚本
    ret = JS_Eval(ctx, script_str, script_len, "main", JS_EVAL_TYPE_MODULE);
    ret = js_std_await(ctx, ret);
    if (JS_IsException(ret))
    {
        printf("JS exception occurred\n");
        js_std_dump_error(ctx);
        r = -1;
    }

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

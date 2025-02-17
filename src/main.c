#include <stdio.h>
#include "cutils.h"
#include "quickjs-libc.h"
#include "quickjs.h"
#include "module_hello.c"

int main(int argc, char** argv)
{
    int r = 0;
    JSRuntime* rt;
    JSContext* ctx;
    JSValue ret = JS_UNDEFINED;
    char* filename = argv[1];

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);

    JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
    // add console
    js_std_add_helpers(ctx, argc, argv);

    //custom module
    js_init_module_test(ctx, "toyjsruntime:test");

    uint8_t *script_str;
    size_t script_len;
    // 利用 C 语言的文件读取函数读取文件内容
    script_str = js_load_file(ctx, &script_len, filename);
    // 运行读取的 js 脚本
    ret = JS_Eval(ctx, script_str, script_len, filename, JS_EVAL_TYPE_MODULE);
    if (JS_IsException(ret))
    {
        printf("JS exception occured\n");
        js_std_dump_error(ctx);
        r = -1;
    }
    JS_FreeValue(ctx, ret);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return r;
}

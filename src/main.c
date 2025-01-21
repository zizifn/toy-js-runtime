#include <stdio.h>
#include "cutils.h"
#include "quickjs-libc.h"
#include "quickjs.h"


int main(int argc, char** argv)
{
    int r = 0;
    JSRuntime* rt;
    JSContext* ctx;
    JSValue ret = JS_UNDEFINED;

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);
    // add console
    js_std_add_helpers(ctx, argc, argv);

    char* script = "console.log(`hello world!! 1 + 2 = ${1+2}`);";
    // 执行一个 js 脚本，这里使用string， 后续可以读取文件
    ret = JS_Eval(ctx, script, strlen(script), "main", 0);
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

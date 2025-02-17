#include <cutils.h>
#include <quickjs-libc.h>
#include <quickjs.h>
//
// Created by zizifn on 1/21/25.
//
static int eval_buf(JSContext *ctx, const void *buf, int buf_len,
                    const char *filename, int eval_flags)
{
    JSValue val;
    int ret;

    if ((eval_flags & JS_EVAL_TYPE_MASK) == JS_EVAL_TYPE_MODULE) {
        /* for the modules, we compile then run to be able to set
           import.meta */
        val = JS_Eval(ctx, buf, buf_len, filename,
                      eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
        if (!JS_IsException(val)) {
            js_module_set_import_meta(ctx, val, TRUE, TRUE);
            val = JS_EvalFunction(ctx, val);
        }
        val = js_std_await(ctx, val);
    } else {
        val = JS_Eval(ctx, buf, buf_len, filename, eval_flags);
    }
    if (JS_IsException(val)) {
        js_std_dump_error(ctx);
        ret = -1;
    } else {
        ret = 0;
    }
    JS_FreeValue(ctx, val);
    return ret;
}

static int eval_file(JSContext *ctx, const char *filename,  const char *modulename, int module)
{
    uint8_t *buf;
    int ret, eval_flags;
    size_t buf_len;

    buf = js_load_file(ctx, &buf_len, filename);
    if (!buf) {
        perror(filename);
        exit(1);
    }

    if (module < 0) {
        module = (js__has_suffix(filename, ".mjs") ||
                  JS_DetectModule((const char *)buf, buf_len));
    }
    if (module)
        eval_flags = JS_EVAL_TYPE_MODULE;
    else
        eval_flags = JS_EVAL_TYPE_GLOBAL;
    ret = eval_buf(ctx, buf, buf_len, modulename, eval_flags);
    js_free(ctx, buf);
    return ret;
}

 int main(int argc, char **argv)
 {
    printf("begin\n");
     int r =0;
     JSRuntime *rt;
     JSContext *ctx;
     JSValue ret = JS_UNDEFINED;

    char* filename = argv[1];
     rt = JS_NewRuntime();
     ctx = JS_NewContext(rt);

     // add console
     js_std_add_helpers(ctx, argc, argv);

    if (eval_file(ctx, "./src/fib_module.js", "toyruntime:test", 1))
    {
        printf("eval_file failed\n");
        goto fail;
    }

    if (eval_file(ctx, filename, filename, 1))
    {
        printf("eval_file failed\n");
        goto fail;
    }


     if (JS_IsException(ret))
     {
         js_std_dump_error(ctx);
         r = -1;
     }
     ret = js_std_loop(ctx);
     JS_FreeValue(ctx, ret);
     JS_FreeContext(ctx);
     JS_FreeRuntime(rt);
     return r;

    fail:
    js_std_dump_error(ctx);
    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 1;
 }
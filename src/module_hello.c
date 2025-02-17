#include "quickjs.h"
#include <stdlib.h>
#include "quickjs-libc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

static JSValue js_memory(JSContext *ctx, JSValueConst this_val,
                       int argc, JSValueConst *argv)
{
    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("Error opening meminfo file");
        return JS_EXCEPTION;
    }

    char line[256];
    long totalMemoryValue = -1; // Initialize with an error value

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "MemTotal", 8) == 0) {
            char *ptr = line + 9; // Move pointer past "MemTotal:"
            while (*ptr == ' ') ptr++; // Skip spaces
            char *endptr;
            totalMemoryValue = strtol(ptr, &endptr, 10);

            if (ptr == endptr) {
                fprintf(stderr, "Error: No digits were found in MemTotal value\n");
                totalMemoryValue = -1;
            } else if (totalMemoryValue == LONG_MIN || totalMemoryValue == LONG_MAX) {
                fprintf(stderr, "Error: MemTotal value is out of range\n");
                totalMemoryValue = -1;
            }
            break; // Stop after reading MemTotal
        }
    }
    fclose(file);

    if (totalMemoryValue == -1) {
        return JS_ThrowInternalError(ctx, "Failed to read MemTotal from /proc/meminfo");
    }

    return JS_NewInt64(ctx, totalMemoryValue);
}

static const JSCFunctionListEntry js_test_funcs[] = {
    JS_CFUNC_DEF("memory", 1, js_memory ),
};


static int js_test_init(JSContext *ctx, JSModuleDef *m){
    return JS_SetModuleExportList(ctx, m, js_test_funcs,
            countof(js_test_funcs));
}

JSModuleDef *js_init_module_test(JSContext *ctx, const char *module_name)
{
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_test_init);
    if (!m)
        return NULL;
    JS_AddModuleExportList(ctx, m, js_test_funcs, countof(js_test_funcs));
    return m;
}

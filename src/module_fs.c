#include "quickjs.h"
#include "quickjs-libc.h"
#include <uv.h>
#include <stdlib.h>
#include <string.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))

typedef struct {
    JSContext *ctx;
    JSValue promise;
    JSValue resolving_funcs[2];
    char *path;
    char *content;
    size_t content_size;
} fs_read_req_t;

static void fs_read_cb(uv_fs_t *req) {
    fs_read_req_t *fs_req = (fs_read_req_t *)req->data;
    JSContext *ctx = fs_req->ctx;
    JSValue ret;

    if (req->result < 0) {
        // Error handling
        ret = JS_NewError(ctx);
        JS_SetPropertyStr(ctx, ret, "message", 
            JS_NewString(ctx, uv_strerror(req->result)));
        JS_Call(ctx, fs_req->resolving_funcs[1], JS_UNDEFINED, 1, &ret);
    } else if (req->result == 0) {
        // End of file reached, resolve with the complete content
        ret = JS_NewStringLen(ctx, fs_req->content, fs_req->content_size);
        JS_Call(ctx, fs_req->resolving_funcs[0], JS_UNDEFINED, 1, &ret);
    } else {
        // Data read successfully
        ret = JS_NewStringLen(ctx, fs_req->content, req->result);
        JS_Call(ctx, fs_req->resolving_funcs[0], JS_UNDEFINED, 1, &ret);
    }

    // Cleanup
    JS_FreeValue(ctx, fs_req->promise);
    JS_FreeValue(ctx, fs_req->resolving_funcs[0]);
    JS_FreeValue(ctx, fs_req->resolving_funcs[1]);
    free(fs_req->content);
    free(fs_req->path);
    free(fs_req);
    uv_fs_req_cleanup(req);
    free(req);
}

static void fs_open_cb(uv_fs_t *req) {
    fs_read_req_t *fs_req = (fs_read_req_t *)req->data;
    if (req->result < 0) {
        // Handle error
        JSValue error = JS_NewError(fs_req->ctx);
        JS_SetPropertyStr(fs_req->ctx, error, "message", 
            JS_NewString(fs_req->ctx, uv_strerror(req->result)));
        JS_Call(fs_req->ctx, fs_req->resolving_funcs[1], JS_UNDEFINED, 1, &error);
        
        // Cleanup
        JS_FreeValue(fs_req->ctx, fs_req->promise);
        JS_FreeValue(fs_req->ctx, fs_req->resolving_funcs[0]);
        JS_FreeValue(fs_req->ctx, fs_req->resolving_funcs[1]);
        free(fs_req->path);
        free(fs_req);
        uv_fs_req_cleanup(req);
        free(req);
        return;
    }

    // File is open, now read it
    uv_fs_t *read_req = malloc(sizeof(uv_fs_t));
    read_req->data = fs_req;

    // Get file size
    uv_fs_t stat_req;
    if (uv_fs_fstat(uv_default_loop(), &stat_req, req->result, NULL) < 0) {
        JSValue error = JS_NewError(fs_req->ctx);
        JS_SetPropertyStr(fs_req->ctx, error, "message", 
            JS_NewString(fs_req->ctx, "Failed to get file stats"));
        JS_Call(fs_req->ctx, fs_req->resolving_funcs[1], JS_UNDEFINED, 1, &error);
        
        // Cleanup
        uv_fs_req_cleanup(&stat_req);
        JS_FreeValue(fs_req->ctx, fs_req->promise);
        JS_FreeValue(fs_req->ctx, fs_req->resolving_funcs[0]);
        JS_FreeValue(fs_req->ctx, fs_req->resolving_funcs[1]);
        free(fs_req->path);
        free(fs_req);
        uv_fs_req_cleanup(req);
        free(req);
        return;
    }

    size_t file_size = stat_req.statbuf.st_size;
    fs_req->content_size = file_size;
    fs_req->content = malloc(file_size);
    
    uv_buf_t buf = uv_buf_init(fs_req->content, file_size);
    int read_result = uv_fs_read(uv_default_loop(), read_req, req->result, &buf, 1, 0, fs_read_cb);
    
    if (read_result < 0) {
        // Handle read error
        JSValue error = JS_NewError(fs_req->ctx);
        JS_SetPropertyStr(fs_req->ctx, error, "message", 
            JS_NewString(fs_req->ctx, uv_strerror(read_result)));
        JS_Call(fs_req->ctx, fs_req->resolving_funcs[1], JS_UNDEFINED, 1, &error);
        
        // Cleanup
        free(fs_req->content);
        free(fs_req->path);
        free(fs_req);
        uv_fs_req_cleanup(read_req);
        free(read_req);
    }
    
    uv_fs_req_cleanup(&stat_req);
    uv_fs_req_cleanup(req);
    free(req);
}

static JSValue js_readFile(JSContext *ctx, JSValueConst this_val,
                          int argc, JSValueConst *argv) {
    const char *path;
    JSValue promise, resolving_funcs[2];

    if (argc < 1)
        return JS_EXCEPTION;

    path = JS_ToCString(ctx, argv[0]);
    if (!path)
        return JS_EXCEPTION;

    // Create promise and get resolve/reject functions
    promise = JS_NewPromiseCapability(ctx, resolving_funcs);
    if (JS_IsException(promise)) {
        JS_FreeCString(ctx, path);
        return JS_EXCEPTION;
    }

    // Prepare request structure
    fs_read_req_t *fs_req = malloc(sizeof(fs_read_req_t));
    if (!fs_req) {
        JSValue error = JS_NewError(ctx);
        JS_SetPropertyStr(ctx, error, "message", 
            JS_NewString(ctx, "Failed to allocate memory"));
        JS_Call(ctx, resolving_funcs[1], JS_UNDEFINED, 1, &error);
        JS_FreeCString(ctx, path);
        JS_FreeValue(ctx, promise);
        JS_FreeValue(ctx, resolving_funcs[0]);
        JS_FreeValue(ctx, resolving_funcs[1]);
        return promise;
    }

    fs_req->ctx = ctx;
    fs_req->promise = JS_DupValue(ctx, promise);
    fs_req->resolving_funcs[0] = resolving_funcs[0];
    fs_req->resolving_funcs[1] = resolving_funcs[1];
    fs_req->path = strdup(path);
    fs_req->content = NULL;
    fs_req->content_size = 0;
    
    // Start async file open
    uv_fs_t *open_req = malloc(sizeof(uv_fs_t));
    if (!open_req) {
        JSValue error = JS_NewError(ctx);
        JS_SetPropertyStr(ctx, error, "message", 
            JS_NewString(ctx, "Failed to allocate memory"));
        JS_Call(ctx, resolving_funcs[1], JS_UNDEFINED, 1, &error);
        free(fs_req->path);
        free(fs_req);
        JS_FreeCString(ctx, path);
        JS_FreeValue(ctx, promise);
        JS_FreeValue(ctx, resolving_funcs[0]);
        JS_FreeValue(ctx, resolving_funcs[1]);
        return promise;
    }

    open_req->data = fs_req;
    
    int ret = uv_fs_open(uv_default_loop(), open_req, path, O_RDONLY, 0, fs_open_cb);
    if (ret < 0) {
        JSValue error = JS_NewError(ctx);
        JS_SetPropertyStr(ctx, error, "message", 
            JS_NewString(ctx, uv_strerror(ret)));
        JS_Call(ctx, resolving_funcs[1], JS_UNDEFINED, 1, &error);
        
        free(fs_req->path);
        free(fs_req);
        free(open_req);
    }

    JS_FreeCString(ctx, path);
    return promise;
}

static const JSCFunctionListEntry js_fs_funcs[] = {
    JS_CFUNC_DEF("readFile", 2, js_readFile),
};

static int js_fs_init(JSContext *ctx, JSModuleDef *m) {
    return JS_SetModuleExportList(ctx, m, js_fs_funcs,
                                 countof(js_fs_funcs));
}

JSModuleDef *js_init_module_fs(JSContext *ctx, const char *module_name) {
    JSModuleDef *m;
    m = JS_NewCModule(ctx, module_name, js_fs_init);
    if (!m)
        return NULL;
    JS_AddModuleExportList(ctx, m, js_fs_funcs, countof(js_fs_funcs));
    return m;
}
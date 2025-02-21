#ifndef JS_FFI_MODULE_H
#define JS_FFI_MODULE_H

#include "quickjs.h"

JSModuleDef *js_init_module_ffi(JSContext *ctx, const char *module_name);

#endif /* JS_FFI_MODULE_H */

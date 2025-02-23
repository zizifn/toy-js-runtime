#ifndef JS_FS_MODULE_H
#define JS_FS_MODULE_H

#include "quickjs.h"

JSModuleDef *js_init_module_fs(JSContext *ctx, const char *module_name);

#endif /* JS_FS_MODULE_H */

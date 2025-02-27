import { CFunction, CCallback } from 'toyjsruntime:jsffi';
import { readFile } from 'toyjsruntime:fs';
import * as ffi from 'toyjsruntime:ffi';

// int main(void) {
//     #endif
//       webview_t w = webview_create(0, NULL);
//       webview_set_title(w, "Basic Example");
//       webview_set_size(w, 480, 320, WEBVIEW_HINT_NONE);
//       webview_set_html(w, "Thanks for using webview!");
//       webview_run(w);
//       webview_destroy(w);
//       return 0;
//     }

const webviewLib = './libwebviewd.so.0.12.0';

//WEBVIEW_API webview_t webview_create(int debug, void *window);
const webview_create = new CFunction(webviewLib, 'webview_create', null, 'pointer', 'int', 'pointer').invoke;
console.log("----webview_create");

//webview_error_t webview_set_title(webview_t w, const char *title);
const webview_set_title = new CFunction(webviewLib, 'webview_set_title', null, 'int', 'pointer', 'string').invoke;

//webview_error_t webview_set_size(webview_t w, int width, int height,webview_hint_t hints);

const webview_set_size = new CFunction(webviewLib, 'webview_set_size', null, 'int', 'pointer', 'int', 'int', 'int').invoke;

//webview_error_t webview_set_html(webview_t w, const char *html);

const webview_set_html = new CFunction(webviewLib, 'webview_set_html', null, 'int', 'pointer', 'string').invoke;

//WEBVIEW_API webview_error_t webview_navigate(webview_t w, const char *url);
const webview_navigate = new CFunction(webviewLib, 'webview_navigate', null, 'int', 'pointer', 'string').invoke;

// WEBVIEW_API webview_error_t webview_eval(webview_t w, const char *js);
const webview_eval = new CFunction(webviewLib, 'webview_eval', null, 'int', 'pointer', 'string').invoke;

// WEBVIEW_API webview_error_t webview_init(webview_t w, const char *js);
const webview_init = new CFunction(webviewLib, 'webview_init', null, 'int', 'pointer', 'string').invoke;
// webview_error_t webview_run(webview_t w);
const webview_run = new CFunction(webviewLib, 'webview_run', null, 'int', 'pointer').invoke;
const webview_destroy = new CFunction(webviewLib, 'webview_destroy', null, 'int', 'pointer').invoke;

// console.log("----start---");
// const w = webview_create(1, ffi.NULL); // Changed debug parameter to 1
// console.log("webview_create", w);
// webview_set_title(w, "Basic Example1111");
// console.log('webview_set_title');
// webview_set_size(w, 480, 320, 0);
// console.log('webview_set_size');
// webview_set_html(w, `Thanks for using webview!`);
// console.log('webview_set_html');

// // Add a small delay before running
// webview_run(w);
// console.log('webview_run');
// webview_destroy(w);
// console.log('webview_destroy');


class BrowserWindow {
    constructor({
        width = 480,
        height = 320,
    } = {}) {
        this.w = webview_create(1, ffi.NULL);
        webview_set_size(this.w, width, height, 0);
        webview_set_title(this.w, "Basic Example");
    }

    async loadFile(file) {
        const html = await readFile(file);
        webview_set_html(this.w, html);
        console.log("after webview_set_html")
        webview_run(this.w); // 应该跑在 thread 里面，不 block 主线程
        webview_destroy(this.w);
    }
}

const window = new BrowserWindow(
);
await window.loadFile('index.html');




#include "js_engine.hh"

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs.h"
#include "quickjs-libc.h"
#ifdef __cplusplus
}
#endif

#include "html_common.hh"
#include "msg.h"

/* console.log binding */
static JSValue js_console_log(JSContext *ctx, JSValueConst this_val,
                              int argc, JSValueConst *argv)
{
   int i;

   for (i = 0; i < argc; i++) {
      size_t len;
      const char *str = JS_ToCStringLen(ctx, &len, argv[i]);

      if (i > 0)
         fputc(' ', stdout);
      if (str) {
         fwrite(str, 1, len, stdout);
         JS_FreeCString(ctx, str);
      } else {
         fputs("[unknown]", stdout);
      }
   }
   fputc('\n', stdout);
   fflush(stdout);

   return JS_UNDEFINED;
}

/* document.write binding */
static JSValue js_document_write(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv)
{
   DilloJS *js = (DilloJS *)JS_GetContextOpaque(ctx);
   if (!js || argc < 1)
      return JS_UNDEFINED;

   size_t len;
   const char *str = JS_ToCStringLen(ctx, &len, argv[0]);
   if (str) {
      js->appendWrite(str, len);
      JS_FreeCString(ctx, str);
   }
   return JS_UNDEFINED;
}

DilloJS::DilloJS(DilloHtml *p_html)
{
   js_runtime = NULL;
   js_context = NULL;
   html = p_html;
   writeBuf = dStr_new("");
   initialized = false;
}

DilloJS::~DilloJS()
{
   cleanup();
   dStr_free(writeBuf, TRUE);
}

void DilloJS::init()
{
   if (initialized)
      return;

   js_runtime = JS_NewRuntime();
   if (!js_runtime) {
      MSG("DilloJS: failed to create JS runtime\n");
      return;
   }
   js_context = JS_NewContext(js_runtime);
   if (!js_context) {
      MSG("DilloJS: failed to create JS context\n");
      JS_FreeRuntime(js_runtime);
      js_runtime = NULL;
      return;
   }

   JS_SetContextOpaque(js_context, this);
   registerBindings();
   initialized = true;
}

void DilloJS::cleanup()
{
   if (js_context) {
      JS_FreeContext(js_context);
      js_context = NULL;
   }
   if (js_runtime) {
      JS_FreeRuntime(js_runtime);
      js_runtime = NULL;
   }
   initialized = false;
}

void DilloJS::registerBindings()
{
   if (!js_context)
      return;

   JSValue global_obj = JS_GetGlobalObject(js_context);

   JSValue doc_obj = JS_NewObject(js_context);
   JS_SetPropertyStr(js_context, doc_obj, "write",
                     JS_NewCFunction(js_context, js_document_write, "write", 1));
   JS_SetPropertyStr(js_context, global_obj, "document", doc_obj);

   JSValue console_obj = JS_NewObject(js_context);
   JS_SetPropertyStr(js_context, console_obj, "log",
                     JS_NewCFunction(js_context, js_console_log, "log", 1));
   JS_SetPropertyStr(js_context, global_obj, "console", console_obj);

   JS_FreeValue(js_context, global_obj);
}

void DilloJS::reportException()
{
   if (!js_context)
      return;
   JSValue exception = JS_GetException(js_context);
   const char *str = JS_ToCString(js_context, exception);
   if (str) {
      BUG_MSG("JS Error: %s\n", str);
      JS_FreeCString(js_context, str);
   }
   JS_FreeValue(js_context, exception);
}

void DilloJS::eval(const char *code, size_t len, const char *filename)
{
   if (!initialized || !js_context || !code)
      return;

   JSValue ret = JS_Eval(js_context, code, len, filename, JS_EVAL_TYPE_GLOBAL);
   if (JS_IsException(ret)) {
      reportException();
   }
   JS_FreeValue(js_context, ret);
}

void DilloJS::appendWrite(const char *str, size_t len)
{
   if (str && len > 0 && writeBuf) {
      dStr_append_l(writeBuf, str, len);
   }
}

void DilloJS::flushWrites()
{
   if (writeBuf && writeBuf->len > 0 && html) {
      html->injectHtml(writeBuf->str, writeBuf->len);
      dStr_truncate(writeBuf, 0);
   }
}

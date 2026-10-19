#ifndef __JS_ENGINE_HH__
#define __JS_ENGINE_HH__

#include "dlib/dlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;

#ifdef __cplusplus
}
#endif

class DilloHtml;

class DilloJS {
private:
   JSRuntime *js_runtime;
   JSContext *js_context;
   DilloHtml *html;
   Dstr *writeBuf;
   bool initialized;
   void registerBindings();
   void reportException();
public:
   DilloJS(DilloHtml *html);
   ~DilloJS();
   void init();
   void cleanup();
   void eval(const char *code, size_t len, const char *filename = "<script>");
   void appendWrite(const char *str, size_t len);
   void flushWrites();
};
#endif

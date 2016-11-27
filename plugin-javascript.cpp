#undef _

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef _JAVASCRIPT_PLUGIN

/*******************************************************************************
 *
 * JavaScript plugin
 *
 * References:
 * - https://v8docs.nodesource.com/io.js-3.0/d2/dc3/namespacev8.html
 * - https://github.com/weechat/weechat/blob/master/src/plugins/javascript/weechat-js-v8.cpp
 * - /usr/include/v8.h
 * - http://jcla1.com/blog/exploring-the-v8-js-engine-part-1
 * - http://jcla1.com/blog/exploring-the-v8-js-engine-part-2
 */

#include <dlfcn.h>
#include <string.h>
#include <cstdio>

#ifdef __LINUX__
#include <alloca.h>
#endif

extern "C"
{
#include "core.h"
#include "utils.h"
#include "main.h"
#include "plugin.h"
}

#include <v8.h>

#define xlog_js(t, ...) xlog(t, "["_JAVASCRIPT_VERSION_"] " __VA_ARGS__)


using namespace v8;


/**
 *
 */
static void proxenet_javascript_print_exception(v8::TryCatch *trycatch)
{
    Local<Value> exception = trycatch->Exception();
    String::Utf8Value str_exception(exception);
    xlog_js("Exception raised: %s\n", *str_exception);
}


/**
 *
 */
int proxenet_javascript_initialize_vm(plugin_t* plugin)
{
	proxenet_js_t* vm;

        vm = (proxenet_js_t*)proxenet_xmalloc(sizeof(proxenet_js_t))
        vm->global = ObjectTemplate::New();
        vm->context = Context::New(NULL, vm->global);

	plugin->interpreter->vm = (void*)vm;
        plugin->interpreter->ready = true;
	return 0;
}


/**
 *
 */
int proxenet_javascript_destroy_plugin(plugin_t* plugin)
{
        proxenet_plugin_set_state(plugin, INACTIVE);
        plugin->onload_function = NULL;
        plugin->pre_function = NULL;
        plugin->post_function = NULL;
        plugin->onleave_function = NULL;
        return 0;
}


/**
 *
 */
int proxenet_javascript_destroy_vm(interpreter_t* interpreter)
{
        proxenet_js_t* vm = interpreter->vm;
        vm->context.Dispose();
        proxenet_xfree(vm);

        interpreter->vm = NULL;
        interpreter->ready = false;
        interpreter = NULL;
        return 0;
}


/**
 *
 */
static bool is_valid_function(interpreter_t* interpreter, const char *function)
{
        proxenet_js_t* vm = interpreter->vm;
        Context::Scope context_scope(vm->context);
        Handle<Object> global = vm->context->Global();
        Handle<Value> value = global->Get(String::New(function));
        return value->IsFunction();
}


/**
 *
 */
int proxenet_javascript_initialize_plugin(plugin_t* plugin)
{
        interpreter_t* interpreter;
	proxenet_js_t *vm;
        v8::TryCatch trycatch;
        char function_names[] = { CFG_REQUEST_PLUGIN_FUNCTION,
                                  CFG_RESPONSE_PLUGIN_FUNCTION,
                                  CFG_ONLOAD_PLUGIN_FUNCTION,
                                  CFG_ONLEAVE_PLUGIN_FUNCTION,
                                  NULL
        };
        char **n;

        if (plugin->interpreter==NULL || plugin->interpreter->ready==false){
                xlog_js(LOG_ERROR, "%s\n", "not ready");
                return -1;
        }

        interpreter = (proxenet_js_t*)plugin->interpreter;
        vm = (proxenet_js_t*)interpreter->vm;

        // switch to plugin context
        Context::Scope context_scope(vm->context);
        vm->source = String::New(fname);
        vm->script = Script::Compile(vm->source);

        if (vm->script.IsEmpty()){
                proxenet_javascript_print_exception(&trycatch);
                return -1;
        }

        Local<Value> value = vm->script->Run();
        if (value.IsEmpty()) {
                proxenet_javascript_print_exception(&trycatch);
                return -1;
        }

        for(n=function_names; *n; n++){
                if(!is_valid_function(interpreter, *n)){
                        xlog_js("'%s' is not a valid function\n", *n);
                        return -1;
                }
        }

        return 0;
}


/**
 *
 */
char* proxenet_javascript_plugin(plugin_t *plugin, request_t *request)
{
	return "";
}

#endif
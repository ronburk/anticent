/* V8Main() - main entry point for the JavaScript environment.
 *
 * Simplifying assumptions: Only one Isolate. Only modules, no scripts. No wasm.
 * ONLYISOLATE is a pointer to ThisIsolate, the only v8::Isolate* that will exist.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <cassert>

#include "libplatform/libplatform.h"
#include "anti8.h"

using v8::Context;
using v8::Local;
using v8::ScriptOrigin;
using v8::Isolate;
using v8::Maybe;
using v8::MaybeLocal;
using v8::Module;
using v8::Integer;
using v8::Boolean;
using v8::PrimitiveArray;
using v8::Value;
using v8::String;

inline Local<Integer> NewV8Int(int val)        { return Integer::New(ONLYISOLATE, val); }
inline Local<Boolean> NewV8Bool(bool val) { return Boolean::New(ONLYISOLATE, val); }
inline Local<Value> NewV8Null()                   { return v8::Null(ONLYISOLATE); }

#if 0
static Local<Module> CreateModule()
    {
    
    }
#endif

/* NewV8Str() - convert const char* to Local<v8::String>
 */

Local<v8::String> NewV8Str(const char* src)
    {
    auto result =
        v8::String::NewFromUtf8(ONLYISOLATE, src, v8::NewStringType::kNormal)
        .ToLocalChecked();
    return result;
        
    }

/* NewScriptOrigin() - create V8 ScriptOrigin from ordinary C++ variables.
 *
 * Even though it's calls ScriptOrigin, it serves for both modules and scripts.
 * Anticent is only using modules, remember.
 *
 * V8's idea of "origin" is a "context". Since I don't know what we're doing with
 * contexts if anything, we'll set that field false.
 */
ScriptOrigin NewScriptOrigin(Local<String> modname, int line=0, int col=0, const char* mapurl=nullptr)
    {
    Local<v8::String> EmptyString;

    return
        ScriptOrigin(
//        Local< Value > resource_name,
        modname,
        NewV8Int(line),                 // resource_line_offset
        NewV8Int(col),                  // resource_column_offset
        NewV8Bool(false),            // resource_is_shared_cross_origin
        NewV8Int(0),                    // script_id ???
        NewV8Null(),                    // source_map_url=Local< Value >(),
        NewV8Bool(true),            // resource_is_opaque=Local< Boolean >(),
        NewV8Bool(false),           // is_wasm=Local< Boolean >(),
        NewV8Bool(true),            // is_module=Local< Boolean >(true),
//        Local<PrimitiveArray>() //host_defined_options=Local< PrimitiveArray >()
        PrimitiveArray::New(ONLYISOLATE, 0)
            );
    }


#if 0
ScriptOrigin ModuleOrigin(Local<v8::Value> resource_name, Isolate* isolate) {
    return
        ScriptOrigin(
        NewV8Str(
        );
  ScriptOrigin origin(resource_name, Local<v8::Integer>(), Local<v8::Integer>(),
                      Local<v8::Boolean>(), Local<v8::Integer>(),
                      Local<v8::Value>(), Local<v8::Boolean>(),
                      Local<v8::Boolean>(), True(isolate));
  return origin;
}
#endif

/* callback from V8 to load a module. Must return a Local<Module>.
 */
MaybeLocal<Module>  ModuleCallback(Local<Context> context,
    Local<v8::String> specifier,
    Local<Module> referrer)
    {
    v8::String::Utf8Value mystr(Isolate::GetCurrent(), specifier);

    printf("Huzzah: module callback invoked: %s\n", *mystr);
    return referrer;
    }

v8::Isolate* ThisIsolate;
void V8Main(int ArgCount, char** Args)
    {
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(Args[0]);
    v8::V8::InitializeExternalStartupData(Args[0]);

    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
//    v8::Isolate* isolate = v8::Isolate::New(create_params);
    // Pointer to the only Isolate. Access via macro ONLYISOLATE.
    ThisIsolate = v8::Isolate::New(create_params);
        {
        v8::Isolate::Scope isolate_scope(ONLYISOLATE);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(ONLYISOLATE);

        // Create a new context.
        Local<v8::Context> context = v8::Context::New(ONLYISOLATE);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);
            {
            // Create a string containing the JavaScript source code.
            Local<v8::String> source = NewV8Str("import './anticent.mjs';\n'hello '+'world';");
//                .ToLocalChecked();
            Local<v8::String> modname=NewV8Str("foo.mjs");
            

//            ScriptOrigin origin = ModuleOrigin(modname, isolate);
            ScriptOrigin origin = NewScriptOrigin(modname);
            v8::ScriptCompiler::Source modsource(source,origin);
            Local<v8::Module> module;
            if(v8::ScriptCompiler::CompileModule(ONLYISOLATE, &modsource).ToLocal(&module))
                {
                printf("compile module worked.\n");
                }
            else
                {
                printf("compile module failed.\n");
                }
            Maybe<bool> status = module->InstantiateModule(context, ModuleCallback);
            if(status.IsJust())
                {
                printf("Module instantiation %s.\n", status.FromJust()?"SUCCESS":"FAILURE");
                }
            else if(status.IsNothing())
                {
                printf("Module instantiate FAILED with exception.\n");
                }
            
#if 0
            Handle<v8::Script> script = v8::Script::Compile(source);
            // Compile the source code.
            v8::Local<v8::Script> script;
            if(v8::Script::Compile(context, source).ToLocal(&script))
                {
                printf("compile worked!\n");
                }
            else
                {
                printf("compile failed!\n");
                }
#endif

            
#if 0
//            v8::MaybeLocal<v8::Script> script =
//                v8::Script::Compile(context, source);

            // Run the script to get the result.
            v8::Local<v8::Value> result = module->Run(context).ToLocalChecked();

            // Convert the result to an UTF8 string and print it.
            v8::String::Utf8Value utf8(isolate, result);
            printf("%s\n", *utf8);
#endif
            }

        }

    // Dispose the isolate and tear down V8. Note that Dispose does a delete this!
    ThisIsolate->Dispose();
    ThisIsolate = nullptr;
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    }


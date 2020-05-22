/* V8Main() - main entry point for the JavaScript environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <cassert>

#include "libplatform/libplatform.h"
#include "v8.h"

using v8::Local;
using v8::ScriptOrigin;
using v8::Isolate;

Local<v8::String> NewV8Str(Isolate* isolate, const char* src)
    {
    Local<v8::String> result =
        v8::String::NewFromUtf8(isolate, "import * from './main.mjs';",
            v8::NewStringType::kNormal)
        .ToLocalChecked();
    return result;
    }

ScriptOrigin ModuleOrigin(Local<v8::Value> resource_name, Isolate* isolate) {
  ScriptOrigin origin(resource_name, Local<v8::Integer>(), Local<v8::Integer>(),
                      Local<v8::Boolean>(), Local<v8::Integer>(),
                      Local<v8::Value>(), Local<v8::Boolean>(),
                      Local<v8::Boolean>(), True(isolate));
  return origin;
}
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
    v8::Isolate* isolate = v8::Isolate::New(create_params);
        {
        v8::Isolate::Scope isolate_scope(isolate);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);

        // Create a new context.
        Local<v8::Context> context = v8::Context::New(isolate);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);
            {
            // Create a string containing the JavaScript source code.
            Local<v8::String> source =
                v8::String::NewFromUtf8(isolate, "import './main.mjs';",
                    v8::NewStringType::kNormal)
                .ToLocalChecked();
            Local<v8::String> modname=NewV8Str(isolate, "root_module");
            

            ScriptOrigin origin = ModuleOrigin(modname, isolate);
            v8::ScriptCompiler::Source modsource(source,origin);
            Local<v8::Module> module;
            if(v8::ScriptCompiler::CompileModule(isolate, &modsource).ToLocal(&module))
                {
                printf("compile module worked.\n");
                }
            else
                {
                printf("compile module failed.\n");
                }
#if 0
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
//            v8::MaybeLocal<v8::Script> script =
//                v8::Script::Compile(context, source);

            // Run the script to get the result.
            v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

            // Convert the result to an UTF8 string and print it.
            v8::String::Utf8Value utf8(isolate, result);
            printf("%s\n", *utf8);
#endif
            }

        }

    // Dispose the isolate and tear down V8. Note that Dispose does a delete this!
    isolate->Dispose();
    isolate = nullptr;
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    }

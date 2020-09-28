#ifndef ANTI8_H_
#define ANTI8_H_

#include "v8.h"

extern v8::Isolate* ThisIsolate;
#define ONLYISOLATE ThisIsolate

v8::Local<v8::String> NewV8Str(const char* src);

#endif /* ANTI_H_ */

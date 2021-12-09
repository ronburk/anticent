/* database.cc - main database program.
 *
 * Syntax:
 *     database <dbfilename> <command> <options>
 */

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <string>
#include <unordered_map>

void    Error(const char* format, ...)
    {
    va_list argptr;
    va_start(argptr, format);
    vfprintf(stderr, format, argptr);
    va_end(argptr);
    fflush(stderr);
    exit(1);
    }

void    Usage()
    {
    fputs(R"(
Usage:
    database <dbfilename> <command> <options>
Command:
    init
        - creates and initializes a new database. It is an error if
          <dbfilename> already exists.

)", stderr);
    exit(1);
    }

int CommandToInt(const char* arg)
    {
    struct Command { int id; const char* name; };
    std::array<Command, 2> commands =
        {
        0, "init",
        1, "check",
        };
    std::string lowerArg{arg};
    
    return 0;
    }

typedef int (*CmdFunc)(const char* filename, int argCount, char** args);
int     CommandInit(const char* filename, int argCount, char**args)
    {
    fprintf(stderr, "init '%s'\n", filename);
    return 0;
    }
int     CommandCheck(const char* filename, int argCount, char**args)
    {
    return 0;
    }

int main(int argCount, char** args)
    {
    std::unordered_map<std::string, CmdFunc> commands =
        {
        {"init", CommandInit},
        {"check", CommandCheck},
        };
    if(argCount <= 1)
        Usage();
    auto iter = commands.find(args[2]);
    if(iter != commands.end())
        return iter->second(args[1], argCount - 3, args+3);
    else
        Usage();
    }

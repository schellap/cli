// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "args.h"
#include "utils.h"

arguments_t::arguments_t() :
    managed_application(_X("")),
    clr_path(_X("")),
    app_argc(0),
    app_argv(nullptr)
{
}

void display_help()
{
    xerr <<
        _X("Usage: " HOST_EXE_NAME " [-p SERVER_GC=0/1] [ASSEMBLY] [ARGUMENTS]\n")
        _X("Execute the specified managed assembly with the passed in arguments\n\n")
        _X("The Host's behavior can be altered using the following environment variables:\n")
        _X(" DOTNET_HOME            Set the dotnet home directory. The CLR is expected to be in the runtime subdirectory of this directory. Overrides all other values for CLR search paths\n")
        _X(" COREHOST_TRACE          Set to affect trace levels (0 = Errors only (default), 1 = Warnings, 2 = Info, 3 = Verbose)\n");
}

bool parse_arguments(const int argc, const pal::char_t* argv[], arguments_t& args)
{
    // Get the full name of the application
    if (!pal::get_own_executable_path(args.own_path) || !pal::realpath(args.own_path))
    {
        trace::error(_X("failed to locate current executable"));
        return false;
    }

    args.app_argc = argc - 1;
    args.app_argv = argv + 1;

    // Parse own properties specified with -p KEY1=VALUE1 -p KEY2=VALUE2.
    while (args.app_argc > 2)
    {
        pal::string_t curopt(args.app_argv[0]);
        pal::string_t optval(args.app_argv[1]);

        if (curopt.compare(_X("-p")) == 0)
        {
            size_t pos = optval.find(_X("="));
            if (pos == 0 || pos == pal::string_t::npos)
            {
                display_help();
                return false;
            }

            pal::string_t key = optval.substr(0, pos);
            pal::string_t value = optval.substr(pos + 1);
            args.own_properties.emplace(pal::to_stdstring(key), pal::to_stdstring(value));

            args.app_argc -= 2;
            args.app_argv += 2;
        }
        else
        {
            // Encountered an option we do not know about.
            break;
        }
    }

    auto own_name = get_filename(args.own_path);
    auto own_dir = get_directory(args.own_path);
    if (own_name.compare(HOST_EXE_NAME) == 0)
    {
        // corerun mode. Next argument is managed app
        if (args.app_argc < 1)
        {
            display_help();
            return false;
        }
        args.managed_application = pal::string_t(args.app_argv[0]);
    }
    else
    {
        // coreconsole mode. Find the managed app in the same directory
        pal::string_t managed_app(own_dir);
        managed_app.push_back(DIR_SEPARATOR);
        managed_app.append(get_executable(own_name));
        managed_app.append(_X(".dll"));
        args.managed_application = managed_app;
        args.app_argv++;
        args.app_argc--;
    }

    // Read trace environment variable
    pal::string_t trace_str;
    if (pal::getenv(_X("COREHOST_TRACE"), trace_str))
    {
        auto trace_val = pal::xtoi(trace_str.c_str());
        if (trace_val > 0)
        {
            trace::enable();
            trace::info(_X("tracing enabled"));
        }
    }

    // Read CLR path from environment variable
    pal::string_t home_str;
    pal::getenv(_X("DOTNET_HOME"), home_str);
    if (!home_str.empty())
    {
        append_path(home_str, _X("runtime"));
        append_path(home_str, _X("coreclr"));
        args.clr_path.assign(home_str);
    }
    else
    {
        // Use platform-specific search algorithm
        if (pal::find_coreclr(home_str))
        {
            args.clr_path.assign(home_str);
        }
    }

    return true;
}

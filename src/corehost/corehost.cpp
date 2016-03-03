// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "trace.h"
#include "utils.h"
#include "libhost.h"
#include "corehost.h"

extern int corehost_main(const int argc, const pal::char_t* argv[]);

namespace
{
enum StatusCode
{
    Success                   = 0,
    CoreHostLibLoadFailure    = 0x41,
    CoreHostLibMissingFailure = 0x42,
    CoreHostEntryPointFailure = 0x43,
    CoreHostCurExeFindFailure = 0x44,
};

typedef int (*corehost_main_fn) (const int argc, const pal::char_t* argv[]);


bool hostpolicy_exists_in_dir(const pal::string_t& lib_dir, pal::string_t* p_host_path)
{
    pal::string_t host_path = lib_dir;
    append_path(&host_path, LIBHOST_NAME);

    if (!pal::file_exists(host_path))
    {
        return false;
    }
    *p_host_path = host_path;
    return true;
}

// -----------------------------------------------------------------------------
// Load the corehost library from the path specified
//
// Parameters:
//    lib_dir      - dir path to the corehost library
//    h_host       - handle to the library which will be kept live
//    main_fn      - Contains the entrypoint "corehost_main" when returns success.
//
// Returns:
//    Non-zero exit code on failure. "main_fn" contains "corehost_main"
//    entrypoint on success.
//
StatusCode load_host_lib(const pal::string_t& lib_dir, pal::dll_t* h_host, corehost_main_fn* main_fn)
{
    pal::string_t host_path;
    if (!hostpolicy_exists_in_dir(lib_dir, &host_path))
    {
        return StatusCode::CoreHostLibMissingFailure;
    }

    // Load library
    if (!pal::load_library(host_path.c_str(), h_host))
    {
        trace::info(_X("Load library of %s failed"), host_path.c_str());
        return StatusCode::CoreHostLibLoadFailure;
    }

    // Obtain entrypoint symbol
    *main_fn = (corehost_main_fn) pal::get_symbol(*h_host, "corehost_main");

    return (*main_fn != nullptr)
                ? StatusCode::Success
                : StatusCode::CoreHostEntryPointFailure;
}

}; // end of anonymous namespace

HostMode detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_path, pal::string_t* own_dir, pal::string_t* own_name)
{
    // Get the full name of the application
    if (!pal::get_own_executable_path(own_path) || !pal::realpath(own_path))
    {
        trace::error(_X("Failed to locate current executable"));
        own_path->clear();
        return Invalid;
    }

    own_name->assign(get_filename(*own_path));
    own_dir->assign(get_directory(*own_path));

    if (coreclr_exists_in_dir(*own_dir))
    {
        pal::string_t own_deps_json = *own_dir;
        pal::string_t own_deps_filename = strip_file_ext(*own_name) + _X(".deps.json");
        append_path(&own_deps_json, own_deps_filename.c_str());
        return (pal::file_exists(own_deps_json)) ? Standalone : Framework;
    }
    else
    {
        return Muxer;
    }
}

StatusCode resolve_hostpolicy_dir(const int argc, const pal::char_t* argv[], pal::string_t* resolved_path)
{
    pal::string_t own_path, own_dir, own_name;
    HostMode mode = detect_operating_mode(argc, argv, &own_path, &own_dir, &own_name);

    switch (mode)
    {
    case Muxer:
        // do muxing
        break;

    case Framework:
    {
        if (hostpolicy_exists_in_dir(own_dir, resolved_path))
        {
            return StatusCode::Success;
        }
        return StatusCode::CoreHostLibMissingFailure;
    }
    break;

    case Standalone:
        {
#ifdef COREHOST_PACKAGE_SERVICING
            pal::string_t svc_dir;
            if (pal::getenv(_X("DOTNET_SERVICING"), &svc_dir))
            {
                pal::string_t path = svc_dir;
                append_path(&path, COREHOST_PACKAGE_NAME);
                append_path(&path, COREHOST_PACKAGE_VERSION);
                append_path(&path, COREHOST_PACKAGE_COREHOST_RELATIVE_DIR);

                if (hostpolicy_exists_in_dir(path, resolved_path)
                {
                    return StatusCode::Success;
                }
            }
#endif
            if (hostpolicy_exists_in_dir(own_dir, resolved_path))
            {
                return StatusCode::Success;
            }
            return StatusCode::CoreHostLibMissingFailure;
        }
        break;

    }
}

#if defined(_WIN32)
int __cdecl wmain(const int argc, const pal::char_t* argv[])
#else
int main(const int argc, const pal::char_t* argv[])
#endif
{
    trace::setup();

    pal::dll_t corehost;

    corehost_main_fn host_main;
    StatusCode code = load_host_lib(resolve_hostpolicy_path(argc, argv), &corehost, &host_main);
    switch (code)
    {
    // Success, call the entrypoint.
    case StatusCode::Success:
        trace::info(_X("Calling host entrypoint from library at own dir %s"), own_dir.c_str());
        return host_main(argc, argv);

    // Some other fatal error including StatusCode::CoreHostLibMissingFailure.
    default:
        trace::error(_X("Error loading the host library from own dir: %s; Status=%08X"), own_dir.c_str(), code);
        return code;
    }
}


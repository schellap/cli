// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cassert>
#include "pal.h"
#include "utils.h"
#include "libhost.h"
#include "args.h"
#include "fx_ver.h"
#include "fx_muxer.h"
#include "trace.h"
#include "cpprest/json.h"

typedef web::json::value json_value;

pal::string_t fx_muxer_t::resolve_fx_dir(const pal::string_t& app_path)
{
    return _X("");
}

pal::string_t fx_muxer_t::resolve_cli_version(const pal::string_t& global_json)
{
    pal::string_t retval;
    if (!pal::file_exists(global_json))
    {
        return retval;
    }

    pal::ifstream_t file(global_json);
    if (!file.good())
    {
        return retval;
    }

    try
    {
        const auto& json = json_value::parse(file);
        const auto& sdk_ver = json.get(_X("sdkVersion"));
        const auto& autoUpgrade = json.get(_X("sdkVersion"));
        if (sdk_ver.is_null())
        {
            return retval;
        }
        return sdk_ver.as_string();
    }
    catch (...)
    {
        return false;
    }
}

bool fx_muxer_t::resolve_sdk_dotnet_path(const pal::string_t& own_dir, pal::string_t* cli_sdk)
{
    pal::string_t cwd;
    pal::string_t global;
    if (pal::getcwd(&cwd))
    {
        for (pal::string_t parent_dir, cur_dir = cwd; true; cur_dir = parent_dir)
        {
            pal::string_t file = cur_dir;
            append_path(&file, _X("global.json"));
            if (pal::file_exists(file))
            {
                global = file;
                break;
            }
			parent_dir = get_directory(cur_dir);
            if (parent_dir.empty() || parent_dir.size() == cur_dir.size())
            {
                break;
            }
        }
    }
    pal::string_t retval;
    if (!global.empty())
    {
        pal::string_t cli_version = resolve_cli_version(global);
        if (!cli_version.empty())
        {
            pal::string_t sdk_path = own_dir;
            append_path(&sdk_path, _X("sdk"));
            append_path(&sdk_path, cli_version.c_str());
            if (pal::directory_exists(sdk_path))
            {
                retval = sdk_path;
            }
        }
    }
    if (retval.empty())
    {
        pal::string_t sdk_path = own_dir;
        append_path(&sdk_path, _X("sdk"));
        std::vector<pal::string_t> versions;
        pal::readdir(sdk_path, &versions);

        std::vector<fx_ver_t> sem_vers;
        sem_vers.reserve(versions.size());
        for (const auto& version : versions)
        {
            fx_ver_t ver(0, 0, 0);
            if (fx_ver_t::parse(version, &ver, true))
            {
                sem_vers.push_back(ver);
            }
        }
        auto iter = std::max_element(sem_vers.begin(), sem_vers.end());
        if (iter != sem_vers.end())
        {
            append_path(&sdk_path, iter->as_str().c_str());
            retval = sdk_path;
        }
    }
    cli_sdk->assign(retval);
	return !retval.empty();
}

extern int execute_app(const pal::string_t& fx_dir, const int argc, const pal::char_t* argv[]);

/* static */
int fx_muxer_t::execute(const int argc, const pal::char_t* argv[])
{
    pal::string_t app_path = pal::string_t(argv[1]);
    if (ends_with(app_path, _X(".dll"), false))
    {
        if (!pal::realpath(&app_path))
        {
            return LibHostStatusCode::LibHostExecModeFailure;
        }

        pal::string_t fx_dir = resolve_fx_dir(app_path);
        return execute_app(fx_dir, argc, argv);
    }
    else
    {
        // Get the full name of the application
        pal::string_t own_path;
        if (!pal::get_own_executable_path(&own_path) || !pal::realpath(&own_path))
        {
            trace::error(_X("Failed to locate current executable"));
            return LibHostStatusCode::LibHostCurExeFindFailure;
        }

        pal::string_t own_dir = get_directory(own_path);

        pal::string_t sdk_dotnet;
        if (!resolve_sdk_dotnet_path(own_dir, &sdk_dotnet))
        {
            return LibHostStatusCode::LibHostSdkFindFailure;
        }

        pal::string_t fx_dir = resolve_fx_dir(sdk_dotnet);

        // Transform dotnet [command] [args] -> dotnet [dotnet.dll] [command] [args]
        // This can be made better to just setup a arguments_t args and do run(args) if it matters.
        {
            std::vector<const pal::char_t*> new_argv(argc + 1);
            memcpy(&new_argv.data()[1], argv, argc);
            new_argv[0] = argv[0];
            new_argv[1] = sdk_dotnet.c_str();

            assert(ends_with(sdk_dotnet, _X(".dll"), false));
            return execute_app(fx_dir, new_argv.size(), new_argv.data());
        }
    }
}


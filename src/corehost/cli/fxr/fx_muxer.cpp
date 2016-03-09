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
#include "runtime_config.h"
#include "cpprest/json.h"
#include "corehost.h"
#include "policy_load.h"

typedef web::json::value json_value;

pal::string_t fx_muxer_t::resolve_fx_dir(const pal::string_t& muxer_path, runtime_config_t* runtime, const pal::string_t& app_path)
{
	auto specified_fx = runtime->get_fx_version();
	auto roll_fwd = runtime->get_fx_roll_fwd();
	fx_ver_t specified(-1, -1, -1);
	if (!fx_ver_t::parse(specified_fx, &specified, true))
	{
		return pal::string_t();
	}

	std::vector<pal::string_t> list;
	auto fx_dir = get_directory(muxer_path);
	append_path(&fx_dir, _X("Shared"));
	append_path(&fx_dir, _X("NetCoreApp"));

	// If not roll forward or if pre-release, just return.
	if (!roll_fwd || !specified.pre.empty())
	{
		append_path(&fx_dir, specified_fx.c_str());
	}
	else
	{
		pal::readdir(fx_dir, &list);
		fx_ver_t max_specified = specified;
		for (const auto& version : list)
		{
			fx_ver_t ver(-1, -1, -1);
			if (fx_ver_t::parse(version, &ver, true) &&
				ver.major == max_specified.major &&
				ver.minor == max_specified.minor)
			{
				max_specified.patch = max(ver.patch, max_specified.patch);
			}
		}
		append_path(&fx_dir, max_specified.as_str().c_str());
	}
	return pal::directory_exists(fx_dir) ? fx_dir : pal::string_t();
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
		fx_ver_t max_ver(-1, -1, -1);
        for (const auto& version : versions)
        {
            fx_ver_t ver(-1, -1, -1);
            if (fx_ver_t::parse(version, &ver, true))
            {
				max_ver = max(ver, max_ver);
            }
        }
		append_path(&sdk_path, max_ver.as_str().c_str());
		if (pal::directory_exists(sdk_path))
		{
			retval = sdk_path;
		}
	}
    cli_sdk->assign(retval);
	return !retval.empty();
}

/* static */
int fx_muxer_t::execute(const int argc, const pal::char_t* argv[])
{
    pal::string_t app_path;

    // Get the full name of the application
    if (!pal::get_own_executable_path(&app_path) || !pal::realpath(&app_path))
    {
        trace::error(_X("Failed to locate current executable"));
        return LibHostStatusCode::LibHostCurExeFindFailure;
    }

    auto own_dir = get_directory(app_path);

	runtime_config_t config(get_runtime_config_json(app_path));
    if (ends_with(app_path, _X(".dll"), false))
    {
        if (!pal::realpath(&app_path))
        {
            return LibHostStatusCode::LibHostExecModeFailure;
        }

        pal::string_t fx_dir = resolve_fx_dir(own_dir, &config, app_path);
        return policy_load_t::execute_app(fx_dir, fx_dir, &config, argc, argv);
    }
    else
    {
        pal::string_t sdk_dotnet;
        if (!resolve_sdk_dotnet_path(own_dir, &sdk_dotnet))
        {
            return LibHostStatusCode::LibHostSdkFindFailure;
        }

		pal::string_t fx_dir = resolve_fx_dir(own_dir, &config, sdk_dotnet);

        // Transform dotnet [command] [args] -> dotnet [dotnet.dll] [command] [args]
        {
            std::vector<const pal::char_t*> new_argv(argc + 1);
            memcpy(&new_argv.data()[1], argv, argc);
            new_argv[0] = argv[0];
            new_argv[1] = sdk_dotnet.c_str();

            assert(ends_with(sdk_dotnet, _X(".dll"), false));
            return policy_load_t::execute_app(fx_dir, _X(""), &config, new_argv.size(), new_argv.data());
        }
    }
}

SHARED_API int hostfxr_main(const int argc, const pal::char_t* argv[])
{
    return fx_muxer_t().execute(argc, argv);
}
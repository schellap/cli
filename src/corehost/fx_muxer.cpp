// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "utils.h"
#include "fx_muxer.h"

bool fx_muxer_t::determine_sdk_dotnet_path(const pal::string_t& own_dir, pal::string_t* cli_sdk)
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
        pal::string_t sdk_path = own_dir;
        append_path(&sdk_path, _X("sdk"));
        pal::string_t cli_version = global_json_t::get_cli_version(global);
        if (!cli_version.empty())
        {
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

        std::vector<sem_ver_t> sem_vers;
        sem_vers.reserve(versions.size());
        for (const auto& version : versions)
        {
            sem_vers.push_back(sem_ver_t(version));
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

StatusCode fx_muxer_t::execute()
{
    pal::string_t managed_application = pal::string_t(m_argv[1]);
    if (ends_with(managed_application, _X(".dll"), false))
    {
        if (!pal::realpath(&managed_application))
        {
            return false;
        }
		// do app
    }
    else
    {
        pal::string_t retval = determine_sdk_dotnet_path(own_dir);
        if (retval.empty())
        {
            return false;
        }

		std::vector<pal::char_t*> new_argv;
		memcpy(&new_argv.data()[1], m_argv, m_argc);
		new_argv[0] = new_argv[1];
		new_argv[1] = retval;
		return fx_muxer_t(new_argv.size(), new_argv.data()).execute();
    }
}


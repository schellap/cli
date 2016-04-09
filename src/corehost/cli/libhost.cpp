// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "utils.h"
#include "trace.h"
#include "libhost.h"

pal::string_t get_runtime_config_from_file(const pal::string_t& file, pal::string_t* dev_cfg)
{
    auto name = get_filename_without_ext(file);
    auto json_name = name + _X(".runtimeconfig.json");
    auto dev_json_name = name + _X(".runtimeconfig.dev.json");
    auto json_path = get_directory(file);
    auto dev_json_path = json_path;

    append_path(&json_path, json_name.c_str());
    append_path(&dev_json_path, dev_json_name.c_str());
    trace::verbose(_X("Runtime config is cfg=%s dev=%s"), json_path.c_str(), dev_json_path.c_str());

    dev_cfg->assign(dev_json_path);
    return json_path;
}

void try_patch_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str)
{
    pal::string_t path = cur_dir;

    if (trace::is_enabled())
    {
        pal::string_t start_str = start_ver.as_str();
        trace::verbose(_X("Reading patch roll forward candidates in dir [%s] for version [%s]"), path.c_str(), start_str.c_str());
    }

    pal::string_t maj_min_star = start_ver.patch_glob();

    std::vector<pal::string_t> list;
    pal::readdir(path, maj_min_star, &list);

    fx_ver_t max_ver = start_ver;
    fx_ver_t ver(-1, -1, -1);
    for (const auto& str : list)
    {
        trace::verbose(_X("Considering patch roll forward candidate version [%s]"), str.c_str());
        if (fx_ver_t::parse(str, &ver, true))
        {
            max_ver = std::max(ver, max_ver);
        }
    }
    max_str->assign(max_ver.as_str());

    if (trace::is_enabled())
    {
        pal::string_t start_str = start_ver.as_str();
        trace::verbose(_X("Patch roll forwarded [%s] -> [%s] in [%s]"), start_str.c_str(), max_str->c_str(), path.c_str());
    }
}

void try_prerelease_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str)
{
    pal::string_t path = cur_dir;

    if (trace::is_enabled())
    {
        pal::string_t start_str = start_ver.as_str();
        trace::verbose(_X("Reading prerelease roll forward candidates in dir [%s] for version [%s]"), path.c_str(), start_str.c_str());
    }

    pal::string_t maj_min_pat_star = start_ver.prerelease_glob();

    std::vector<pal::string_t> list;
    pal::readdir(path, maj_min_pat_star, &list);

    fx_ver_t max_ver = start_ver;
    fx_ver_t ver(-1, -1, -1);
    for (const auto& str : list)
    {
        trace::verbose(_X("Considering prerelease roll forward candidate version [%s]"), str.c_str());
        if (fx_ver_t::parse(str, &ver, false)
            && ver.is_prerelease()) // Pre-release can roll forward to only pre-release
        {
            max_ver = std::max(ver, max_ver);
        }
    }
    max_str->assign(max_ver.as_str());

    if (trace::is_enabled())
    {
        pal::string_t start_str = start_ver.as_str();
        trace::verbose(_X("Prerelease roll forwarded [%s] -> [%s] in [%s]"), start_str.c_str(), max_str->c_str(), path.c_str());
    }
}

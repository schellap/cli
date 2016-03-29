// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef ARGS_H
#define ARGS_H

#include "utils.h"
#include "pal.h"
#include "trace.h"
#include "libhost.h"

struct probe_config_t
{
    pal::string_t probe_dir;
    bool match_hash;
    bool roll_forward;
    bool only_runtime_assets;
    bool only_serviceable_assets;
    bool only_portable_assets;
    deps_json_t* probe_deps_json;

    probe_config_t(
        const pal::string_t& probe_dir,
        bool match_hash,
        bool roll_forward,
        bool only_serviceable_assets,
        bool only_runtime_assets,
        const deps_json_t* consult_deps_json)
        : probe_dir(probe_dir)
        , match_hash(match_hash)
        , roll_forward(roll_forward)
        , only_serviceable_assets(only_serviceable_assets)
        , only_runtime_assets(only_runtime_assets)
        , probe_deps_json(probe_deps_json)
    {
        // Cannot roll forward and also match hash.
        assert(!roll_forward || !match_hash);
    }

    static probe_config_t svc_ni(const pal::string_t dir, bool roll_fwd)
    {
        return probe_config_t(dir, false, roll_fwd, true, true, nullptr);
    }

    static probe_config_t svc(const pal::string_t dir, bool roll_fwd)
    {
        return probe_config_t(dir, false, roll_fwd, true, false, nullptr);
    }

    static probe_config_t cache_ni(const pal::string_t dir)
    {
        return probe_config_t(dir, true, false, false, true, nullptr);
    }
    
    static probe_config_t cache(const pal::string_t dir)
    {
        return probe_config_t(dir, true, false, false, false, nullptr);
    }

    static probe_config_t fx(const pal::string_t dir, const deps_json_t* deps)
    {
        return probe_config_t(dir, false, false, false, false, deps);
    }

    static probe_config_t additional(const pal::string_t dir, bool roll_fwd)
    {
        return probe_config_t(dir, false, roll_fwd, false, false, nullptr);
    }
};

struct arguments_t
{
    pal::string_t own_path;
    pal::string_t app_dir;
    pal::string_t deps_path;
    pal::string_t dotnet_extensions;
    pal::string_t probe_dir;
    pal::string_t dotnet_packages_cache;
    pal::string_t managed_application;

    int app_argc;
    const pal::char_t** app_argv;

    arguments_t();

    inline void print()
    {
        if (trace::is_enabled())
        {
            trace::verbose(_X("args: own_path=%s app_dir=%s deps=%s extensions=%s probe_dir=%s packages_cache=%s mgd_app=%s"),
                own_path.c_str(), app_dir.c_str(), deps_path.c_str(), dotnet_extensions.c_str(), probe_dir.c_str(), dotnet_packages_cache.c_str(), managed_application.c_str());
        }
    }
};

bool parse_arguments(const pal::string_t& deps_path, const pal::string_t& probe_dir, host_mode_t mode, const int argc, const pal::char_t* argv[], arguments_t* args);

#endif // ARGS_H

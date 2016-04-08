// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __LIBHOST_H__
#define __LIBHOST_H__

#include "runtime_config.h"
#include "fx_ver.h"

enum host_mode_t
{
    invalid = 0,
    muxer,
    standalone,
    split_fx
};

class fx_ver_t;
class runtime_config_t;

class corehost_init_t
{
private:
    std::vector<pal::string_t> m_keys;
    std::vector<pal::string_t> m_values;
    pal::string_t m_fx_dir;
    const runtime_config_t* m_runtime_config;
public:
    corehost_init_t(
        const pal::string_t& deps_file,
        const std::vector<pal::string_t>& probe_paths,
        const pal::string_t& fx_dir,
        const host_mode_t mode,
        const runtime_config_t* config)
        : m_runtime_config(config)
        , m_fx_dir(fx_dir)
    {
        pal::stringstream_t joined;
        for (const auto& probe : probe_paths)
        {
            joined << probe;
            joined << PATH_SEPARATOR;
        }
        for (const auto& probe : config->get_probe_paths())
        {
            joined << probe;
            joined << PATH_SEPARATOR;
        }

        m_keys.push_back(_X("host.app.deps_file"));
        m_keys.push_back(_X("host.app.probe_paths"));
        m_keys.push_back(_X("host.fx.dir"));
        m_keys.push_back(_X("host.mode"));
        m_keys.push_back(_X("host.patch_roll_forward"));
        m_keys.push_back(_X("host.prerelease_roll_forward"));
        m_keys.push_back(_X("host.is_portable"));
        m_keys.push_back(_X("host.fx.name"));

        m_values.push_back(deps_file);
        m_values.push_back(joined.str());
        m_values.push_back(fx_dir);
        m_values.push_back(pal::to_string(mode));
        m_values.push_back(pal::to_string(config->get_patch_roll_fwd()));
        m_values.push_back(pal::to_string(config->get_prerelease_roll_fwd()));
        m_values.push_back(pal::to_string(config->get_portable()));
        m_values.push_back(config->get_fx_name());

        std::vector<pal::string_t> keys;
        std::vector<pal::string_t> values;
        for (const auto& pair : config->properties())
        {
            keys.push_back(_X("coreclr.") + pair.first);
            values.push_back(pair.second);
        }
    }

    const runtime_config_t* runtime_config() const
    {
        return m_runtime_config;
    }

    const pal::string_t& fx_dir() const
    {
        return m_fx_dir;
    }

};


struct hostpolicy_init_t
{
    pal::string_t deps_file;
    std::vector<pal::string_t> probe_paths;
    pal::string_t fx_dir;
    pal::string_t fx_name;
    host_mode_t host_mode;
    bool patch_roll_forward;
    bool prerelease_roll_forward;
    bool is_portable;
    std::vector<std::string> cfg_keys;
    std::vector<std::string> cfg_values;

    static void init(pal::char_t** keys, pal::char_t** values, int size, hostpolicy_init_t* init)
    {
        init->patch_roll_forward = false;
        init->prerelease_roll_forward = false;
        init->is_portable = false;

        for (int i = 0; i < size; ++i)
        {
            if (pal::strcasecmp(keys[i], _X("host.app.deps_file")) == 0)
            {
                init->deps_file = values[i];
            }
            else if (pal::strcasecmp(keys[i], _X("host.fx.dir")) == 0)
            {
                init->fx_dir = values[i];
            }
            else if (pal::strcasecmp(keys[i], _X("host.fx.name")) == 0)
            {
                init->fx_name = values[i];
            }
            else if (pal::strcasecmp(keys[i], _X("host.mode")) == 0)
            {
                init->host_mode = (host_mode_t)std::stoi(pal::string_t(values[i]));
            }
            else if (pal::strcasecmp(keys[i], _X("host.app.probe_paths")) == 0)
            {
                pal::char_t* ptr = values[i];
                pal::char_t* buf = ptr;
                while (*ptr)
                {
                    if (*ptr == PATH_SEPARATOR)
                    {
                        init->probe_paths.push_back(pal::string_t(buf, ptr - buf));
                        buf = ptr + 1;
                    }
                    ptr++;
                }
                init->probe_paths.push_back(pal::string_t(buf, ptr - buf));
            }
            else if (pal::strcasecmp(keys[i], _X("host.patch_roll_forward")) == 0)
            {
                init->patch_roll_forward = std::stoi(pal::string_t(values[i]));
            }
            else if (pal::strcasecmp(keys[i], _X("host.prerelease_roll_forward")) == 0)
            {
                init->prerelease_roll_forward = std::stoi(pal::string_t(values[i]));
            }
            else if (pal::strcasecmp(keys[i], _X("host.is_portable")) == 0)
            {
                init->is_portable = std::stoi(pal::string_t(values[i]));
            }
            else if (starts_with(keys[i], _X("coreclr."), false))
            {
                pal::string_t key = keys[i];
                key.substr(pal::string_t(_X("coreclr.")).size());
                init->cfg_keys.push_back(pal::to_stdstring(key));
                init->cfg_values.push_back(pal::to_stdstring(values[i]));
            }
        }
    }
};

pal::string_t get_runtime_config_from_file(const pal::string_t& file, pal::string_t* dev_config_file);
host_mode_t detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_dir = nullptr);

void try_patch_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);
void try_prerelease_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);

#endif // __LIBHOST_H__

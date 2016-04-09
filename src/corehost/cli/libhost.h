// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __LIBHOST_H__
#define __LIBHOST_H__
#include <stdint.h>
#include "trace.h"
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

struct strarr_t
{
    // Do not modify this struct
    int32_t len;
    const pal::char_t** arr;
};

struct host_interface_t
{
    int32_t own_size;
    strarr_t config_keys;
    strarr_t config_values;
    const pal::char_t* fx_dir;
    const pal::char_t* fx_name;
    const pal::char_t* deps_file;
    int8_t is_portable;
    strarr_t probe_paths;
    int8_t patch_roll_forward;
    int8_t prerelease_roll_forward;
    // Only append to this structure to maintain compat. Only ints and strings, no bools.
};

class corehost_init_t
{
private:
    std::vector<pal::string_t> m_clr_keys;
    std::vector<pal::string_t> m_clr_values;
    std::vector<const pal::char_t*> m_clr_keys_cstr;
    std::vector<const pal::char_t*> m_clr_values_cstr;
    const pal::string_t m_fx_dir;
    const pal::string_t m_fx_name;
    const pal::string_t m_fx_ver;
    const pal::string_t m_deps_file;
    bool m_portable;
    std::vector<pal::string_t> m_probe_paths;
    std::vector<const pal::char_t*> m_probe_paths_cstr;
    host_mode_t m_host_mode;
    bool m_patch_roll_forward;
    bool m_prerelease_roll_forward;

    host_interface_t m_host_interface;
public:
    corehost_init_t(
        const pal::string_t& deps_file,
        const std::vector<pal::string_t>& probe_paths,
        const pal::string_t& fx_dir,
        const host_mode_t mode,
        const runtime_config_t& runtime_config)
        : m_fx_dir(fx_dir)
        , m_fx_name(runtime_config.get_fx_name())
        , m_fx_ver(runtime_config.get_fx_version())
        , m_deps_file(deps_file)
        , m_probe_paths(probe_paths)
        , m_host_mode(mode)
        , m_portable(runtime_config.get_portable())
        , m_patch_roll_forward(runtime_config.get_patch_roll_fwd())
        , m_prerelease_roll_forward(runtime_config.get_prerelease_roll_fwd())
        , m_host_interface{ 0 }
    {
        runtime_config.config_kv(&m_clr_keys, &m_clr_values);
        make_cstr_arr(m_clr_keys, &m_clr_keys_cstr);
        make_cstr_arr(m_clr_values, &m_clr_values_cstr);
        make_cstr_arr(m_probe_paths, &m_probe_paths_cstr);
    }

    const pal::string_t& fx_dir() const
    {
        return m_fx_dir;
    }

    const pal::string_t& fx_name() const
    {
        return m_fx_name;
    }

    const pal::string_t& fx_version() const
    {
        return m_fx_ver;
    }

    const host_interface_t& get_host_init_data()
    {
        host_interface_t& hi = m_host_interface;

        hi.own_size = sizeof(host_interface_t);

        hi.config_keys.len = m_clr_keys_cstr.size();
        hi.config_keys.arr = m_clr_keys_cstr.data();

        hi.config_values.len = m_clr_values_cstr.size();
        hi.config_values.arr = m_clr_values_cstr.data();

        hi.fx_dir = m_fx_dir.c_str();
        hi.fx_name = m_fx_name.c_str();
        hi.deps_file = m_deps_file.c_str();
        hi.is_portable = m_portable;

        hi.probe_paths.len = m_probe_paths_cstr.size();
        hi.probe_paths.arr = m_probe_paths_cstr.data();

        hi.patch_roll_forward = m_patch_roll_forward;
        hi.prerelease_roll_forward = m_prerelease_roll_forward;
        
        return hi;
    }

private:

    static void make_cstr_arr(const std::vector<pal::string_t>& arr, std::vector<const pal::char_t*>* out)
    {
        out->reserve(arr.size());
        for (const auto& str : arr)
        {
            out->push_back(str.c_str());
        }
    }
};


struct hostpolicy_init_t
{
    std::vector<std::string> cfg_keys;
    std::vector<std::string> cfg_values;
    pal::string_t deps_file;
    std::vector<pal::string_t> probe_paths;
    pal::string_t fx_dir;
    pal::string_t fx_name;
    host_mode_t host_mode;
    bool patch_roll_forward;
    bool prerelease_roll_forward;
    bool is_portable;

    static void init(host_interface_t* input, hostpolicy_init_t* init)
    {
        trace::verbose(_X("Reading from host interface version: [%d] policy version: [%d]"), input->own_size, sizeof(host_interface_t));

        make_stdstr_arr(input->config_keys.len, input->config_keys.arr, &init->cfg_keys);
        make_stdstr_arr(input->config_values.len, input->config_values.arr, &init->cfg_values);

        init->fx_dir = input->fx_dir;
        init->fx_name = input->fx_name;
        init->deps_file = input->deps_file;
        init->is_portable = input->is_portable;

        make_palstr_arr(input->probe_paths.len, input->probe_paths.arr, &init->probe_paths);

        init->patch_roll_forward = input->patch_roll_forward;
        init->prerelease_roll_forward = input->prerelease_roll_forward;
    }

private:
    static void make_palstr_arr(int argc, const pal::char_t** argv, std::vector<pal::string_t>* out)
    {
        out->reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            out->push_back(argv[i]);
        }
    }

    static void make_stdstr_arr(int argc, const pal::char_t** argv, std::vector<std::string>* out)
    {
        out->reserve(argc);
        for (int i = 0; i < argc; ++i)
        {
            out->push_back(pal::to_stdstring(argv[i]));
        }
    }
};

pal::string_t get_runtime_config_from_file(const pal::string_t& file, pal::string_t* dev_config_file);
host_mode_t detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_dir = nullptr);

void try_patch_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);
void try_prerelease_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);

#endif // __LIBHOST_H__

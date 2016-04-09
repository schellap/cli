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

enum host_keys_t
{
    // Only append to this list.
    invalid,
    config_keys,
    config_values,
    fx_dir,
    fx_name,
    fx_version,
    deps_file,
    probe_paths,
    host_mode,
    patch_roll_forward,
    prerelease_roll_forward,
    // Only append to this list.
};

struct host_interface_t
{
    virtual bool peek_strarr(host_keys_t key, const pal::char_t** arr, int* len) = 0;
    virtual bool peek_string(host_keys_t key, const pal::char_t** val) = 0;
    virtual bool peek_intarr(host_keys_t key, const int** arr, int* len) = 0;
    virtual bool peek_int(host_keys_t key, int* val) = 0;
    virtual bool peek_arr(host_keys_t key, const void** val, int* len) = 0;
    virtual bool peek(host_keys_t key, const void** val) = 0;
};

class corehost_init_t : host_interface_t
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
        return *this;
    }

    bool peek_string(host_keys_t keys, const pal::char_t** str)
    {
        switch (keys)
        {
        case host_keys_t::fx_dir:
            *str = m_fx_dir.c_str();
            return true;
        case host_keys_t::fx_name:
            *str = m_fx_name.c_str();
            return true;
        case host_keys_t::fx_version:
            *str = m_fx_ver.c_str();
            return true;
        case host_keys_t::deps_file:
            *str = m_deps_file.c_str();
            return true;
        default:
            return false;
        }
    }

    bool peek_array(host_keys_t keys, void** arr, int* len)
    {
        switch (keys)
        {
        case host_keys_t::config_keys:
            *arr = m_clr_keys_cstr.data();
            *len = m_clr_keys_cstr.size();
            return true;

        case host_keys_t::config_values:
            *arr = m_clr_values_cstr.data();
            *len = m_clr_values_cstr.size();
            return true;

        case host_keys_t::probe_paths:
            *arr = m_probe_paths_cstr.data();
            *len = m_probe_paths_cstr.size();
            return true;

        default:
            return false;
        }
    }

    bool peek_int(host_keys_t keys, int* value)
    {
        switch (keys)
        {
        case host_mode:
            *value = m_host_mode;
            return true;
        case patch_roll_forward:
            *value = m_patch_roll_forward;
            return true;
        case prerelease_roll_forward:
            *value = prerelease_roll_forward;
            return true;
        default:
            return false;
        }
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
        input->peek_array(host_keys_t::config_keys, )
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

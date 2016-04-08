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

class host_interface_t
{
    virtual pal::char_t* get_string_value(pal::char_t* name) = 0;
    virtual void del_string_value(pal::char_t* value) = 0;
    virtual int get_string_array(pal::char_t* name, pal::char_t*** value) = 0;
    virtual void del_string_array(pal::char_t** value) = 0;
    virtual int get_int_value(pal::char_t* name) = 0;
};

class corehost_init_t : host_interface_t
{
private:
    int m_version;
    std::vector<pal::string_t> m_probe_paths;
    const pal::string_t m_deps_file;
    const pal::string_t m_fx_dir;
    host_mode_t m_host_mode;
    const runtime_config_t* m_runtime_config;
public:
    corehost_init_t(
        const pal::string_t& deps_file,
        const std::vector<pal::string_t>& probe_paths,
        const pal::string_t& fx_dir,
        const host_mode_t mode,
        const runtime_config_t* runtime_config)
        : m_fx_dir(fx_dir)
        , m_runtime_config(runtime_config)
        , m_deps_file(deps_file)
        , m_probe_paths(probe_paths)
        , m_host_mode(mode)
    {
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

    static void get_config_string(const pal::char_t* name, pal::string_t* val)
    {
        pal::char_t* str = input->get_string_value(name);
        val->assign(str);
        input->del_string_value(str);
    }

    static void get_config_array(const pal::char_t* name, std::vector<pal::string_t>* val)
    {
        char** values;
        int size = input->get_string_array(name, &values);
        for (int i = 0; i < size; ++i)
        {
            val->push_back(values[i]);
        }
        input->del_string_array(values);
    }

    static void init(host_interface_t* input, hostpolicy_init_t* init)
    {
        init->host_mode = (host_mode_t) input->get_int_value(_X("host.mode"));
        init->patch_roll_forward = input->get_int_value(_X("host.patch_roll_forward"));
        init->prerelease_roll_forward = input->get_int_value(_X("host.prerelease_roll_forward"));
        init->is_portable = input->get_int_value(_X("host.is_portable"));

        get_config_string(input, _X("host.app.deps_file"), &init->deps_file);
        get_config_string(input, _X("host.fx.dir"), &init->fx_dir);
        get_config_string(input, _X("host.fx.name"), &init->fx_name);

        get_config_string_array(input, _X("host.app.probe_paths"), &init->probe_paths);
        get_config_string_array(input, _X("coreclr.config.values"), &init->cfg_values);
        get_config_string_array(input, _X("coreclr.config.keys"), &init->cfg_keys);
    }
};

pal::string_t get_runtime_config_from_file(const pal::string_t& file, pal::string_t* dev_config_file);
host_mode_t detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_dir = nullptr);

void try_patch_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);
void try_prerelease_roll_forward_in_dir(const pal::string_t& cur_dir, const fx_ver_t& start_ver, pal::string_t* max_str);

#endif // __LIBHOST_H__

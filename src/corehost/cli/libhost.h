// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __LIBHOST_H__
#define __LIBHOST_H__

#define LIBHOST_NAME MAKE_LIBNAME("hostpolicy")

enum HostMode
{
    Invalid = 0,
    Muxer,
    Standalone,
    Framework
};

enum LibHostStatusCode
{
    // 0x80 prefix to distinguish from corehost main's error codes.
    InvalidArgFailure        = 0x81,
    CoreClrResolveFailure    = 0x82,
    CoreClrBindFailure       = 0x83,
    CoreClrInitFailure       = 0x84,
    CoreClrExeFailure        = 0x85,
    ResolverInitFailure      = 0x86,
    ResolverResolveFailure   = 0x87,
    LibHostCurExeFindFailure = 0x88,
    LibHostInitFailure       = 0x89,
    LibHostMuxFailure        = 0x90,
    LibHostExecModeFailure   = 0x91,
    LibHostSdkFindFailure    = 0x92,
    LibHostInvalidArgs       = 0x93,
};

class runtime_config_t;

class corehost_init_t
{
    const pal::string_t m_fx_dir;
    const runtime_config_t* m_runtime_config;
public:
    corehost_init_t(const pal::string_t& fx_dir, const runtime_config_t* runtime_config)
        : m_fx_dir(fx_dir)
        , m_runtime_config(runtime_config)
    {
    }

    const pal::string_t& fx_dir()
    {
        return m_fx_dir;
    }

    const runtime_config_t* runtime_config()
    {
        return m_runtime_config;
    }
};

pal::string_t get_runtime_config_json(const pal::string_t& app_path);
HostMode detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_dir = nullptr);

#endif // __LIBHOST_H__
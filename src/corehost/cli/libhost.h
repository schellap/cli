// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

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

class libhost_init_t
{
public:
    libhost_init_t(const HostMode mode)
        : m_mode(mode)
    {
    }

    HostMode get_host_mode() const
    {
        return m_mode;
    }

    HostMode m_mode;
};
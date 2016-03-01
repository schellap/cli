// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"

struct fx_ver_t
{
    int major;
    int minor;
    int patch;
    pal::string_t pre;
    pal::string_t build;
public:
    fx_ver_t(int major, int minor, int patch);
    fx_ver_t(int major, int minor, int patch, const pal::string_t& pre);
    fx_ver_t(int major, int minor, int patch, const pal::string_t& pre, const pal::string_t& build);

    pal::string_t as_str();

    bool operator ==(const fx_ver_t& b) const;
    bool operator !=(const fx_ver_t& b) const;
    bool operator <(const fx_ver_t& b) const;
    bool operator >(const fx_ver_t& b) const;

    static bool parse(const pal::string_t& ver, fx_ver_t* fx_ver, bool parse_only_production = false);
    static int compare(const fx_ver_t&a, const fx_ver_t& b, bool ignore_build = false);
};


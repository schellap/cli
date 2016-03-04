// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

class fx_muxer_t
{
public:
    fx_muxer_t(const pal::char_t* first_arg, const pal::string_t& install_dir);
    bool resolve_framework_dir(pal::string_t* resolved_path) const;
};


struct fx_ver_t
{
    int major;
    int minor;
    int patch;
    pal::string_t pre;
public:
    fx_ver_t(int major, int minor, int patch, const pal::string_t& pre)
        : major(major)
        , minor(minor)
        , patch(patch)
        , pre(pre)
    {
    }
    fx_ver_t(int major, int minor, int patch)
        : fx_ver_t(major, minor, patch, _X(""))
    {
    }

    static bool parse(const pal::string_t& ver, fx_ver_t* fx_ver);

    bool operator ==(const fx_ver_t& b) const;
    bool operator !=(const fx_ver_t& b) const;
    bool operator <(const fx_ver_t& b) const;
    bool operator >(const fx_ver_t& b) const;
    static int compare(const fx_ver_t&a, const fx_ver_t& b);
};


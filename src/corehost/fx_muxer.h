// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

class fx_muxer_t
{
public:
    static bool create_fx_muxer(int argc, const pal::char_t* argv[]);
    bool resolve_path(pal::string_t* resolved_path);
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


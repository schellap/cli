// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "utils.h"
#include "fx_muxer.h"

static bool fx_ver_t::parse(const pal::string_t& ver, fx_ver_t* fx_ver)
{
    size_t maj_sep = ver.find(_X('.'));
    if (maj_sep == pal::string_t::npos)
    {
        return false;
    }
    size_t min_sep = ver.find(maj_sep + 1, _X('.'));
    if (min_sep == pal::string_t::npos)
    {
        return false;
    }

    size_t maj_start = 0;
    size_t min_start = maj_sep + 1;
    size_t pat_start = min_sep + 1;
    size_t pat_sep = ver.find(min_sep + 1, _X('.'));
    if (pat_sep == pal::string_t::npos)
    {
        *fx_ver = fx_ver_t(
            std::stoi(ver.substr(maj_start, maj_sep)),
            std::stoi(ver.substr(min_start, min_sep - min_start)),
            std::stoi(ver.substr(pat_start, pat_sep - pat_start)));
        return true;
    }
    else if (ver.size() - pat_sep > 1)
    {
        *fx_ver = fx_ver_t(
            std::stoi(ver.substr(maj_start, maj_sep)),
            std::stoi(ver.substr(min_start, min_sep - min_start)),
            std::stoi(ver.substr(pat_start, pat_sep - pat_start)),
            ver.substr(pat_sep + 1));
        return true;
    }
    return false;
}

bool fx_ver_t::operator ==(const fx_ver_t& b) const
{
    return compare(*this, b) == 0;
}

bool fx_ver_t::operator !=(const fx_ver_t& b) const
{
    return !operator ==(b);
}

bool fx_ver_t::operator <(const fx_ver_t& b) const
{
    return compare(*this, b) < 0;
}

bool fx_ver_t::operator >(const fx_ver_t& b) const
{
    return compare(*this, b) > 0;
}

static int fx_ver_t::compare(const fx_ver_t&a, const fx_ver_t& b)
{
    return
    (a.major == b.major)
        ? ((a.minor == b.minor)
            ? ((a.patch == b.patch)
                ? (pal::strcasecmp(a.pre.c_str(), b.pre.c_str()))
                : (a.patch > b.patch ? 1 : -1))
            : (a.minor > b.minor ? 1 : -1))
        : ((a.major > b.major) ? 1 : -1)
        ;
}

void fx_muxer_t::determine_sdk_location(const pal::string_t& own_dir, pal::string_t* cli_sdk)
{
    pal::string_t cwd;
    pal::string_t global;
    if (pal::getcwd(&cwd))
    {
        for (pal::string_t cur_dir = cwd; true; cur_dir = parent)
        {
            pal::string_t file = cur_dir;
            append_path(&file, _X("global.json"));
            if (pal::file_exists(file))
            {
                global = file;
                break;
            }
            pal::string_t parent = get_directory(cur_dir);
            if (parent.empty() || parent.size() == cur_dir.size())
            {
                break;
            }
        }
    }
    pal::string_t retval;
    if (!global.empty())
    {
        pal::string_t sdk_path = own_dir;
        append_path(&sdk_path, _X("sdk"));
        pal::string_t cli_version = global_json_t::get_cli_version(global);
        if (!cli_version.empty())
        {
            append_path(&sdk_path, cli_version);
            if (pal::directory_exists(sdk_path))
            {
                retval = sdk_path;
            }
        }
    }
    if (retval.empty())
    {
        pal::string_t sdk_path = own_dir;
        append_path(&sdk_path, _X("sdk"));
        std::vector<pal::string_t> versions;
        pal::readdir(sdk_path, &versions);

        pal::vector<sem_ver_t> sem_vers;
        sem_vers.reserve(versions.size());
        for (const auto& version : versions)
        {
            sem_vers.push_back(sem_ver_t(version));
        }
        auto iter = std::max_element(sem_vers.begin(), sem_vers.end());
        if (iter != sem_vers.end())
        {
            append_path(&sdk_path, iter->as_str().c_str());
            retval = sdk_path;
        }
    }
    return retval;
}

bool fx_muxer_t::create_fx_muxer(const pal::string_t& own_dir, int argc, const pal::char_t* argv[])
{
    pal::string_t managed_application = pal::string_t(argv[1]);
    if (ends_with(managed_application, _X(".dll"), false))
    {
        if (!pal::realpath(&managed_application))
        {
            return false;
        }
    }
    else
    {
        pal::string_t retval = determine_sdk_location(own_dir);
        if (retval.empty())
        {
            return false;
        }
    }
}


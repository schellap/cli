// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cassert>
#include "pal.h"
#include "fx_ver.h"

fx_ver_t::fx_ver_t(int major, int minor, int patch, const pal::string_t& pre, const pal::string_t& build)
    : major(major)
    , minor(minor)
    , patch(patch)
    , pre(pre)
    , build(build)
{
}

fx_ver_t::fx_ver_t(int major, int minor, int patch, const pal::string_t& pre)
	: fx_ver_t(major, minor, patch, pre, _X(""))
{
}

fx_ver_t::fx_ver_t(int major, int minor, int patch)
	: fx_ver_t(major, minor, patch, _X(""), _X(""))
{
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

pal::string_t fx_ver_t::as_str()
{
    pal::stringstream_t stream;
    stream << major << _X(".") << minor << _X(".") << patch;
    if (!pre.empty())
    {
        stream << _X(".") << pre;
    }
    if (!build.empty())
    {
        stream << _X("+") << build;
    }
    return stream.str();
}

/* static */
int fx_ver_t::compare(const fx_ver_t&a, const fx_ver_t& b, bool ignore_build)
{
    // compare(u.v.w-p+b, x.y.z-q+c)
    return
    (a.major == b.major)
        ? ((a.minor == b.minor)
            ? ((a.patch == b.patch)
                ? ((a.pre.empty() == b.pre.empty())
                    ? ((a.pre.empty())
                        ? (ignore_build ? 0 : a.build.compare(b.build))
                        : a.pre.compare(b.pre))
                    : a.pre.empty() ? 1 : -1)
                : (a.patch > b.patch ? 1 : -1))
            : (a.minor > b.minor ? 1 : -1))
        : ((a.major > b.major) ? 1 : -1)
        ;
}

/* static */
bool fx_ver_t::parse(const pal::string_t& ver, fx_ver_t* fx_ver, bool parse_only_production)
{
    size_t maj_start = 0;
    size_t maj_sep = ver.find(_X('.'));
    if (maj_sep == pal::string_t::npos)
    {
        return false;
    }
	auto crap = ver.find_first_not_of(_X("0123456789"), maj_start, maj_sep);
	if (ver.find_first_not_of(_X("0123456789"), maj_start, maj_sep) != pal::string_t::npos)
	{
		return false;
	}

	int major = std::stoi(ver.substr(maj_start, maj_sep));

    size_t min_start = maj_sep + 1;
    size_t min_sep = ver.find(min_start, _X('.'));
    if (min_sep == pal::string_t::npos)
    {
        return false;
    }
	if (ver.find_first_not_of(_X("0123456789"), min_start, min_sep) != pal::string_t::npos)
	{
		return false;
	}
    int minor = std::stoi(ver.substr(min_start, min_sep - min_start));

    size_t pat_start = min_sep + 1;
    size_t pat_sep = ver.find_first_of(_X(".+"), pat_start);
    if (pat_sep == pal::string_t::npos)
    {
		if (ver.find_first_not_of(_X("0123456789"), pat_start) != pal::string_t::npos)
		{
			return false;
		}

        int patch = std::stoi(ver.substr(pat_start));
        *fx_ver = fx_ver_t(major, minor, patch);
        return true;
    }

    if (parse_only_production)
    {
        // This is a prerelease or has build suffix.
        return false;
    }

	if (ver.find_first_not_of(_X("0123456789"), pat_start, pat_sep - pat_start) != pal::string_t::npos)
	{
		return false;
	}
    int patch = std::stoi(ver.substr(pat_start, pat_sep - pat_start));
    
    int last_sep = pat_sep;
    pal::string_t pre = _X("");
    if (ver[pat_sep] == _X('.'))
    {
        size_t pre_start = pat_sep + 1;
        size_t pre_sep = ver.find(pre_start, _X('+'));
		if (ver.find_first_not_of(_X("0123456789"), pre_start, pre_sep - pre_start) != pal::string_t::npos)
		{
			return false;
		}
		pre = std::stoi(ver.substr(pre_start, pre_sep - pre_start));
        if (pre_sep == pal::string_t::npos)
        {
            *fx_ver = fx_ver_t(major, minor, patch, pre);
            return true;
        }
        last_sep = pre_sep;
    }

    assert(ver[last_sep] == _X('+'));
    size_t build_start = last_sep + 1;
    *fx_ver = fx_ver_t(
        major,
        minor,
        patch,
        pre,
        ver.substr(build_start));
    return true;
}


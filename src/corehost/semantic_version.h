// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

struct SemVer
{
	int major;
	int minor;
	int patch;
	pal::string_t pre;
    pal::string_t build;
public:
	SemVer(int major, int minor, int patch, const pal::string_t& pre, const pal::string_t& build)
		: major(major)
		, minor(minor)
		, patch(patch)
		, pre(pre)
        , build(build)
	{
	}

	bool operator ==(const SemVer& b) const
	{
        return compare(*this, b) == 0;
	}
	bool operator !=(const SemVer& b) const
	{
		return !operator ==(b);
	}
	bool operator <(const SemVer& b) const
	{
        return compare(*this, b) < 0;
	}
    bool operator >(const SemVer& b) const
    {
        return compare(*this, b) > 0;
    }

    static int compare(const SemVer&a, const SemVer& b)
    {
        // compare(u.v.w-p+b, x.y.z-q+c)
        return
        (a.major == b.major)
            ? ((a.minor == b.minor)
                ? ((a.patch == b.patch)
                    ? ((a.pre.empty() == b.pre.empty())
                        ? ((a.pre.empty())
                            // SemVer says build metadata should be ignored, but we will just strcmp them.
                            ? a.build.compare(b.build)
                            : a.pre.compare(b.pre))
                        : a.pre.empty() ? 1 : -1)
                    : (a.patch > b.patch ? 1 : -1))
                : (a.minor > b.minor ? 1 : -1))
            : ((a.major > b.major) ? 1 : -1)
            ;
    }
};


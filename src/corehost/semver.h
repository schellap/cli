// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

struct sem_ver_t
{
	int major;
	int minor;
	int patch;
	pal::string_t pre;
    pal::string_t build;
public:
	sem_ver_t(int major, int minor, int patch, const pal::string_t& pre, const pal::string_t& build);
    sem_ver_t(const pal::string_t& str);

	bool operator ==(const sem_ver_t& b) const;
	bool operator !=(const sem_ver_t& b) const;
	bool operator <(const sem_ver_t& b) const;
    bool operator >(const sem_ver_t& b) const;
    static int compare(const sem_ver_t&a, const sem_ver_t& b, bool ignore_build = false);
};


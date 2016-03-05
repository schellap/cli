// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "utils.h"
#include "trace.h"
#include "libhost.h"

HostMode detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* p_own_dir)
{
    pal::string_t own_path;
    if (!pal::get_own_executable_path(&own_path) || !pal::realpath(&own_path))
    {
        trace::error(_X("Failed to locate current executable"));
        return HostMode::Invalid;
    }

	pal::string_t own_name = get_filename(own_path);
	pal::string_t own_dir = get_directory(own_path);
    if (p_own_dir)
    {
        p_own_dir->assign(own_dir);
    }

	if (coreclr_exists_in_dir(own_dir))
	{
		pal::string_t own_deps_json = own_dir;
		pal::string_t own_deps_filename = strip_file_ext(own_name) + _X(".deps.json");
		append_path(&own_deps_json, own_deps_filename.c_str());
		return (pal::file_exists(own_deps_json)) ? HostMode::Standalone : HostMode::Framework;
	}
	else
	{
		return HostMode::Muxer;
	}
}


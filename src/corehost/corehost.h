// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"

enum HostMode
{
	Invalid = 0,
	Muxer,
	Standalone,
	Framework
};

HostMode detect_operating_mode(const int argc, const pal::char_t* argv[], pal::string_t* own_path, pal::string_t* own_dir, pal::string_t* own_name);

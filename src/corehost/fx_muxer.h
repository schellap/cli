// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "corehost.h"
#include "semver.h"

class fx_muxer_t
{
public:
    fx_muxer_t(const int argc, const pal::char_t* argv[]);
	StatusCode execute();
private:
    bool resolve_framework_dir(pal::string_t* resolved_path) const;
	bool determine_sdk_dotnet_path(const pal::string_t& own_dir, pal::string_t* cli_sdk);
};


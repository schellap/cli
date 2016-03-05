// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

struct fx_ver_t;

class fx_muxer_t
{
private:
    static int execute(const int argc, const pal::char_t* argv[]);
    static pal::string_t get_cli_version(const pal::string_t& global);
    static bool determine_sdk_dotnet_path(const pal::string_t& own_dir, pal::string_t* cli_sdk);
};


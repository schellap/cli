// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"

class runtime_config_t
{
public:
	runtime_config_t(const pal::string_t& path);
    bool is_valid() { return m_valid; }
	const pal::string_t& get_fx_version();
	bool get_fx_roll_fwd();
private:
	bool ensure_parsed();
    pal::string_t m_fx_ver;
	bool m_fx_roll_fwd;
    pal::string_t m_path;
    bool m_valid;
};

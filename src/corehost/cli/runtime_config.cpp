// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "utils.h"
#include "cpprest/json.h"
#include "runtime_config.h"
#include <cassert>

typedef web::json::value json_value;

runtime_config_t::runtime_config_t(const pal::string_t& path)
	: m_fx_roll_fwd(true)
	, m_path(path)
{
	m_valid = ensure_parsed();
} 

void parse_fx(const json_value& opts, pal::string_t* name, pal::string_t* version, bool* roll_fwd)
{
	version->clear();
	*roll_fwd = true;
	if (opts.is_null())
	{
		return;
	}
	auto framework = opts.get(_X("framework"));
	if (framework.is_null())
	{
		return;
	}
	*name = framework.get(_X("name")).as_string();
	if (pal::to_lower(*name) != _X("netcoreapp"))
	{
		return;
	}
	*version = framework.get(_X("version")).as_string();

	auto value = framework.get(_X("autoRollForward"));
	if (value.is_null())
	{
		return;
	}

	*roll_fwd = value.as_bool();
}

bool runtime_config_t::ensure_parsed()
{
	pal::string_t retval;
	if (!pal::file_exists(m_path))
	{
		// Not existing is not an error.
		return true;
	}

	pal::ifstream_t file(m_path);
	if (!file.good())
	{
		return false;
	}

	try
	{
		const auto& json = json_value::parse(file);
		const auto& opts = json.get(_X("runtimeOptions"));
		parse_fx(opts, &m_fx_name, &m_fx_ver, &m_fx_roll_fwd);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

const pal::string_t& runtime_config_t::get_fx_name() const
{
	assert(m_valid);
	return m_fx_name;
}

const pal::string_t& runtime_config_t::get_fx_version() const
{
	assert(m_valid);
    return m_fx_ver;
}

bool runtime_config_t::get_fx_roll_fwd() const
{
	assert(m_valid);
	return m_fx_roll_fwd;
}
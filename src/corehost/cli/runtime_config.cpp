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
    , m_portable(false)
    , m_gc_server(_X("0"))
{
    m_valid = ensure_parsed();
} 

struct property_converter_t
{
    const pal::char_t* property;
    std::function<pal::string_t(const json_value& value)> convert;
   
    pal::string_t int_to_str(const json_value& value)
    {
        return pal::to_string(value.as_integer());
    };

    pal::string_t bool_to_str(const json_value& value)
    {
        return value.is_bool() ? pal::to_string((int) value.as_bool()) : int_to_str(value);
    };
};

const property_converter_t property_converters[] = {
    { _X("System.GC.Server"), property_converter_t::bool_to_str },
    { _X("System.GC.Concurrent"), property_converter_t::bool_to_str },
   
    { _X("System.Threading.ThreadPool.MinThreads"), property_converter_t::int_to_str },
    { _X("System.Threading.ThreadPool.MaxThreads"), property_converter_t::int_to_str },
    { _X("System.Threading.Thread.UseAllCpuGroups"), property_converter_t::int_to_str }
};

bool runtime_config_t::parse_opts(const json_value& opts)
{
    if (opts.is_null())
    {
        return true;
    }

    const auto& opts_obj = opts.as_object();
    
    for (const auto& pc : property_converters)
    {
        auto property = opts_obj.find(pc.property);
        if (property != opts_obj.end())
        {
            m_properties[property->first] = pc.convert(property->second);
        }
    }

    auto framework =  opts_obj.find(_X("framework"));
    if (framework == opts_obj.end())
    {
        return true;
    }

    m_portable = true;

    const auto& fx_obj = framework->second.as_object();
    m_fx_name = fx_obj.at(_X("name")).as_string();
    m_fx_ver = fx_obj.at(_X("version")).as_string();

    auto value = fx_obj.find(_X("rollForward"));
    if (value == fx_obj.end())
    {
        return true;
    }

    m_fx_roll_fwd = value->second.as_bool();
    return true;
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
        const auto root = json_value::parse(file);
        const auto& json = root.as_object();
        const auto iter = json.find(_X("runtimeOptions"));
        if (iter != json.end())
        {
            parse_opts(iter->second);
        }
    }
    catch (...)
    {
        return false;
    }
    return true;
}

const pal::string_t& runtime_config_t::get_gc_server() const
{
    assert(m_valid);
    if (m_properties.count(_X("System.GC.Server")))
    {
        m_gc_server = m_properties[_X("System.GC.Server")];
    }
    return m_gc_server;
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

bool runtime_config_t::get_portable() const
{
    return m_portable;
}

// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef __DEPS_FORMAT_H_
#define __DEPS_FORMAT_H_

#include <iostream>
#include <vector>
#include "pal.h"
#include "deps_entry.h"
#include "cpprest/json.h"

class deps_json_t
{
    typedef web::json::value json_value;

public:
    deps_json_t(const pal::string_t& deps_path)
        : m_valid(load(deps_path))
    {
    }
    bool load(const pal::string_t& deps_path);
    bool is_valid() { return m_valid; }
    const std::vector<deps_entry_t>& get_entries() { return m_deps_entries; }

private:
    bool load_standalone(const json_value& json, const pal::string_t& target_name);
    bool load_portable(const json_value& json, const pal::string_t& target_name);

    static const std::array<pal::char_t*, 3> s_known_types;

    std::vector<deps_entry_t> m_deps_entries;
    bool m_valid;
};

class deps_text_t
{
public:
    deps_text_t(const pal::string_t& deps_path)
        : m_valid(load(deps_path))
    {
    }

    bool load(const pal::string_t& deps_path);
    bool is_valid() { return m_valid; }
    const std::vector<deps_entry_t>& get_entries() { return m_deps_entries; }

private:
    std::vector<deps_entry_t> m_deps_entries;
    bool m_valid;
};

#endif // __DEPS_FORMAT_H_
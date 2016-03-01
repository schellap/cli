// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "deps_format.h"
#include "utils.h"
#include "trace.h"
#include "cpprest/json.h"
#include <unordered_set>
#include <tuple>
#include <array>
#include <iterator>

namespace
{
// -----------------------------------------------------------------------------
// Read a single field from the deps entry
//
// Parameters:
//    line  - A deps file entry line
//    buf   - The temporary buffer to use while parsing (with size to contain "line")
//    ofs   - The offset that this method will read from "line" on invocation and
//            the offset that has been consumed by this method upon successful exit
//    field - The current field read from the line
//
// Assumption:
//    The line should be in a CSV format, with commas separating the fields.
//    The fields themselves will be quoted. The escape character is '\\'
//
// Returns:
//    True if parsed successfully. Else, false
//
// Note:
//      Callers cannot call with the same "line" upon an unsuccessful exit.
bool read_field(const pal::string_t& line, pal::char_t* buf, unsigned* ofs, pal::string_t* field)
{
    unsigned& offset = *ofs;
    pal::string_t& value_recv = *field;

    // The first character should be a '"'
    if (line[offset] != '"')
    {
        trace::error(_X("Error reading TPA file"));
        return false;
    }
    offset++;

    auto buf_offset = 0;

    // Iterate through characters in the string
    for (; offset < line.length(); offset++)
    {
        // Is this a '\'?
        if (line[offset] == '\\')
        {
            // Skip this character and read the next character into the buffer
            offset++;
            buf[buf_offset] = line[offset];
        }
        // Is this a '"'?
        else if (line[offset] == '\"')
        {
            // Done! Advance to the pointer after the input
            offset++;
            break;
        }
        else
        {
            // Take the character
            buf[buf_offset] = line[offset];
        }
        buf_offset++;
    }
    buf[buf_offset] = '\0';
    value_recv.assign(buf);

    // Consume the ',' if we have one
    if (line[offset] == ',')
    {
        offset++;
    }
    return true;
}
} // end of anonymous namespace
  
// -----------------------------------------------------------------------------
// Load the deps file and parse its "entry" lines which contain the "fields" of
// the entry. Populate an array of these entries.
//
bool deps_text_t::load(const pal::string_t& deps_path)
{
    // If file doesn't exist, then assume parsed.
    if (!pal::file_exists(deps_path))
    {
        return true;
    }

    // Somehow the file stream could not be opened. This is an error.
    pal::ifstream_t file(deps_path);
    if (!file.good())
    {
        return false;
    }

    // Parse the "entry" lines of the deps file.
    std::string stdline;
    while (std::getline(file, stdline))
    {
        pal::string_t line;
        pal::to_palstring(stdline.c_str(), &line);

        deps_entry_t entry;
        pal::string_t is_serviceable;
        pal::string_t* fields[] = {
            &entry.library_type,
            &entry.library_name,
            &entry.library_version,
            &entry.library_hash,
            &entry.asset_type,
            &entry.asset_name,
            &entry.relative_path,
            // TODO: Add when the deps file support is enabled.
            // &is_serviceable
        };

        std::vector<pal::char_t> buf(line.length());

        for (unsigned i = 0, offset = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        {
            if (!(read_field(line, buf.data(), &offset, fields[i])))
            {
                return false;
            }
        }

        // Serviceable, if not false, default is true.
        entry.is_serviceable = pal::strcasecmp(is_serviceable.c_str(), _X("false")) != 0;

        // TODO: Deps file does not follow spec. It uses '\\', should use '/'
        replace_char(&entry.relative_path, _X('\\'), _X('/'));

        m_deps_entries.push_back(entry);

        trace::verbose(_X("Added deps entry [%d] [%s, %s, %s]"), m_deps_entries.size() - 1, entry.library_name.c_str(), entry.library_version.c_str(), entry.relative_path.c_str());
    }
    
    return true;
}


// -----------------------------------------------------------------------------
// Load the deps file and parse its "entry" lines which contain the "fields" of
// the entry. Populate an array of these entries.
//
bool deps_json_t::load(const pal::string_t& deps_path)
{
    // If file doesn't exist, then assume parsed.
    if (!pal::file_exists(deps_path))
    {
        return true;
    }

    // Somehow the file stream could not be opened. This is an error.
    pal::ifstream_t file(deps_path);
    if (!file.good())
    {
        return false;
    }

    std::unordered_map<pal::string_t, std::array<std::vector<pal::string_t>, 3>> runtime_assets;
    std::array<pal::char_t*, 3> types = { _X("runtime"), _X("resources"), _X("native") };

    const web::json::value& json = web::json::value::parse(file);
    const web::json::object& targets = json.at(_X("targets")).as_object();
    for (const auto& target : targets)
    {
        if (target.first.rfind(_X("/")) == utility::string_t::npos)
        {
            continue;
        }

        for (const auto& package : target.second.as_object())
        {
            const web::json::object& properties = package.second.as_object();

            if (properties.at(_X("type")).as_string() != _X("package"))
            {
                continue;
            }

            for (int i = 0; i < types.size(); ++i)
            {
                auto iter = properties.find(types[i]);
                if (iter != properties.end())
                {
                    for (const auto& file : iter->second.as_object())
                    {
                        runtime_assets[package.first][i].push_back(file.first);
                    }
                }
            }
        }
    }

    const web::json::object& libraries = json.at(_X("libraries")).as_object();
    for (const auto& library : libraries)
    {
        auto iter = runtime_assets.find(library.first);
        if (iter == runtime_assets.end())
        {
            continue;
        }
        if (library.second.at(_X("type")).as_string() != _X("package"))
        {
            continue;
        }
        const auto& properties = library.second.as_object();
        auto hash = properties.find(_X("sha512"));
        auto serviceable = properties.find(_X("serviceable"));
        for (int i = 0; i < types.size(); ++i)
        {
            for (const auto& file : iter->second[i])
            {
                deps_entry_t entry;
                size_t pos = library.first.find(_X("/"));
                entry.library_name = library.first.substr(0, pos);
                entry.library_version = library.first.substr(pos + 1);
                entry.library_hash = hash != properties.end() ? hash->second.as_string() : _X("");
                entry.library_type = _X("package");
                entry.asset_name = get_filename_without_extension(file);
                entry.asset_type = types[i];
                entry.relative_path = file;
                entry.is_serviceable = (serviceable == properties.end()) ||
                    (pal::strcasecmp(serviceable->second.as_string().c_str(), _X("false")) != 0);
                m_deps_entries.push_back(entry);
            }
        }
    }
}
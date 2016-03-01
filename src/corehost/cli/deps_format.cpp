// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "deps_format.h"
#include "utils.h"
#include "trace.h"
#include "cpprest/json.h"

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

    web::json::value json = web::json::value::parse(file);

    for (auto iter = json.as_object().cbegin(); iter != json.as_object().cend(); ++iter)
    {
        auto k = iter->first;
        auto v = iter->second;
    }
}
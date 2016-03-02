// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "deps_format.h"
#include "utils.h"
#include "trace.h"
#include <unordered_set>
#include <tuple>
#include <array>
#include <iterator>

const std::array<pal::char_t*, deps_json_t::NUM_ASSET_TYPES> deps_json_t::s_known_asset_types = { _X("runtime"), _X("resources"), _X("native") };

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
    
    deps_json_t json(_X("H:\\code\\deps.json"));
    return json.is_valid();
}

bool deps_json_t::load_portable(const json_value& json, const pal::string_t& target_name, const rid_fallback_graph_t& rid_fallback_graph)
{
    portable_assets_t runtime_assets;

    for (const auto& package : json.at(_X("targets")).at(target_name).as_object())
    {
        const auto& targets = package.second.as_object();
        auto iter = targets.find(_X("subtargets"));
        if (iter == targets.end())
        {
            continue;
        }
        const auto& asset_types = iter->second.as_object();
        for (int i = 0; i < s_known_asset_types.size(); ++i)
        {
            const auto& iter = asset_types.find(s_known_asset_types[i]);
            if (iter != asset_types.end())
            {
                for (const auto& file : iter->second.as_object())
                {
                    const auto& rid = file.second.as_object().at(_X("rid")).as_string();
                    runtime_assets[package.first][rid][i].push_back(file.first);
                }
            }
        }
    }

    if (!perform_rid_fallback(&runtime_assets, rid_fallback_graph))
    {
        return false;
    }
}

bool deps_json_t::perform_rid_fallback(portable_assets_t* portable_assets, const rid_fallback_graph_t& rid_fallback_graph)
{
    pal::string_t host_rid = get_own_rid();
    for (auto& package : *portable_assets)
    {
		pal::string_t matched_rid = package.second.count(host_rid) ? host_rid : _X("");
		if (matched_rid.empty())
		{
			if (rid_fallback_graph.count(host_rid) == 0)
			{
				trace::error(_X("Did not find fallback rids for package %s for the host rid %s"), package.first.c_str(), host_rid.c_str());
				return false;
			}
			const auto& fallback_rids = rid_fallback_graph.find(host_rid)->second;
			auto iter = std::find_if(fallback_rids.begin(), fallback_rids.end(), [&package](const pal::string_t& rid) {
				return package.second.count(rid);
			});
			if (iter == fallback_rids.end() || (*iter).empty())
			{
				trace::error(_X("Did not find a matching fallback rid for package %s for the host rid %s"), package.first.c_str(), host_rid.c_str());
				return false;
			}
			matched_rid = *iter;
		}
    }
    return false;
}

bool deps_json_t::load_standalone(const json_value& json, const pal::string_t& target_name)
{
    standalone_assets_t standalone_assets;
    for (const auto& package : json.at(_X("targets")).at(target_name).as_object())
    {
        if (package.second.at(_X("type")).as_string() != _X("package"))
        {
            continue;
        }

        const auto& asset_types = package.second.as_object();
        for (int i = 0; i < s_known_asset_types.size(); ++i)
        {
            const auto& iter = asset_types.find(s_known_asset_types[i]);
            if (iter != asset_types.end())
            {
                for (const auto& file : iter->second.as_object())
                {
                    standalone_assets[package.first][i].push_back(file.first);
                }
            }
        }
    }

    const auto& libraries = json.at(_X("libraries")).as_object();
    for (const auto& library : libraries)
    {
        if (library.second.at(_X("type")).as_string() != _X("package"))
        {
            continue;
        }
        const auto iter = standalone_assets.find(library.first);
        if (iter == standalone_assets.end())
        {
            continue;
        }

        const auto& properties = library.second.as_object();

        const pal::string_t& hash = properties.at(_X("sha512")).as_string();
        bool serviceable = properties.at(_X("serviceable")).as_bool();

        for (int i = 0; i < s_known_asset_types.size(); ++i)
        {
            for (const auto& rel_path : iter->second[i])
            {
                auto asset_name = get_filename_without_ext(rel_path);
                if (ends_with(asset_name, _X(".ni")))
                {
                    asset_name = strip_file_ext(asset_name);
                }

                deps_entry_t entry;
                size_t pos = library.first.find(_X("/"));
                entry.library_name = library.first.substr(0, pos);
                entry.library_version = library.first.substr(pos + 1);
                entry.library_type = _X("package");
                entry.library_hash = hash;
                entry.asset_name = asset_name;
                entry.asset_type = s_known_asset_types[i];
                entry.relative_path = rel_path;
                entry.is_serviceable = serviceable;

                // TODO: Deps file does not follow spec. It uses '\\', should use '/'
                replace_char(&entry.relative_path, _X('\\'), _X('/'));

                m_deps_entries.push_back(entry);
            }
        }
    }

    const auto& json_object = json.as_object();
    const auto iter = json_object.find(_X("runtimes"));
    if (iter != json_object.end())
    {
        for (const auto& rid : iter->second.as_object())
        {
            auto& vec = m_rid_fallback_graph[rid.first];
            for (const auto& fallback : rid.second.as_array())
            {
                vec.push_back(fallback.as_string());
            }
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
// Load the deps file and parse its "entry" lines which contain the "fields" of
// the entry. Populate an array of these entries.
//
bool deps_json_t::load(const pal::string_t& deps_path, const rid_fallback_graph_t& rid_fallback_graph)
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

    const auto& json = json_value::parse(file);

    const auto& runtime_target = json.at(_X("runtimeTarget")).as_object();
    bool portable = runtime_target.at(_X("portable")).as_bool();
    const pal::string_t& name = runtime_target.at(_X("name")).as_string();

    return (portable) ? load_portable(json, name, rid_fallback_graph) : load_standalone(json, name);
}
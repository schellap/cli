// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "deps_format.h"
#include "utils.h"
#include "trace.h"
#include <unordered_set>
#include <tuple>
#include <array>
#include <iterator>
#include <cassert>
#include <functional>

const std::array<pal::char_t*, deps_entry_t::asset_types::count> deps_json_t::s_known_asset_types = {
	_X("runtime"), _X("resources"), _X("native") };

bool deps_json_t::load_portable(const json_value& json, const pal::string_t& target_name, const rid_fallback_graph_t& rid_fallback_graph)
{
    portable_assets_t portable_targets;

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
                    portable_targets[package.first][rid][i].push_back(file.first);
                }
            }
        }
    }

    if (!perform_rid_fallback(&portable_targets, rid_fallback_graph))
    {
        return false;
    }

    reconcile_libraries_with_targets(
        json,
        [&portable_targets](const pal::string_t& package) -> bool {
            return portable_targets.find(package) != portable_targets.end();
        },
        [&portable_targets](const pal::string_t& package, int type_index) -> const std::vector<pal::string_t>& {
            return portable_targets[package].begin()->second[type_index];
        });

    return true;
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
        assert(!matched_rid.empty());
        for (auto iter = package.second.begin(); iter != package.second.end(); ++iter)
        {
            if (iter->first != matched_rid)
            {
                package.second.erase(iter);
            }
        }
    }
    return true;
}

bool deps_json_t::load_standalone(const json_value& json, const pal::string_t& target_name)
{
    standalone_assets_t standalone_targets;
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
                    standalone_targets[package.first][i].push_back(file.first);
                }
            }
        }
    }

    reconcile_libraries_with_targets(
        json,
        [&standalone_targets](const pal::string_t& package) -> bool {
            return standalone_targets.count(package);
        },
        [&standalone_targets](const pal::string_t& package, int type_index) -> const std::vector<pal::string_t>& {
            return standalone_targets[package][type_index];
        });

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

void deps_json_t::reconcile_libraries_with_targets(
    const json_value& json,
    const std::function<bool(const pal::string_t&)>& library_exists_fn,
    const std::function<const std::vector<pal::string_t>&(const pal::string_t&, int)>& get_rel_paths_by_asset_type_fn)
{
    const auto& libraries = json.at(_X("libraries")).as_object();
    for (const auto& library : libraries)
    {
        if (library.second.at(_X("type")).as_string() != _X("package"))
        {
            continue;
        }
        if (library_exists_fn(library.first))
        {
            continue;
        }

        const auto& properties = library.second.as_object();

        const pal::string_t& hash = properties.at(_X("sha512")).as_string();
        bool serviceable = properties.at(_X("serviceable")).as_bool();

        for (int i = 0; i < s_known_asset_types.size(); ++i)
        {
            for (const auto& rel_path : get_rel_paths_by_asset_type_fn(library.first, i))
            {
                auto asset_name = get_filename_without_ext(rel_path);
                if (ends_with(asset_name, _X(".ni"), false))
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

                m_deps_entries[i].push_back(entry);

                if (i == deps_entry_t::asset_types::native &&
                    entry.asset_name == LIBCORECLR_FILENAME)
                {
                    m_coreclr_index = m_deps_entries[i].size() - 1;
                    trace::verbose(_X("Found coreclr from deps %d [%s, %s, %s]"),
                        m_coreclr_index,
                        entry.library_name.c_str(),
                        entry.library_version.c_str(),
                        entry.relative_path.c_str());
                }

            }
        }
    }
}

// -----------------------------------------------------------------------------
// Load the deps file and parse its "entry" lines which contain the "fields" of
// the entry. Populate an array of these entries.
//
bool deps_json_t::load(bool portable, const pal::string_t& deps_path, const rid_fallback_graph_t& rid_fallback_graph)
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

    try
    {
        const auto& json = json_value::parse(file);

        const auto& runtime_target = json.at(_X("runtimeTarget"));
        const pal::string_t& name = runtime_target.as_string();

        return (portable) ? load_portable(json, name, rid_fallback_graph) : load_standalone(json, name);
    }
    catch (...)
    {
        return false;
    }
}
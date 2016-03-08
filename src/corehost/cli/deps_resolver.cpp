// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <set>
#include <functional>
#include <cassert>

#include "trace.h"
#include "deps_format.h"
#include "deps_resolver.h"
#include "utils.h"

namespace
{
// -----------------------------------------------------------------------------
// A uniqifying append helper that doesn't let two entries with the same
// "asset_name" be part of the "output" paths.
//
void add_tpa_asset(
    const pal::string_t& asset_name,
    const pal::string_t& asset_path,
    std::set<pal::string_t>* items,
    pal::string_t* output)
{
    if (items->count(asset_name))
    {
        return;
    }

    trace::verbose(_X("Adding tpa entry: %s"), asset_path.c_str());

    // Workaround for CoreFX not being able to resolve sym links.
    pal::string_t real_asset_path = asset_path;
    pal::realpath(&real_asset_path);
    output->append(real_asset_path);

    output->push_back(PATH_SEPARATOR);
    items->insert(asset_name);
}

// -----------------------------------------------------------------------------
// Add mscorlib from the CLR directory. Even if CLR is serviced, we should pick
// mscorlib from the CLR directory. If mscorlib could not be found in the CLR
// location, then leave it to the CLR to pick the right mscorlib.
//
void add_mscorlib_to_tpa(const pal::string_t& clr_dir, std::set<pal::string_t>* items, pal::string_t* output)
{
    pal::string_t mscorlib_ni_path = clr_dir + DIR_SEPARATOR + _X("mscorlib.ni.dll");
    if (pal::file_exists(mscorlib_ni_path))
    {
        add_tpa_asset(_X("mscorlib"), mscorlib_ni_path, items, output);
        return;
    }

    pal::string_t mscorlib_path = clr_dir + DIR_SEPARATOR + _X("mscorlib.dll");
    if (pal::file_exists(mscorlib_path))
    {
        add_tpa_asset(_X("mscorlib"), mscorlib_path, items, output);
        return;
    }
}

// -----------------------------------------------------------------------------
// A uniqifying append helper that doesn't let two "paths" to be identical in
// the "output" string.
//
void add_unique_path(
    const pal::string_t& type,
    const pal::string_t& path,
    std::set<pal::string_t>* existing,
    pal::string_t* output)
{
    // Resolve sym links.
    pal::string_t real = path;
    pal::realpath(&real);

    if (existing->count(real))
    {
        return;
    }

    trace::verbose(_X("Adding to %s path: %s"), type.c_str(), real.c_str());

    output->append(real);

    output->push_back(PATH_SEPARATOR);
    existing->insert(real);
}

} // end of anonymous namespace

// -----------------------------------------------------------------------------
// Load the deps file and parse its "entry" lines which contain the "fields" of
// the entry. Populate an array of these entries.
//
bool deps_resolver_t::load()
{
	return m_deps.is_valid() && m_fx_deps.is_valid();
}

// -----------------------------------------------------------------------------
// Load local assemblies by priority order of their file extensions and
// unique-fied  by their simple name.
//
void deps_resolver_t::get_dir_assemblies(
    const pal::string_t& dir,
    const pal::string_t& dir_name,
    std::unordered_map<pal::string_t, pal::string_t>* dir_assemblies)
{
    trace::verbose(_X("Adding files from %s dir %s"), dir_name.c_str(), dir.c_str());

    // Managed extensions in priority order, pick DLL over EXE and NI over IL.
    const pal::string_t managed_ext[] = { _X(".ni.dll"), _X(".dll"), _X(".ni.exe"), _X(".exe") };

    // List of files in the dir
    std::vector<pal::string_t> files;
    pal::readdir(dir, &files);

    for (const auto& ext : managed_ext)
    {
        for (const auto& file : files)
        {
            // Nothing to do if file length is smaller than expected ext.
            if (file.length() <= ext.length())
            {
                continue;
            }

            auto file_name = file.substr(0, file.length() - ext.length());
            auto file_ext = file.substr(file_name.length());

            // Ext did not match expected ext, skip this file.
            if (pal::strcasecmp(file_ext.c_str(), ext.c_str()))
            {
                continue;
            }

            // TODO: Do a case insensitive lookup.
            // Already added entry for this asset, by priority order skip this ext
            if (dir_assemblies->count(file_name))
            {
                trace::verbose(_X("Skipping %s because the %s already exists in %s assemblies"), file.c_str(), dir_assemblies->find(file_name)->second.c_str(), dir_name.c_str());
                continue;
            }

            // Add entry for this asset
            pal::string_t file_path = dir + DIR_SEPARATOR + file;
            trace::verbose(_X("Adding %s to %s assembly set from %s"), file_name.c_str(), dir_name.c_str(), file_path.c_str());
            dir_assemblies->emplace(file_name, file_path);
        }
    }
}

// -----------------------------------------------------------------------------
// Resolve coreclr directory from the deps file.
//
// Description:
//    Look for CoreCLR from the dependency list in the package cache and then
//    the packages directory.
//
pal::string_t deps_resolver_t::resolve_coreclr_dir(
    const pal::string_t& app_dir,
    const pal::string_t& package_dir,
    const pal::string_t& package_cache_dir)
{
	if (!m_fx_dir.empty())
	{
		trace::verbose(_X("Probing for CoreCLR in framework dir=[%s]"), m_fx_dir.c_str());
		return m_fx_dir;
	}

    // Package cache.
    trace::verbose(_X("Probing for CoreCLR in package cache=[%s]"), package_cache_dir.c_str());
    pal::string_t coreclr_cache;
    if (!package_cache_dir.empty() &&
        m_deps.has_coreclr_entry() &&
        m_deps.get_coreclr_entry().to_hash_matched_path(package_cache_dir, &coreclr_cache))
    {
        return get_directory(coreclr_cache);
    }

    // App dir.
    trace::verbose(_X("Probing for CoreCLR in app directory=[%s]"), app_dir.c_str());
    if (coreclr_exists_in_dir(app_dir))
    {
        return app_dir;
    }

    // Packages dir
    trace::verbose(_X("Probing for CoreCLR in packages=[%s]"), package_dir.c_str());
    pal::string_t coreclr_package;
    if (!package_dir.empty() &&
        m_deps.has_coreclr_entry() &&
        m_deps.get_coreclr_entry().to_full_path(package_dir, &coreclr_package))
    {
        return get_directory(coreclr_package);
    }

    // Use platform-specific search algorithm
    pal::string_t install_dir;
    if (pal::find_coreclr(&install_dir))
    {
        return install_dir;
    }

    return pal::string_t();
}

// -----------------------------------------------------------------------------
// Resolve the TPA list order.
//
// Description:
//    First, add mscorlib to the TPA. Then for each deps entry, check if they
//    are serviced. If they are not serviced, then look if they are present
//    app local. Worst case, default to the primary and seconday package
//    caches. Finally, for cases where deps file may not be present or if deps
//    did not have an entry for an app local assembly, just use them from the
//    app dir in the TPA path.
//
//  Parameters:
//     app_dir           - The application local directory
//     package_dir       - The directory path to where packages are restored
//     package_cache_dir - The directory path to secondary cache for packages
//     clr_dir           - The directory where the host loads the CLR
//
//  Returns:
//     output - Pointer to a string that will hold the resolved TPA paths
//
void deps_resolver_t::resolve_tpa_list(
        const pal::string_t& app_dir,
        const pal::string_t& package_dir,
        const pal::string_t& package_cache_dir,
        const pal::string_t& clr_dir,
        pal::string_t* output)
{
    // Obtain the local assemblies in the app dir.
    get_dir_assemblies(app_dir, _X("local"), &m_app_and_fx_assemblies);
    if (!m_fx_dir.empty())
    {
        get_dir_assemblies(m_fx_dir, _X("fx"), &m_app_and_fx_assemblies);
    }

    std::set<pal::string_t> items;

    add_mscorlib_to_tpa(clr_dir, &items, output);

    auto process_entry = [&](const deps_entry_t& entry)
    {
        // Is this asset a "runtime" type?
        if (items.count(entry.asset_name))
        {
			return;
        }

        pal::string_t candidate;

        // Is this a serviceable entry and is there an entry in the servicing index?
        if (entry.is_serviceable && entry.library_type == _X("Package") &&
                m_svc.find_redirection(entry.library_name, entry.library_version, entry.relative_path, &candidate))
        {
            add_tpa_asset(entry.asset_name, candidate, &items, output);
        }
        // Is this entry present in the secondary package cache?
        else if (entry.to_hash_matched_path(package_cache_dir, &candidate))
        {
            add_tpa_asset(entry.asset_name, candidate, &items, output);
        }
        // Is this entry present locally?
        else if (m_app_and_fx_assemblies.count(entry.asset_name))
        {
            // TODO: case insensitive
            add_tpa_asset(entry.asset_name, m_app_and_fx_assemblies.find(entry.asset_name)->second, &items, output);
        }
        // Is this entry present in the package restore dir?
        else if (entry.to_full_path(package_dir, &candidate))
        {
            add_tpa_asset(entry.asset_name, candidate, &items, output);
        }
	};
	
	const auto& deps_entries = m_deps.get_entries(_X("runtime"));
	std::for_each(deps_entries.begin(), deps_entries.end(), process_entry);
	const auto& fx_entries = m_fx_deps.get_entries(_X("runtime"));
	std::for_each(fx_entries.begin(), fx_entries.end(), process_entry);

    // Finally, if the deps file wasn't present or has missing entries, then
    // add the app local assemblies to the TPA.
    for (const auto& kv : m_app_and_fx_assemblies)
    {
        add_tpa_asset(kv.first, kv.second, &items, output);
    }
}

// -----------------------------------------------------------------------------
// Resolve the directories order for resources/native lookup
//
// Description:
//    This general purpose function specifies priority order of directory lookup
//    for both native images and resources specific resource images. Lookup for
//    resources assemblies is done by looking up two levels above from the file
//    path. Lookup for native images is done by looking up one level from the
//    file path.
//
//  Parameters:
//     asset_type        - The type of the asset that needs lookup, currently
//                         supports "resources" and "native"
//     app_dir           - The application local directory
//     package_dir       - The directory path to where packages are restored
//     package_cache_dir - The directory path to secondary cache for packages
//     clr_dir           - The directory where the host loads the CLR
//
//  Returns:
//     output - Pointer to a string that will hold the resolved lookup dirs
//
void deps_resolver_t::resolve_probe_dirs(
        const pal::string_t& asset_type,
        const pal::string_t& app_dir,
        const pal::string_t& package_dir,
        const pal::string_t& package_cache_dir,
        const pal::string_t& clr_dir,
        pal::string_t* output)
{
    assert(asset_type == _X("resources") || asset_type == _X("native"));

    // For resources assemblies, we need to provide the base directory of the resources path.
    // For example: .../Foo/en-US/Bar.dll, then, the resolved path is .../Foo
    std::function<pal::string_t(const pal::string_t&)> resources = [] (const pal::string_t& str) {
        return get_directory(get_directory(str));
    };
    // For native assemblies, obtain the directory path from the file path
    std::function<pal::string_t(const pal::string_t&)> native = [] (const pal::string_t& str) {
        return get_directory(str);
    };
    std::function<pal::string_t(const pal::string_t&)>& action = (asset_type == _X("resources")) ? resources : native;

    std::set<pal::string_t> items;

	const auto& entries = m_deps.get_entries(asset_type);
	const auto& fx_entries = m_fx_deps.get_entries(asset_type);

    // Fill the "output" with serviced DLL directories if they are serviceable
    // and have an entry present.
    auto add_serviced_entry = [&](const deps_entry_t& entry)
    {
        pal::string_t redirection_path;
        if (entry.is_serviceable && entry.library_type == _X("Package") &&
                m_svc.find_redirection(entry.library_name, entry.library_version, entry.relative_path, &redirection_path))
        {
            add_unique_path(asset_type, action(redirection_path), &items, output);
        }
	};

	std::for_each(entries.begin(), entries.end(), add_serviced_entry);
	std::for_each(fx_entries.begin(), fx_entries.end(), add_serviced_entry);

    pal::string_t candidate;

    // Take care of the secondary cache path
    if (!package_cache_dir.empty())
    {
        auto add_package_cache_entry = [&](const deps_entry_t& entry)
        {
            if (entry.to_hash_matched_path(package_cache_dir, &candidate))
            {
                add_unique_path(asset_type, action(candidate), &items, output);
            }
		};
		std::for_each(entries.begin(), entries.end(), add_package_cache_entry);
		std::for_each(fx_entries.begin(), fx_entries.end(), add_package_cache_entry);
    }

    // App local path
    add_unique_path(asset_type, app_dir, &items, output);

    // FX path if present
    if (!m_fx_dir.empty())
    {
        add_unique_path(asset_type, m_fx_dir, &items, output);
    }

    // Take care of the package restore path
    if (!package_dir.empty())
    {
        auto add_packages_entry = [&](const deps_entry_t& entry)
        {
            if (entry.asset_type == asset_type && entry.to_full_path(package_dir, &candidate))
            {
                add_unique_path(asset_type, action(candidate), &items, output);
            }
		};
		std::for_each(entries.begin(), entries.end(), add_packages_entry);
		std::for_each(fx_entries.begin(), fx_entries.end(), add_packages_entry);
    }

    // CLR path
    add_unique_path(asset_type, clr_dir, &items, output);
}


// -----------------------------------------------------------------------------
// Entrypoint to resolve TPA, native and resources path ordering to pass to CoreCLR.
//
//  Parameters:
//     app_dir           - The application local directory
//     package_dir       - The directory path to where packages are restored
//     package_cache_dir - The directory path to secondary cache for packages
//     clr_dir           - The directory where the host loads the CLR
//     probe_paths       - Pointer to struct containing fields that will contain
//                         resolved path ordering.
//
//
bool deps_resolver_t::resolve_probe_paths(
    const pal::string_t& app_dir,
    const pal::string_t& package_dir,
    const pal::string_t& package_cache_dir,
    const pal::string_t& clr_dir,
    probe_paths_t* probe_paths)
{
    resolve_tpa_list(app_dir, package_dir, package_cache_dir, clr_dir, &probe_paths->tpa);
    resolve_probe_dirs(_X("native"), app_dir, package_dir, package_cache_dir, clr_dir, &probe_paths->native);
    resolve_probe_dirs(_X("resources"), app_dir, package_dir, package_cache_dir, clr_dir, &probe_paths->resources);
    return true;
}

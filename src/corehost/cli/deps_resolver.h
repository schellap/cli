// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef DEPS_RESOLVER_H
#define DEPS_RESOLVER_H

#include <vector>

#include "pal.h"
#include "trace.h"
#include "deps_format.h"
#include "deps_entry.h"
#include "servicing_index.h"

// Probe paths to be resolved for ordering
struct probe_paths_t
{
    pal::string_t tpa;
    pal::string_t native;
    pal::string_t resources;
};

class deps_resolver_t
{
public:
    deps_resolver_t(const pal::string_t& fx_deps, const arguments_t& args)
        : m_svc(args.dotnet_servicing)
		, m_fx_deps(fx_deps)
		, m_fx_dir(get_directory(fx_deps))
        , m_coreclr_index(-1)
        , m_deps(args.deps_path)
    {
        load();
    }

    bool load();

    bool valid() { return m_deps.is_valid(); }

    bool resolve_probe_paths(
      const pal::string_t& app_dir,
      const pal::string_t& package_dir,
      const pal::string_t& package_cache_dir,
      const pal::string_t& clr_dir,
      probe_paths_t* probe_paths);

    pal::string_t resolve_coreclr_dir(
        const pal::string_t& app_dir,
        const pal::string_t& package_dir,
        const pal::string_t& package_cache_dir);

private:

    // Resolve order for TPA lookup.
    void resolve_tpa_list(
        const pal::string_t& app_dir,
        const pal::string_t& package_dir,
        const pal::string_t& package_cache_dir,
        const pal::string_t& clr_dir,
        pal::string_t* output);

    // Resolve order for culture and native DLL lookup.
    void resolve_probe_dirs(
        const pal::string_t& asset_type,
        const pal::string_t& app_dir,
        const pal::string_t& package_dir,
        const pal::string_t& package_cache_dir,
        const pal::string_t& clr_dir,
        pal::string_t* output);

    // Populate assemblies from the directory.
    void get_dir_assemblies(
        const pal::string_t& dir,
        const pal::string_t& dir_name,
        std::unordered_map<pal::string_t, pal::string_t>* dir_assemblies);

    // Servicing index to resolve serviced assembly paths.
    servicing_index_t m_svc;

	// Framework deps file.
	pal::string_t m_fx_dir;

    // Map of simple name -> full path of local/fx assemblies populated
    // in priority order of their extensions.
    std::unordered_map<pal::string_t, pal::string_t> m_app_and_fx_assemblies;

    // Special entry for coreclr in the deps entries
    int m_coreclr_index;

    // The dep file path
    deps_json_t m_deps;

	// Framework deps, not present if app local.
	deps_json_t m_fx_deps;

    // Is the deps file valid
    bool m_deps_valid;
};

#endif // DEPS_RESOLVER_H

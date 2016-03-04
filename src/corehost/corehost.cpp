// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "trace.h"
#include "utils.h"
#include "libhost.h"
#include "corehost.h"
#include "fx_muxer.h"

bool corehost_t::hostpolicy_exists_in_dir(const pal::string_t& lib_dir, pal::string_t* p_host_path)
{
	pal::string_t host_path = lib_dir;
	append_path(&host_path, LIBHOST_NAME);

	if (!pal::file_exists(host_path))
	{
		return false;
	}
	*p_host_path = host_path;
	return true;
}

StatusCode corehost_t::load_host_lib(const pal::string_t& lib_dir, pal::dll_t* h_host, corehost_main_fn* main_fn)
{
	pal::string_t host_path;
	if (!hostpolicy_exists_in_dir(lib_dir, &host_path))
	{
		return StatusCode::CoreHostLibMissingFailure;
	}

	// Load library
	if (!pal::load_library(host_path.c_str(), h_host))
	{
		trace::info(_X("Load library of %s failed"), host_path.c_str());
		return StatusCode::CoreHostLibLoadFailure;
	}

	// Obtain entrypoint symbol
	*main_fn = (corehost_main_fn)pal::get_symbol(*h_host, "corehost_main");

	return (*main_fn != nullptr)
		? StatusCode::Success
		: StatusCode::CoreHostEntryPointFailure;
}

int corehost_t::load_hostpolicy_and_execute(const pal::string_t& resolved_dir, const int argc, const pal::char_t* argv[])
{
	pal::dll_t corehost;
	corehost_main_fn host_main;
	StatusCode code = load_host_lib(resolved_dir, &corehost, &host_main);
	if (code != StatusCode::Success)
	{
		return code;
	}
	return host_main(argc, argv);
}

bool corehost_t::hostpolicy_exists_in_svc(pal::string_t* resolved_path)
{
#ifdef COREHOST_PACKAGE_SERVICING
	pal::string_t svc_dir;
	if (!pal::getenv(_X("DOTNET_SERVICING"), &svc_dir))
	{
		return false;
	}

	pal::string_t path = svc_dir;
	append_path(&path, COREHOST_PACKAGE_NAME);
	append_path(&path, COREHOST_PACKAGE_VERSION);
	append_path(&path, COREHOST_PACKAGE_COREHOST_RELATIVE_DIR);
	return hostpolicy_exists_in_dir(path, resolved_path);
#else
	return false;
#endif
}

HostMode corehost_t::detect_operating_mode(pal::string_t* own_path, pal::string_t* own_dir, pal::string_t* own_name)
{
	// Get the full name of the application
	if (!pal::get_own_executable_path(own_path) || !pal::realpath(own_path))
	{
		trace::error(_X("Failed to locate current executable"));
		own_path->clear();
		return HostMode::Invalid;
	}

	own_name->assign(get_filename(*own_path));
	own_dir->assign(get_directory(*own_path));

	if (coreclr_exists_in_dir(*own_dir))
	{
		pal::string_t own_deps_json = *own_dir;
		pal::string_t own_deps_filename = strip_file_ext(*own_name) + _X(".deps.json");
		append_path(&own_deps_json, own_deps_filename.c_str());
		return (pal::file_exists(own_deps_json)) ? HostMode::Standalone : HostMode::Framework;
	}
	else
	{
		return HostMode::Muxer;
	}
}

int corehost_t::run(const int argc, const pal::char_t* argv[])
{
	pal::string_t own_path, own_dir, own_name, resolved_dir;
	switch (detect_operating_mode(&own_path, &own_dir, &own_name))
	{
	case Muxer:
		return fx_muxer_t(argc, argv).execute();

	case Framework:
		if (hostpolicy_exists_in_dir(own_dir, &resolved_dir))
		{
			return load_hostpolicy_and_execute(resolved_dir, argc, argv);
		}
		return StatusCode::CoreHostLibMissingFailure;

	case Standalone:
		if (hostpolicy_exists_in_svc(&resolved_dir) ||
			hostpolicy_exists_in_dir(own_dir, &resolved_dir))
		{
			return load_hostpolicy_and_execute(resolved_dir, argc, argv);
		}
		return StatusCode::CoreHostLibMissingFailure;

	default:
		return StatusCode::CoreHostResolveModeFailure;
	}
}

#if defined(_WIN32)
int __cdecl wmain(const int argc, const pal::char_t* argv[])
#else
int main(const int argc, const pal::char_t* argv[])
#endif
{
    trace::setup();
	corehost_t corehost;
	return corehost.run(argc, argv);
}


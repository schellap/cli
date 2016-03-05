// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "trace.h"
#include "utils.h"
#include "corehost.h"

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

int corehost_t::load_host_lib(const pal::string_t& lib_dir, pal::dll_t* h_host, corehost_main_fn* main_fn)
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

int corehost_t::load_hostpolicy_and_execute(
    const pal::string_t& resolved_dir,
    const int argc,
    const pal::char_t* argv[])
{
	pal::dll_t corehost;
	corehost_main_fn host_main = nullptr;

    int code = load_host_lib(resolved_dir, &corehost, &host_main);

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

int corehost_t::run(const int argc, const pal::char_t* argv[])
{
	pal::string_t own_dir;
    auto mode = detect_operating_mode(argc, argv, &own_dir);

    switch (mode)
	{
	case Muxer:
        return load_hostpolicy_and_execute(own_dir, argc, argv);

	case Framework:
        {
            pal::string_t resolved_dir;
            pal::string_t fx_root = get_directory(get_directory(get_directory(own_dir)));
            if (hostpolicy_exists_in_dir(fx_root, &resolved_dir))
            {
                return load_hostpolicy_and_execute(resolved_dir, argc, argv);
            }
        }
        return StatusCode::CoreHostLibMissingFailure;

	case Standalone:
        {
            pal::string_t resolved_dir;
            if (hostpolicy_exists_in_svc(&resolved_dir) ||
                hostpolicy_exists_in_dir(own_dir, &resolved_dir))
            {
                return load_hostpolicy_and_execute(resolved_dir, argc, argv);
            }
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


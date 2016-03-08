// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "trace.h"
#include "utils.h"
#include "corehost.h"
#include "fx_muxer.h"

bool corehost_t::hostpolicy_exists_in_dir(const pal::string_t& lib_dir, pal::string_t* p_host_path = nullptr)
{
	pal::string_t host_path = lib_dir;
	append_path(&host_path, LIBHOST_NAME);

	if (!pal::file_exists(host_path))
	{
		return false;
	}
    if (p_host_path)
    {
        *p_host_path = host_path;
    }
	return true;
}

int corehost_t::load_host_lib(
    const pal::string_t& lib_dir,
    pal::dll_t* h_host,
    corehost_load_fn* load_fn,
    corehost_main_fn* main_fn,
    corehost_unload_fn* unload_fn)
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

	// Obtain entrypoint symbols
    *load_fn = (corehost_load_fn)pal::get_symbol(*h_host, "corehost_load");
	*main_fn = (corehost_main_fn)pal::get_symbol(*h_host, "corehost_main");
    *unload_fn = (corehost_unload_fn)pal::get_symbol(*h_host, "corehost_unload");

	return (*main_fn) && (*load_fn) && (*unload_fn)
		? StatusCode::Success
		: StatusCode::CoreHostEntryPointFailure;
}

int corehost_t::execute_app(
    const pal::string_t& policy_dir,
    const pal::string_t& fx_dir,
    const runtime_config_t* config,
    const int argc,
    const pal::char_t* argv[])
{
	pal::dll_t corehost;
	corehost_main_fn host_main = nullptr;
    corehost_load_fn host_load = nullptr;
    corehost_unload_fn host_unload = nullptr;

    int code = load_host_lib(policy_dir, &corehost, &host_load, &host_main, &host_unload);

	if (code != StatusCode::Success)
	{
		return code;
	}
    corehost_init_t init(fx_dir, config);
    if ((code = host_load(&init)) == 0)
    {
        code = host_main(argc, argv);
        (void) host_unload();
    }
    return code;
}

bool corehost_t::hostpolicy_exists_in_svc(pal::string_t* resolved_dir)
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
    if (hostpolicy_exists_in_dir(path))
    {
        resolved_dir->assign(path);
    }
    return true;
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
        return fx_muxer_t().execute(own_dir, argc, argv);

	case Framework:
        return execute_app(own_dir, own_dir, nullptr, argc, argv);

	case Standalone:
        {
            pal::string_t svc_dir;
            return execute_app(
                hostpolicy_exists_in_svc(&svc_dir) ? svc_dir : own_dir, _X(""), nullptr, argc, argv);
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


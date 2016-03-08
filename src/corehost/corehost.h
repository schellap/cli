// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "pal.h"
#include "libhost.h"

extern int corehost_main(const int argc, const pal::char_t* argv[]);

enum StatusCode
{
    Success                    = 0,
    CoreHostLibLoadFailure     = 0x41,
    CoreHostLibMissingFailure  = 0x42,
    CoreHostEntryPointFailure  = 0x43,
    CoreHostCurExeFindFailure  = 0x44,
	CoreHostResolveModeFailure = 0x45,
	// Only append here, do not modify existing ones.
};

typedef int (*corehost_load_fn) (const corehost_init_t* init);
typedef int (*corehost_main_fn) (const int argc, const pal::char_t* argv[]);
typedef int (*corehost_unload_fn) ();

class corehost_t
{
public:

	int run(const int argc, const pal::char_t* argv[]);
    static int corehost_t::execute_app(
        const pal::string_t& policy_dir,
        const pal::string_t& fx_dir,
        const runtime_config_t* config,
        const int argc,
        const pal::char_t* argv[]);

private:

	static int load_host_lib(
        const pal::string_t& lib_dir,
        pal::dll_t* h_host,
        corehost_load_fn* load_fn,
        corehost_main_fn* main_fn,
        corehost_unload_fn* unload_fn);

	static bool hostpolicy_exists_in_svc(pal::string_t* resolved_path);
	static bool hostpolicy_exists_in_dir(const pal::string_t& lib_dir, pal::string_t* p_host_path);
};

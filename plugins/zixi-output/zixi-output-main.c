#include <util/text-lookup.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>


OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("zixi-output", "en-US")

#define ZIXI_SERVICES_LOG_STR "[zixi-output plugin] "
#define ZIXI_SERVICES_VER_STR "zixi-output plugin (libobs " OBS_VERSION ")"

int zixi_load_dll();
int zixi_unload_dll();

extern struct obs_service_info zixi_service;
extern struct obs_output_info zixi_output;

bool obs_module_load(void)
{
	if (zixi_load_dll() == 0)
	{
		obs_register_output(&zixi_output);
		obs_register_service(&zixi_service);
		return true;
	}
	return false;
}

void obs_module_unload(void)
{
	zixi_unload_dll();
}

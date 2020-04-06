#include <obs-module.h>

struct zixi_service {
	char * zixi_url;
	char * zixi_password;
	int	zixi_latency_id;
	int	zixi_encryption_type;
	char * zixi_encryption_key;
	bool	zixi_encoder_feedback;
	bool    zixi_bonding;
};

static void zixi_service_update(void *data, obs_data_t *settings);

static const char *zixi_service_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ZixiServer");
}
static void zixi_service_update_ex(void *data, obs_data_t *settings, bool set_defaults);

static void *zixi_service_create(obs_data_t *settings, obs_service_t *service)
{
	struct zixi_service *data = bzalloc(sizeof(struct zixi_service));
	zixi_service_update_ex(data, settings,true);

	UNUSED_PARAMETER(service);
	return data;
}

static void zixi_service_update_ex(void *data, obs_data_t *settings, bool set_defaults) {
	struct zixi_service* service = data;
	bfree(service->zixi_url);
	bfree(service->zixi_password);
	bfree(service->zixi_encryption_key);

	service->zixi_url = bstrdup(obs_data_get_string(settings, "zixi_url"));
	service->zixi_password = bstrdup(obs_data_get_string(settings, "zixi_password"));
	service->zixi_encryption_key = bstrdup(obs_data_get_string(settings, "zixi_encryption_key"));
	service->zixi_encoder_feedback = obs_data_get_bool(settings, "zixi_encoder_feedback");
	service->zixi_bonding = obs_data_get_bool(settings, "zixi_bonding");

	if (set_defaults) {
		service->zixi_encryption_type = 3;
		obs_data_set_int(settings, "zixi_encryption_id",3);
		service->zixi_latency_id = 6;
		obs_data_set_int(settings, "zixi_latency_id",6);
	} else {
		service->zixi_encryption_type = obs_data_get_int(settings, "zixi_encryption_id");
		service->zixi_latency_id = obs_data_get_int(settings, "zixi_latency_id");
	}
}
static void zixi_service_update(void *data , obs_data_t *settings) {
	zixi_service_update_ex(data, settings, false);
}

static void zixi_service_destroy(void *data) {
	struct zixi_service *service = data;
	bfree(service->zixi_url);
	bfree(service->zixi_password);
	bfree(service->zixi_encryption_key);
}

static obs_properties_t *zixi_service_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	p = obs_properties_add_bool(ppts, "zixi_fwd", obs_module_text("ZixiFwd"));
	obs_properties_add_text(ppts, "zixi_url", obs_module_text("ZixiUrl"),
		OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "zixi_password", obs_module_text("ZixiPassword"),
		OBS_TEXT_PASSWORD);
	obs_properties_add_list(ppts, "zixi_latency", obs_module_text("ZixiLatency"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(ppts, "zixi_encryption_type", obs_module_text("ZixiEncryptionType"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);
	obs_properties_add_text(ppts, "zixi_encryption_key", obs_module_text("ZixiEncryptionKey"),
		OBS_TEXT_PASSWORD);
	obs_properties_add_bool(ppts, "zixi_encoder_feedback", obs_module_text("ZixiEncoderFeedback"));
	obs_properties_add_bool(ppts, "zixi_bonding", obs_module_text("ZixiBonding"));

	return ppts;
}

static const char *zixi_service_url(void *data)
{
	struct zixi_service *service = data;
	return service->zixi_url;
}

static const char *zixi_service_key(void *data)
{
	struct zixi_service *service = data;
	return service->zixi_encryption_key;
}

static const char *zixi_service_username(void *data)
{
	UNUSED_PARAMETER(data);
	return NULL;
}

static const char *zixi_service_password(void *data)
{
	struct zixi_service *service = data;
	return service->zixi_password;
}

static const char * zixi_service_get_output_type(void * unused) {
	UNUSED_PARAMETER(unused);
	return "zixi_output";
}
struct obs_service_info zixi_service = {
	.id = "zixi_service",
	.get_name = zixi_service_name,
	.create = zixi_service_create,
	.destroy = zixi_service_destroy,
	.update = zixi_service_update,
	.get_properties = zixi_service_properties,
	.get_url = zixi_service_url,
	.get_key = zixi_service_key,
	.get_username = zixi_service_username,
	.get_password = zixi_service_password,
	.get_output_type = zixi_service_get_output_type
};

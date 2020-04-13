#include <obs-module.h>

struct rtmp_custom {
	char *server, *key;
	bool use_auth;
	char *username, *password;

#ifdef ENABLE_ZIXI_SUPPORT
	bool zixi_fwd;
	char * zixi_url;
	char * zixi_password;
	int	zixi_latency_id;
	int	zixi_encryption_type;
	char * zixi_encryption_key;
	bool	zixi_encoder_feedback;
	bool    zixi_bonding;
#endif
};

static const char *rtmp_custom_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("CustomStreamingServer");
}

static void rtmp_custom_update_ex(void *data, obs_data_t *settings, bool load_defaults) {
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service->username);
	bfree(service->password);
#ifdef ENABLE_ZIXI_SUPPORT
	bfree(service->zixi_url);
	bfree(service->zixi_password);
	bfree(service->zixi_encryption_key);

	service->zixi_fwd = obs_data_get_bool(settings, "zixi_fwd");
	service->zixi_url = bstrdup(obs_data_get_string(settings, "zixi_url"));
	service->zixi_password = bstrdup(obs_data_get_string(settings, "zixi_password"));
	service->zixi_latency_id = obs_data_get_int(settings, "zixi_latency_id");
	service->zixi_encryption_type = obs_data_get_int(settings, "zixi_encryption_id");
	service->zixi_encryption_key = bstrdup(obs_data_get_string(settings, "zixi_encryption_key"));
	service->zixi_encoder_feedback = obs_data_get_bool(settings, "zixi_encoder_feedback");
	service->zixi_bonding = obs_data_get_bool(settings, "zixi_bonding");
	if (load_defaults) {
		obs_data_set_int(settings, "zixi_latency_id",6);
		service->zixi_latency_id = 6;
		service->zixi_encryption_type = 3;
		obs_data_set_int(settings, "zixi_encryption_id",3);
	}
#endif

	service->server = bstrdup(obs_data_get_string(settings, "server"));
	service->key = bstrdup(obs_data_get_string(settings, "key"));
	service->use_auth = obs_data_get_bool(settings, "use_auth");
	service->username = bstrdup(obs_data_get_string(settings, "username"));
	service->password = bstrdup(obs_data_get_string(settings, "password"));
}

static void rtmp_custom_update(void *data, obs_data_t *settings)
{
	rtmp_custom_update_ex(data, settings, false);
}

static void rtmp_custom_destroy(void *data)
{
	struct rtmp_custom *service = data;

	bfree(service->server);
	bfree(service->key);
	bfree(service->username);
	bfree(service->password);
#ifdef ENABLE_ZIXI_SUPPORT
	bfree(service->zixi_url);
	bfree(service->zixi_password);
	bfree(service->zixi_encryption_key);
#endif

	bfree(service);
}

static void *rtmp_custom_create(obs_data_t *settings, obs_service_t *service)
{
	struct rtmp_custom *data = bzalloc(sizeof(struct rtmp_custom));
	rtmp_custom_update_ex(data, settings, true);

	UNUSED_PARAMETER(service);
	return data;
}

static bool use_auth_modified(obs_properties_t *ppts, obs_property_t *p,
			      obs_data_t *settings)
{
	bool use_auth = obs_data_get_bool(settings, "use_auth");
	p = obs_properties_get(ppts, "username");
	obs_property_set_visible(p, use_auth);
	p = obs_properties_get(ppts, "password");
	obs_property_set_visible(p, use_auth);
	return true;
}

static obs_properties_t *rtmp_custom_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *ppts = obs_properties_create();
	obs_property_t *p;

	obs_properties_add_text(ppts, "server", "URL", OBS_TEXT_DEFAULT);

	obs_properties_add_text(ppts, "key", obs_module_text("StreamKey"),
				OBS_TEXT_PASSWORD);

	p = obs_properties_add_bool(ppts, "use_auth",
				    obs_module_text("UseAuth"));
	obs_properties_add_text(ppts, "username", obs_module_text("Username"),
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "password", obs_module_text("Password"),
				OBS_TEXT_PASSWORD);
	obs_property_set_modified_callback(p, use_auth_modified);

#ifdef ENABLE_ZIXI_SUPPORT
	p = obs_properties_add_bool(ppts, "zixi_fwd", obs_module_text("ZixiFwd"));
	obs_properties_add_text(ppts,"zixi_url", obs_module_text("ZixiUrl"),
		OBS_TEXT_DEFAULT);
	obs_properties_add_text(ppts, "zixi_password", obs_module_text("ZixiPassword"),
		OBS_TEXT_PASSWORD);
	obs_properties_add_list(ppts, "zixi_latency", obs_module_text("ZixiLatency"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_properties_add_list(ppts, "zixi_encryption_type", obs_module_text("ZixiEncryptionType"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_STRING);
	obs_properties_add_text(ppts, "zixi_encryption_key", obs_module_text("ZixiEncryptionKey"),
		OBS_TEXT_PASSWORD);
	obs_properties_add_bool(ppts,"zixi_encoder_feedback",obs_module_text("ZixiEncoderFeedback"));
	obs_properties_add_bool(ppts, "zixi_bonding", obs_module_text("ZixiBonding"));
#endif 	// ENABLE_ZIXI_SUPPORT
	return ppts;
}

static const char *rtmp_custom_url(void *data)
{
	struct rtmp_custom *service = data;
	return service->server;
}

static const char *rtmp_custom_key(void *data)
{
	struct rtmp_custom *service = data;
	return service->key;
}

static const char *rtmp_custom_username(void *data)
{
	struct rtmp_custom *service = data;
	if (!service->use_auth)
		return NULL;
	return service->username;
}

static const char *rtmp_custom_password(void *data)
{
	struct rtmp_custom *service = data;
	if (!service->use_auth)
		return NULL;
	return service->password;
}

static const char * rtmp_custom_get_output_type(void *data) {
#ifndef ENABLE_ZIXI_SUPPORT
	UNUSED_PARAMETER(data);
#else
	struct rtmp_custom* service = data;
#endif
	const char* ret = NULL;

#ifdef ENABLE_ZIXI_SUPPORT
	if (service->zixi_fwd) {
		ret = "zixi_output";
	}
#endif
	return ret;
}

struct obs_service_info rtmp_custom_service = {
	.id              = "rtmp_custom",
	.get_name        = rtmp_custom_name,
	.create          = rtmp_custom_create,
	.destroy         = rtmp_custom_destroy,
	.update          = rtmp_custom_update,
	.get_properties  = rtmp_custom_properties,
	.get_url         = rtmp_custom_url,
	.get_key         = rtmp_custom_key,
	.get_username    = rtmp_custom_username,
	.get_password    = rtmp_custom_password,
	.get_output_type = rtmp_custom_get_output_type
};

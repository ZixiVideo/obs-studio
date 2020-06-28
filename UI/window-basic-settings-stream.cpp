#include <QMessageBox>
#include <QUrl>

#include "window-basic-settings.hpp"
#include "obs-frontend-api.h"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "url-push-button.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#include "auth-oauth.hpp"
#endif

struct QCef;
struct QCefCookieManager;

extern QCef *cef;
extern QCefCookieManager *panel_cookies;

enum class ListOpt : int { ShowAll = 1, Custom, Zixi };

enum class Section : int {
	Connect,
	StreamKey,
};

inline bool OBSBasicSettings::IsZixiService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Zixi;
}

inline bool OBSBasicSettings::IsCustomService() const
{
	return ui->service->currentData().toInt() == (int)ListOpt::Custom;
}

void OBSBasicSettings::InitStreamPage()
{
	ui->connectAccount2->setVisible(false);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

	int vertSpacing = ui->topStreamLayout->verticalSpacing();

	QMargins m = ui->topStreamLayout->contentsMargins();
	m.setBottom(vertSpacing / 2);
	ui->topStreamLayout->setContentsMargins(m);

	m = ui->loginPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->loginPageLayout->setContentsMargins(m);

	m = ui->streamkeyPageLayout->contentsMargins();
	m.setTop(vertSpacing / 2);
	ui->streamkeyPageLayout->setContentsMargins(m);

	LoadServices(false);

	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.None"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.BTTV"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.FFZ"));
	ui->twitchAddonDropdown->addItem(
		QTStr("Basic.Settings.Stream.TTVAddon.Both"));

	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateServerList()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateKeyLink()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateVodTrackSetting()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateServiceRecommendations()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateResFPSLimits()));
	connect(ui->customServer, SIGNAL(textChanged(const QString &)), this,
		SLOT(UpdateKeyLink()));
	connect(ui->ignoreRecommended, SIGNAL(clicked(bool)), this,
		SLOT(DisplayEnforceWarning(bool)));
	connect(ui->ignoreRecommended, SIGNAL(toggled(bool)), this,
		SLOT(UpdateResFPSLimits()));
	connect(ui->customServer, SIGNAL(editingFinished(const QString &)),
		this, SLOT(UpdateKeyLink()));
	connect(ui->service, SIGNAL(currentIndexChanged(int)), this,
		SLOT(UpdateMoreInfoLink()));
	bool disableAllZixiControls = false;
#ifdef ENABLE_ZIXI_SUPPORT
	disableAllZixiControls = !IsZixiPluginLoaded();

	if (!disableAllZixiControls) {
		populateZixiCombos();
	}
#else
	disableAllZixiControls = true;
#endif

	DisableZixiControls(disableAllZixiControls);
}

void OBSBasicSettings::populateZixiCombos()
{
	obs_properties_t *zixi_props = obs_get_output_properties("zixi_output");
	obs_property_t *latencies =
		obs_properties_get(zixi_props, "zixi_latencies");
	obs_property_t *encryptions =
		obs_properties_get(zixi_props, "zixi_encryptions");
	obs_data_t *zixi_defaults = obs_output_defaults("zixi_output");

	int latencies_count = obs_property_list_item_count(latencies);
	for (int iter = 0; iter < latencies_count; ++iter) {
		const char *name = obs_property_list_item_name(latencies, iter);
		ui->zixiFwdLatency->addItem(name, iter);
	}

	int encryptions_count = obs_property_list_item_count(encryptions);
	for (int iter = 0; iter < encryptions_count; ++iter) {
		const char *name =
			obs_property_list_item_name(encryptions, iter);
		ui->zixiFwdEncryptionType->addItem(name, iter);
	}

	ui->zixiFwdVersion->setText(
		QT_UTF8(obs_data_get_string(zixi_defaults, "zixi_version")));
	obs_data_release(zixi_defaults);
}

bool OBSBasicSettings::IsZixiPluginLoaded()
{
	obs_properties_t *zixi_props = obs_get_output_properties("zixi_output");
	return !!zixi_props;
}
void OBSBasicSettings::on_zixiFwdPasswordShow_clicked()
{
	if (ui->zixiFwdPassword->echoMode() == QLineEdit::Password) {
		ui->zixiFwdPassword->setEchoMode(QLineEdit::Normal);
		ui->zixiFwdPasswordShow->setText(QTStr("Hide"));
	} else {
		ui->zixiFwdPassword->setEchoMode(QLineEdit::Password);
		ui->zixiFwdPasswordShow->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_zixiFwdEncryptionKeyShow_clicked()
{
	if (ui->zixiFwdEncryptionKey->echoMode() == QLineEdit::Password) {
		ui->zixiFwdEncryptionKey->setEchoMode(QLineEdit::Normal);
		ui->zixiFwdEncryptionKeyShow->setText(QTStr("Hide"));
	} else {
		ui->zixiFwdEncryptionKey->setEchoMode(QLineEdit::Password);
		ui->zixiFwdEncryptionKeyShow->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_zixiFwd_toggled()
{
	bool fwd = ui->zixiFwd->isChecked();
	ui->zixiFwdVersion->setVisible(fwd);
	ui->zixiFwdVersionLabel->setVisible(fwd);
	ui->zixiFwdUrl->setVisible(fwd);
	ui->zixiFwdUrlLabel->setVisible(fwd);
	ui->zixiFwdSampleUri->setVisible(fwd);
	ui->zixiFwdSampleUriLabel->setVisible(fwd);
	ui->zixiFwdPassword->setVisible(fwd);
	ui->zixiFwdPasswordLabel->setVisible(fwd);
	ui->zixiFwdPasswordShow->setVisible(fwd);
	ui->zixiFwdLatencyLabel->setVisible(fwd);
	ui->zixiFwdLatency->setVisible(fwd);
	ui->zixiFwdEncryptionType->setVisible(fwd);
	ui->zixiFwdEncryptionTypeLabel->setVisible(fwd);

	ui->zixiFwdEnableBonding->setVisible(fwd);
	ui->zixiFwdEncoderFeedback->setVisible(fwd);
	on_zixiFwdEncryptionType_currentIndexChanged(
		ui->zixiFwdEncryptionType->currentIndex());
}

void OBSBasicSettings::on_zixiFwdEncryptionType_currentIndexChanged(int idx)
{
	bool show_key = idx != 3 && ui->zixiFwd->isChecked();
	ui->zixiFwdEncryptionKey->setVisible(show_key);
	ui->zixiFwdEncryptionKeyShow->setVisible(show_key);
	ui->zixiFwdEncryptionKeyLabel->setVisible(show_key);
}

void OBSBasicSettings::DisableZixiControls(bool disable_all)
{
	bool visible_value = !disable_all;

	ui->zixiFwd->setVisible(visible_value);
	ui->zixiFwdVersion->setVisible(visible_value);
	ui->zixiFwdVersionLabel->setVisible(visible_value);
	ui->zixiFwdUrl->setVisible(visible_value);
	ui->zixiFwdUrlLabel->setVisible(visible_value);
	ui->zixiFwdPassword->setVisible(visible_value);
	ui->zixiFwdPasswordLabel->setVisible(visible_value);
	ui->zixiFwdPasswordShow->setVisible(visible_value);
	ui->zixiFwdLatencyLabel->setVisible(visible_value);
	ui->zixiFwdLatency->setVisible(visible_value);
	ui->zixiFwdEncryptionType->setVisible(visible_value);
	ui->zixiFwdEncryptionTypeLabel->setVisible(visible_value);
	ui->zixiFwdEncryptionKey->setVisible(visible_value);
	ui->zixiFwdEncryptionKeyShow->setVisible(visible_value);
	ui->zixiFwdEncryptionKeyLabel->setVisible(visible_value);
	ui->zixiFwdEnableBonding->setVisible(visible_value);
	ui->zixiFwdEncoderFeedback->setVisible(visible_value);
	ui->zixiFwdSampleUri->setVisible(visible_value);
	ui->zixiFwdSampleUriLabel->setVisible(visible_value);
	if (visible_value)
		on_zixiFwdEncryptionType_currentIndexChanged(
			ui->zixiFwdEncryptionType->currentIndex());
}

void OBSBasicSettings::LoadStream1Settings()
{
	bool ignoreRecommended =
		config_get_bool(main->Config(), "Stream1", "IgnoreRecommended");

	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);

	loading = true;

	obs_data_t *settings = obs_service_get_settings(service_obj);

	const char *service = obs_data_get_string(settings, "service");
	const char *server = obs_data_get_string(settings, "server");
	const char *key = obs_data_get_string(settings, "key");
#ifdef ENABLE_ZIXI_SUPPORT
	if (strcmp(type, "zixi_service") == 0) {
		ui->service->setCurrentIndex(1);
		on_service_currentIndexChanged(1);
		const char *zixi_url =
			obs_data_get_string(settings, "zixi_url");
		const char *zixi_password =
			obs_data_get_string(settings, "zixi_password");
		int zixi_latency_id =
			obs_data_get_int(settings, "zixi_latency_id");
		int zixi_encryption_id =
			obs_data_get_int(settings, "zixi_encryption_id");
		const char *zixi_encryption_key = nullptr;
		if (zixi_encryption_id != 3) {
			zixi_encryption_key = obs_data_get_string(
				settings, "zixi_encryption_key");
		}
		bool zixi_bonding = obs_data_get_bool(settings, "zixi_bonding");
		bool encoder_feedback =
			obs_data_get_bool(settings, "zixi_encoder_feedback");

		ui->zixiFwdUrl->setText(QT_UTF8(zixi_url));
		ui->zixiFwdPassword->setText(QT_UTF8(zixi_password));
		ui->zixiFwdLatency->setCurrentIndex(zixi_latency_id);
		ui->zixiFwdEncryptionType->setCurrentIndex(zixi_encryption_id);

		if (zixi_encryption_key != nullptr) {
			ui->zixiFwdEncryptionKey->setText(
				QT_UTF8(zixi_encryption_key));
		}
		ui->zixiFwdEnableBonding->setChecked(zixi_bonding);
		ui->zixiFwdEncoderFeedback->setChecked(encoder_feedback);
		ui->zixiFwd->setChecked(true);
		on_zixiFwd_toggled();
		on_zixiFwdEncryptionType_currentIndexChanged(
			zixi_encryption_id);
		loading = false;
		return;
	} else
#endif
		if (strcmp(type, "rtmp_custom") == 0) {
		ui->service->setCurrentIndex(0);
		ui->customServer->setText(server);

		bool use_auth = obs_data_get_bool(settings, "use_auth");
		const char *username =
			obs_data_get_string(settings, "username");
		const char *password =
			obs_data_get_string(settings, "password");
		ui->authUsername->setText(QT_UTF8(username));
		ui->authPw->setText(QT_UTF8(password));
		ui->useAuth->setChecked(use_auth);
	} else {
		int idx = ui->service->findText(service);
		if (idx == -1) {
			if (service && *service)
				ui->service->insertItem(1, service);
			idx = 1;
		}
		ui->service->setCurrentIndex(idx);

		bool bw_test = obs_data_get_bool(settings, "bwtest");
		ui->bandwidthTestEnable->setChecked(bw_test);

		idx = config_get_int(main->Config(), "Twitch", "AddonChoice");
		ui->twitchAddonDropdown->setCurrentIndex(idx);
	}

	UpdateServerList();

	if (strcmp(type, "rtmp_common") == 0) {
		int idx = ui->server->findData(server);
		if (idx == -1) {
			if (server && *server)
				ui->server->insertItem(0, server, server);
			idx = 0;
		}
		ui->server->setCurrentIndex(idx);
	}

	ui->key->setText(key);

	lastService.clear();
	on_service_currentIndexChanged(0);

#ifdef ENABLE_ZIXI_SUPPORT
	if (IsZixiPluginLoaded()) {
		bool zixi_fwd = obs_data_get_bool(settings, "zixi_fwd");
		ui->zixiFwd->setChecked(zixi_fwd);

		const char *zixi_url =
			obs_data_get_string(settings, "zixi_url");
		const char *zixi_password =
			obs_data_get_string(settings, "zixi_password");
		int zixi_latency_id =
			obs_data_get_int(settings, "zixi_latency_id");
		int zixi_encryption_id =
			obs_data_get_int(settings, "zixi_encryption_id");
		const char *zixi_encryption_key = nullptr;
		if (zixi_encryption_id != 3) {
			zixi_encryption_key = obs_data_get_string(
				settings, "zixi_encryption_key");
		}
		bool zixi_bonding = obs_data_get_bool(settings, "zixi_bonding");
		bool encoder_feedback =
			obs_data_get_bool(settings, "zixi_encoder_feedback");

		ui->zixiFwdUrl->setText(QT_UTF8(zixi_url));
		ui->zixiFwdPassword->setText(QT_UTF8(zixi_password));
		ui->zixiFwdLatency->setCurrentIndex(zixi_latency_id);
		ui->zixiFwdEncryptionType->setCurrentIndex(zixi_encryption_id);

		if (zixi_encryption_key != nullptr) {
			ui->zixiFwdEncryptionKey->setText(
				QT_UTF8(zixi_encryption_key));
		}
		ui->zixiFwdEnableBonding->setChecked(zixi_bonding);
		ui->zixiFwdEncoderFeedback->setChecked(encoder_feedback);
		on_zixiFwd_toggled();
		on_zixiFwdEncryptionType_currentIndexChanged(
			zixi_encryption_id);

	} else {
		ui->zixiFwd->setVisible(false);
	}
#endif

	obs_data_release(settings);

	UpdateKeyLink();
	UpdateMoreInfoLink();
	UpdateVodTrackSetting();
	UpdateServiceRecommendations();

	bool streamActive = obs_frontend_streaming_active();
	ui->streamPage->setEnabled(!streamActive);

	ui->ignoreRecommended->setChecked(ignoreRecommended);

	loading = false;

	QMetaObject::invokeMethod(this, "UpdateResFPSLimits",
				  Qt::QueuedConnection);
}

void OBSBasicSettings::SaveStream1Settings()
{
	bool customServer = IsCustomService();
	const char *service_id = customServer ? "rtmp_custom" : "rtmp_common";

#ifdef ENABLE_ZIXI_SUPPORT
	if (IsZixiService())
		service_id = "zixi_service";
#endif

	obs_service_t *oldService = main->GetService();
	OBSData hotkeyData = obs_hotkeys_save_service(oldService);
	obs_data_release(hotkeyData);

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (strcmp(service_id, "zixi_service") != 0) {
		if (!customServer) {
			obs_data_set_string(
				settings, "service",
				QT_TO_UTF8(ui->service->currentText()));
			obs_data_set_string(
				settings, "server",
				QT_TO_UTF8(
					ui->server->currentData().toString()));
		} else {
			obs_data_set_string(
				settings, "server",
				QT_TO_UTF8(ui->customServer->text()));
			obs_data_set_bool(settings, "use_auth",
					  ui->useAuth->isChecked());
			if (ui->useAuth->isChecked()) {
				obs_data_set_string(
					settings, "username",
					QT_TO_UTF8(ui->authUsername->text()));
				obs_data_set_string(
					settings, "password",
					QT_TO_UTF8(ui->authPw->text()));
			}
		}

		obs_data_set_bool(settings, "bwtest",
				  ui->bandwidthTestEnable->isChecked());
		obs_data_set_string(settings, "key",
				    QT_TO_UTF8(ui->key->text()));

#ifdef ENABLE_ZIXI_SUPPORT
		if (IsZixiPluginLoaded()) {
			obs_data_set_bool(settings, "zixi_fwd",
					  ui->zixiFwd->isChecked());
			if (ui->zixiFwd->isChecked()) {
				obs_data_set_string(
					settings, "zixi_url",
					QT_TO_UTF8(ui->zixiFwdUrl->text()));
				obs_data_set_string(
					settings, "zixi_password",
					QT_TO_UTF8(
						ui->zixiFwdPassword->text()));
				obs_data_set_int(
					settings, "zixi_latency_id",
					ui->zixiFwdLatency->currentIndex());
				obs_data_set_int(settings, "zixi_encryption_id",
						 ui->zixiFwdEncryptionType
							 ->currentIndex());
				if (ui->zixiFwdEncryptionType->currentIndex() !=
				    3) {
					obs_data_set_string(
						settings, "zixi_encryption_key",
						QT_TO_UTF8(
							ui->zixiFwdEncryptionKey
								->text()));
				}
				obs_data_set_bool(
					settings, "zixi_bonding",
					ui->zixiFwdEnableBonding->isChecked());
				obs_data_set_bool(
					settings, "zixi_encoder_feedback",
					ui->zixiFwdEncoderFeedback->isChecked());
				obs_data_set_bool(settings, "zixi_rtmp_fwd",
						  true);
			}
		}
#endif
	} else {
		obs_data_set_string(settings, "zixi_url",
				    QT_TO_UTF8(ui->zixiFwdUrl->text()));
		obs_data_set_string(settings, "zixi_password",
				    QT_TO_UTF8(ui->zixiFwdPassword->text()));
		obs_data_set_int(settings, "zixi_latency_id",
				 ui->zixiFwdLatency->currentIndex());
		obs_data_set_int(settings, "zixi_encryption_id",
				 ui->zixiFwdEncryptionType->currentIndex());
		if (ui->zixiFwdEncryptionType->currentIndex() != 3) {
			obs_data_set_string(
				settings, "zixi_encryption_key",
				QT_TO_UTF8(ui->zixiFwdEncryptionKey->text()));
		}
		obs_data_set_bool(settings, "zixi_bonding",
				  ui->zixiFwdEnableBonding->isChecked());
		obs_data_set_bool(settings, "zixi_encoder_feedback",
				  ui->zixiFwdEncoderFeedback->isChecked());
		obs_data_set_bool(settings, "zixi_rtmp_fwd", false);
	}

	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSService newService = obs_service_create(
		service_id, "default_service", settings, hotkeyData);
	obs_service_release(newService);

	if (!newService)
		return;

	main->SetService(newService);
	main->SaveService();
	main->auth = auth;
	if (!!main->auth)
		main->auth->LoadUI();

	SaveCheckBox(ui->ignoreRecommended, "Stream1", "IgnoreRecommended");
}

void OBSBasicSettings::UpdateMoreInfoLink()
{
	if (IsCustomService()) {
		ui->moreInfoButton->hide();
		return;
	}

	QString serviceName = ui->service->currentText();
	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	const char *more_info_link =
		obs_data_get_string(settings, "more_info_link");

	if (!more_info_link || (*more_info_link == '\0')) {
		ui->moreInfoButton->hide();
	} else {
		ui->moreInfoButton->setTargetUrl(QUrl(more_info_link));
		ui->moreInfoButton->show();
	}
	obs_properties_destroy(props);
}

void OBSBasicSettings::UpdateKeyLink()
{
	QString serviceName = ui->service->currentText();
	QString customServer = ui->customServer->text();
	QString streamKeyLink;
	if (serviceName == "Twitch") {
		streamKeyLink = "https://dashboard.twitch.tv/settings/stream";
	} else if (serviceName.startsWith("YouTube")) {
		streamKeyLink = "https://www.youtube.com/live_dashboard";
	} else if (serviceName.startsWith("Restream.io")) {
		streamKeyLink =
			"https://restream.io/settings/streaming-setup?from=OBS";
	} else if (serviceName == "Facebook Live" ||
		   (customServer.contains("fbcdn.net") && IsCustomService())) {
		streamKeyLink =
			"https://www.facebook.com/live/producer?ref=OBS";
	} else if (serviceName.startsWith("Twitter")) {
		streamKeyLink = "https://www.pscp.tv/account/producer";
	} else if (serviceName.startsWith("YouStreamer")) {
		streamKeyLink = "https://app.youstreamer.com/stream/";
	} else if (serviceName == "Trovo") {
		streamKeyLink = "https://studio.trovo.live/mychannel/stream";
	}

	if (QString(streamKeyLink).isNull()) {
		ui->getStreamKeyButton->hide();
	} else {
		ui->getStreamKeyButton->setTargetUrl(QUrl(streamKeyLink));
		ui->getStreamKeyButton->show();
	}
}

void OBSBasicSettings::LoadServices(bool showAll)
{
	obs_properties_t *props = obs_get_service_properties("rtmp_common");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_bool(settings, "show_all", showAll);

	obs_property_t *prop = obs_properties_get(props, "show_all");
	obs_property_modified(prop, settings);

	ui->service->blockSignals(true);
	ui->service->clear();

	QStringList names;

	obs_property_t *services = obs_properties_get(props, "service");
	size_t services_count = obs_property_list_item_count(services);
	for (size_t i = 0; i < services_count; i++) {
		const char *name = obs_property_list_item_string(services, i);
		names.push_back(name);
	}

	if (showAll)
		names.sort(Qt::CaseInsensitive);

	for (QString &name : names)
		ui->service->addItem(name);

	if (!showAll) {
		ui->service->addItem(
			QTStr("Basic.AutoConfig.StreamPage.Service.ShowAll"),
			QVariant((int)ListOpt::ShowAll));
	}

	ui->service->insertItem(
		0, QTStr("Basic.AutoConfig.StreamPage.Service.Custom"),
		QVariant((int)ListOpt::Custom));

#ifdef ENABLE_ZIXI_SUPPORT
	if (IsZixiPluginLoaded()){
		ui->service->insertItem(
		1, QTStr("Basic.AutoConfig.StreamPage.Service.Zixi"),
		QVariant((int)ListOpt::Zixi));
	}
#endif

	if (!lastService.isEmpty()) {
		int idx = ui->service->findText(lastService);
		if (idx != -1)
			ui->service->setCurrentIndex(idx);
	}

	obs_properties_destroy(props);

	ui->service->blockSignals(false);
}

static inline bool is_auth_service(const std::string &service)
{
	return Auth::AuthType(service) != Auth::Type::None;
}

void OBSBasicSettings::ReloadService()
{
	obs_service_t *service_obj = main->GetService();
	OBSData settings = obs_data_create();
	obs_data_release(settings);
	const char *service_id = IsCustomService() ? "rtmp_custom"
						   : "rtmp_common";
	if (IsZixiService()) {
		service_id = "zixi_service";
	}

	OBSService newService = obs_service_create(service_id, "temp_service",
						   settings, nullptr);
	main->SetService(newService);
	LoadStream1Settings();
}

void OBSBasicSettings::on_service_currentIndexChanged(int)
{
	bool force_load_stream_settings = false;
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;
	if (showMore)
		return;

	bool zixiService = false;
#ifdef ENABLE_ZIXI_SUPPORT
	zixiService = IsZixiService();
	obs_service_t *service_obj = main->GetService();
	const char *type = obs_service_get_type(service_obj);
	bool previousWasZixi = strcmp(type, "zixi_service") == 0;
	if (previousWasZixi != zixiService) {
		force_load_stream_settings = true;
	}
#endif
	ui->server->setVisible(!zixiService);
	ui->serverLabel->setVisible(!zixiService);
	ui->customServer->setVisible(!zixiService);
	ui->key->setVisible(!zixiService);
	ui->streamKeyLabel->setVisible(!zixiService);
	ui->show->setVisible(!zixiService);
	ui->connectAccount2->setVisible(!zixiService);
	ui->authUsernameLabel->setVisible(!zixiService);
	ui->authUsername->setVisible(!zixiService);
	ui->authPwLabel->setVisible(!zixiService);
	ui->authPw->setVisible(!zixiService);
	ui->useAuth->setVisible(!zixiService);
	if (IsZixiPluginLoaded())
		ui->zixiFwd->setVisible(!zixiService);
	else
		ui->zixiFwd->setVisible(false);
	ui->authPwShow->setVisible(!zixiService);
	if (zixiService) {
		ui->zixiFwd->setChecked(true);
		on_zixiFwd_toggled();
		if (force_load_stream_settings)
			ReloadService();
		return;
	}

	std::string service = QT_TO_UTF8(ui->service->currentText());
	bool custom = IsCustomService();

	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);

#ifdef BROWSER_AVAILABLE
	if (cef) {
		if (lastService != service.c_str()) {
			QString key = ui->key->text();
			bool can_auth = is_auth_service(service);
			int page = can_auth && (!loading || key.isEmpty())
					   ? (int)Section::Connect
					   : (int)Section::StreamKey;

			ui->streamStackWidget->setCurrentIndex(page);
			ui->streamKeyWidget->setVisible(true);
			ui->streamKeyLabel->setVisible(true);
			ui->connectAccount2->setVisible(can_auth);
		}
	} else {
		ui->connectAccount2->setVisible(false);
	}
#else
	ui->connectAccount2->setVisible(false);
#endif

	ui->useAuth->setVisible(custom);
	ui->authUsernameLabel->setVisible(custom);
	ui->authUsername->setVisible(custom);
	ui->authPwLabel->setVisible(custom);
	ui->authPwWidget->setVisible(custom);

	if (custom) {
		ui->streamkeyPageLayout->insertRow(1, ui->serverLabel,
						   ui->serverStackedWidget);

		ui->serverStackedWidget->setCurrentIndex(1);
		ui->serverStackedWidget->setVisible(true);
		ui->serverLabel->setVisible(true);
		on_useAuth_toggled();
	} else {
		ui->serverStackedWidget->setCurrentIndex(0);
	}

#ifdef BROWSER_AVAILABLE
	auth.reset();

	if (!!main->auth &&
	    service.find(main->auth->service()) != std::string::npos) {
		auth = main->auth;
		OnAuthConnected();
	}
#endif

	if (force_load_stream_settings) {
		//ReloadService();
	}
}

void OBSBasicSettings::UpdateServerList()
{
	QString serviceName = ui->service->currentText();
	bool showMore = ui->service->currentData().toInt() ==
			(int)ListOpt::ShowAll;

	if (showMore) {
		LoadServices(true);
		ui->service->showPopup();
		return;
	} else {
		lastService = serviceName;
	}

	obs_properties_t *props = obs_get_service_properties("rtmp_common");
	obs_property_t *services = obs_properties_get(props, "service");

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	obs_data_set_string(settings, "service", QT_TO_UTF8(serviceName));
	obs_property_modified(services, settings);

	obs_property_t *servers = obs_properties_get(props, "server");

	ui->server->clear();

	size_t servers_count = obs_property_list_item_count(servers);
	for (size_t i = 0; i < servers_count; i++) {
		const char *name = obs_property_list_item_name(servers, i);
		const char *server = obs_property_list_item_string(servers, i);
		ui->server->addItem(name, server);
	}

	obs_properties_destroy(props);
}

void OBSBasicSettings::on_show_clicked()
{
	if (ui->key->echoMode() == QLineEdit::Password) {
		ui->key->setEchoMode(QLineEdit::Normal);
		ui->show->setText(QTStr("Hide"));
	} else {
		ui->key->setEchoMode(QLineEdit::Password);
		ui->show->setText(QTStr("Show"));
	}
}

void OBSBasicSettings::on_authPwShow_clicked()
{
	if (ui->authPw->echoMode() == QLineEdit::Password) {
		ui->authPw->setEchoMode(QLineEdit::Normal);
		ui->authPwShow->setText(QTStr("Hide"));
	} else {
		ui->authPw->setEchoMode(QLineEdit::Password);
		ui->authPwShow->setText(QTStr("Show"));
	}
}

OBSService OBSBasicSettings::SpawnTempService()
{
	bool custom = IsCustomService();
	const char *service_id = custom ? "rtmp_custom" : "rtmp_common";

	OBSData settings = obs_data_create();
	obs_data_release(settings);

	if (!custom) {
		obs_data_set_string(settings, "service",
				    QT_TO_UTF8(ui->service->currentText()));
		obs_data_set_string(
			settings, "server",
			QT_TO_UTF8(ui->server->currentData().toString()));
	} else {
		obs_data_set_string(settings, "server",
				    QT_TO_UTF8(ui->customServer->text()));
	}
	obs_data_set_string(settings, "key", QT_TO_UTF8(ui->key->text()));

	OBSService newService = obs_service_create(service_id, "temp_service",
						   settings, nullptr);
	obs_service_release(newService);

	return newService;
}

void OBSBasicSettings::OnOAuthStreamKeyConnected()
{
#ifdef BROWSER_AVAILABLE
	OAuthStreamKey *a = reinterpret_cast<OAuthStreamKey *>(auth.get());

	if (a) {
		bool validKey = !a->key().empty();

		if (validKey)
			ui->key->setText(QT_UTF8(a->key().c_str()));

		ui->streamKeyWidget->setVisible(false);
		ui->streamKeyLabel->setVisible(false);
		ui->connectAccount2->setVisible(false);
		ui->disconnectAccount->setVisible(true);

		if (strcmp(a->service(), "Twitch") == 0) {
			ui->bandwidthTestEnable->setVisible(true);
			ui->twitchAddonLabel->setVisible(true);
			ui->twitchAddonDropdown->setVisible(true);
		} else {
			ui->bandwidthTestEnable->setChecked(false);
		}
	}

	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
#endif
}

void OBSBasicSettings::OnAuthConnected()
{
	std::string service = QT_TO_UTF8(ui->service->currentText());
	Auth::Type type = Auth::AuthType(service);

	if (type == Auth::Type::OAuth_StreamKey) {
		OnOAuthStreamKeyConnected();
	}

	if (!loading) {
		stream1Changed = true;
		EnableApplyButton(true);
	}
}

void OBSBasicSettings::on_connectAccount_clicked()
{
#ifdef BROWSER_AVAILABLE
	std::string service = QT_TO_UTF8(ui->service->currentText());

	OAuth::DeleteCookies(service);

	auth = OAuthStreamKey::Login(this, service);
	if (!!auth)
		OnAuthConnected();
#endif
}

#define DISCONNECT_COMFIRM_TITLE \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Title"
#define DISCONNECT_COMFIRM_TEXT \
	"Basic.AutoConfig.StreamPage.DisconnectAccount.Confirm.Text"

void OBSBasicSettings::on_disconnectAccount_clicked()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr(DISCONNECT_COMFIRM_TITLE),
					 QTStr(DISCONNECT_COMFIRM_TEXT));

	if (button == QMessageBox::No) {
		return;
	}

	main->auth.reset();
	auth.reset();

	std::string service = QT_TO_UTF8(ui->service->currentText());

#ifdef BROWSER_AVAILABLE
	OAuth::DeleteCookies(service);
#endif

	ui->bandwidthTestEnable->setChecked(false);

	ui->streamKeyWidget->setVisible(true);
	ui->streamKeyLabel->setVisible(true);
	ui->connectAccount2->setVisible(true);
	ui->disconnectAccount->setVisible(false);
	ui->bandwidthTestEnable->setVisible(false);
	ui->twitchAddonDropdown->setVisible(false);
	ui->twitchAddonLabel->setVisible(false);
	ui->key->setText("");
}

void OBSBasicSettings::on_useStreamKey_clicked()
{
	ui->streamStackWidget->setCurrentIndex((int)Section::StreamKey);
}

void OBSBasicSettings::on_useAuth_toggled()
{
	if (!IsCustomService())
		return;

	bool use_auth = ui->useAuth->isChecked();

	ui->authUsernameLabel->setVisible(use_auth);
	ui->authUsername->setVisible(use_auth);
	ui->authPwLabel->setVisible(use_auth);
	ui->authPwWidget->setVisible(use_auth);
}

void OBSBasicSettings::UpdateVodTrackSetting()
{
	bool enableForCustomServer = config_get_bool(
		GetGlobalConfig(), "General", "EnableCustomServerVodTrack");
	bool enableVodTrack = ui->service->currentText() == "Twitch";
	bool wasEnabled = !!vodTrackCheckbox;

	if (enableForCustomServer && IsCustomService())
		enableVodTrack = true;

	if (enableVodTrack == wasEnabled)
		return;

	if (!enableVodTrack) {
		delete vodTrackCheckbox;
		delete vodTrackContainer;
		delete simpleVodTrack;
		return;
	}

	/* -------------------------------------- */
	/* simple output mode vod track widgets   */

	bool simpleAdv = ui->simpleOutAdvanced->isChecked();
	bool vodTrackEnabled = config_get_bool(main->Config(), "SimpleOutput",
					       "VodTrackEnabled");

	simpleVodTrack = new QCheckBox(this);
	simpleVodTrack->setText(
		QTStr("Basic.Settings.Output.Simple.TwitchVodTrack"));
	simpleVodTrack->setVisible(simpleAdv);
	simpleVodTrack->setChecked(vodTrackEnabled);

	int pos;
	ui->simpleStreamingLayout->getWidgetPosition(ui->simpleOutAdvanced,
						     &pos, nullptr);
	ui->simpleStreamingLayout->insertRow(pos + 1, nullptr, simpleVodTrack);

	HookWidget(simpleVodTrack, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));
	connect(ui->simpleOutAdvanced, SIGNAL(toggled(bool)),
		simpleVodTrack.data(), SLOT(setVisible(bool)));

	/* -------------------------------------- */
	/* advanced output mode vod track widgets */

	vodTrackCheckbox = new QCheckBox(this);
	vodTrackCheckbox->setText(
		QTStr("Basic.Settings.Output.Adv.TwitchVodTrack"));
	vodTrackCheckbox->setLayoutDirection(Qt::RightToLeft);

	vodTrackContainer = new QWidget(this);
	QHBoxLayout *vodTrackLayout = new QHBoxLayout();
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i] = new QRadioButton(QString::number(i + 1));
		vodTrackLayout->addWidget(vodTrack[i]);

		HookWidget(vodTrack[i], SIGNAL(clicked(bool)),
			   SLOT(OutputsChanged()));
	}

	HookWidget(vodTrackCheckbox, SIGNAL(clicked(bool)),
		   SLOT(OutputsChanged()));

	vodTrackLayout->addStretch();
	vodTrackLayout->setContentsMargins(0, 0, 0, 0);

	vodTrackContainer->setLayout(vodTrackLayout);

	ui->advOutTopLayout->insertRow(2, vodTrackCheckbox, vodTrackContainer);

	vodTrackEnabled =
		config_get_bool(main->Config(), "AdvOut", "VodTrackEnabled");
	vodTrackCheckbox->setChecked(vodTrackEnabled);
	vodTrackContainer->setEnabled(vodTrackEnabled);

	connect(vodTrackCheckbox, SIGNAL(clicked(bool)), vodTrackContainer,
		SLOT(setEnabled(bool)));

	int trackIndex =
		config_get_int(main->Config(), "AdvOut", "VodTrackIndex");
	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		vodTrack[i]->setChecked((i + 1) == trackIndex);
	}
}

OBSService OBSBasicSettings::GetStream1Service()
{
	return stream1Changed ? SpawnTempService()
			      : OBSService(main->GetService());
}

void OBSBasicSettings::UpdateServiceRecommendations()
{
	bool customServer = IsCustomService();
	ui->ignoreRecommended->setVisible(!customServer);
	ui->enforceSettingsLabel->setVisible(!customServer);

	OBSService service = GetStream1Service();

	int vbitrate, abitrate;
	BPtr<obs_service_resolution> res_list;
	size_t res_count;
	int fps;

	obs_service_get_max_bitrate(service, &vbitrate, &abitrate);
	obs_service_get_supported_resolutions(service, &res_list, &res_count);
	obs_service_get_max_fps(service, &fps);

	QString text;

#define ENFORCE_TEXT(x) QTStr("Basic.Settings.Stream.Recommended." x)
	if (vbitrate)
		text += ENFORCE_TEXT("MaxVideoBitrate")
				.arg(QString::number(vbitrate));
	if (abitrate) {
		if (!text.isEmpty())
			text += "\n";
		text += ENFORCE_TEXT("MaxAudioBitrate")
				.arg(QString::number(abitrate));
	}
	if (res_count) {
		if (!text.isEmpty())
			text += "\n";

		obs_service_resolution best_res = {};
		int best_res_pixels = 0;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution res = res_list[i];
			int res_pixels = res.cx + res.cy;
			if (res_pixels > best_res_pixels) {
				best_res = res;
				best_res_pixels = res_pixels;
			}
		}

		QString res_str =
			QString("%1x%2").arg(QString::number(best_res.cx),
					     QString::number(best_res.cy));
		text += ENFORCE_TEXT("MaxResolution").arg(res_str);
	}
	if (fps) {
		if (!text.isEmpty())
			text += "\n";

		text += ENFORCE_TEXT("MaxFPS").arg(QString::number(fps));
	}
#undef ENFORCE_TEXT

	ui->enforceSettingsLabel->setText(text);
}

void OBSBasicSettings::DisplayEnforceWarning(bool checked)
{
	if (IsCustomService())
		return;

	if (!checked) {
		SimpleRecordingEncoderChanged();
		return;
	}

	QMessageBox::StandardButton button;

#define ENFORCE_WARNING(x) \
	QTStr("Basic.Settings.Stream.IgnoreRecommended.Warn." x)

	button = OBSMessageBox::question(this, ENFORCE_WARNING("Title"),
					 ENFORCE_WARNING("Text"));
#undef ENFORCE_WARNING

	if (button == QMessageBox::No) {
		QMetaObject::invokeMethod(ui->ignoreRecommended, "setChecked",
					  Qt::QueuedConnection,
					  Q_ARG(bool, false));
		return;
	}

	SimpleRecordingEncoderChanged();
}

bool OBSBasicSettings::ResFPSValid(obs_service_resolution *res_list,
				   size_t res_count, int max_fps)
{
	if (!res_count && !max_fps)
		return true;

	if (res_count) {
		QString res = ui->outputResolution->currentText();
		bool found_res = false;

		int cx, cy;
		if (sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy) != 2)
			return false;

		for (size_t i = 0; i < res_count; i++) {
			if (res_list[i].cx == cx && res_list[i].cy == cy) {
				found_res = true;
				break;
			}
		}

		if (!found_res)
			return false;
	}

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();
		if (fpsType != 0)
			return false;

		std::string fps_str = QT_TO_UTF8(ui->fpsCommon->currentText());
		float fps;
		sscanf(fps_str.c_str(), "%f", &fps);
		if (fps > (float)max_fps)
			return false;
	}

	return true;
}

extern void set_closest_res(int &cx, int &cy,
			    struct obs_service_resolution *res_list,
			    size_t count);

/* Checks for and updates the resolution and FPS limits of a service, if any.
 *
 * If the service has a resolution and/or FPS limit, this will enforce those
 * limitations in the UI itself, preventing the user from selecting a
 * resolution or FPS that's not supported.
 *
 * This is an unpleasant thing to have to do to users, but there is no other
 * way to ensure that a service's restricted resolution/framerate values are
 * properly enforced, otherwise users will just be confused when things aren't
 * working correctly. The user can turn it off if they're partner (or if they
 * want to risk getting in trouble with their service) by selecting the "Ignore
 * recommended settings" option in the stream section of settings.
 *
 * This only affects services that have a resolution and/or framerate limit, of
 * which as of this writing, and hopefully for the foreseeable future, there is
 * only one.
 */
void OBSBasicSettings::UpdateResFPSLimits()
{
	if (loading)
		return;

	int idx = ui->service->currentIndex();
	if (idx == -1)
		return;

	bool ignoreRecommended = ui->ignoreRecommended->isChecked();
	BPtr<obs_service_resolution> res_list;
	size_t res_count = 0;
	int max_fps = 0;

	if (!IsCustomService() && !ignoreRecommended) {
		OBSService service = GetStream1Service();
		obs_service_get_supported_resolutions(service, &res_list,
						      &res_count);
		obs_service_get_max_fps(service, &max_fps);
	}

	/* ------------------------------------ */
	/* Check for enforced res/FPS           */

	QString res = ui->outputResolution->currentText();
	QString fps_str;
	int cx = 0, cy = 0;
	double max_fpsd = (double)max_fps;
	int closest_fps_index = -1;
	double fpsd;

	sscanf(QT_TO_UTF8(res), "%dx%d", &cx, &cy);

	if (res_count)
		set_closest_res(cx, cy, res_list, res_count);

	if (max_fps) {
		int fpsType = ui->fpsType->currentIndex();

		if (fpsType == 1) { //Integer
			fpsd = (double)ui->fpsInteger->value();
		} else if (fpsType == 2) { //Fractional
			fpsd = (double)ui->fpsNumerator->value() /
			       (double)ui->fpsDenominator->value();
		} else { //Common
			sscanf(QT_TO_UTF8(ui->fpsCommon->currentText()), "%lf",
			       &fpsd);
		}

		double closest_diff = 1000000000000.0;

		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				continue;
			}

			double diff = fabs(com_fpsd - fpsd);
			if (diff < closest_diff) {
				closest_diff = diff;
				closest_fps_index = i;
				fps_str = ui->fpsCommon->itemText(i);
			}
		}
	}

	QString res_str =
		QString("%1x%2").arg(QString::number(cx), QString::number(cy));

	/* ------------------------------------ */
	/* Display message box if res/FPS bad   */

	bool valid = ResFPSValid(res_list, res_count, max_fps);

	if (!valid) {
		/* if the user was already on facebook with an incompatible
		 * resolution, assume it's an upgrade */
		if (lastServiceIdx == -1 && lastIgnoreRecommended == -1) {
			ui->ignoreRecommended->setChecked(true);
			ui->ignoreRecommended->setProperty("changed", true);
			stream1Changed = true;
			EnableApplyButton(true);
			UpdateResFPSLimits();
			return;
		}

		QMessageBox::StandardButton button;

#define WARNING_VAL(x) \
	QTStr("Basic.Settings.Output.Warn.EnforceResolutionFPS." x)

		QString str;
		if (res_count)
			str += WARNING_VAL("Resolution").arg(res_str);
		if (max_fps) {
			if (!str.isEmpty())
				str += "\n";
			str += WARNING_VAL("FPS").arg(fps_str);
		}

		button = OBSMessageBox::question(this, WARNING_VAL("Title"),
						 WARNING_VAL("Msg").arg(str));
#undef WARNING_VAL

		if (button == QMessageBox::No) {
			if (idx != lastServiceIdx)
				QMetaObject::invokeMethod(
					ui->service, "setCurrentIndex",
					Qt::QueuedConnection,
					Q_ARG(int, lastServiceIdx));
			else
				QMetaObject::invokeMethod(ui->ignoreRecommended,
							  "setChecked",
							  Qt::QueuedConnection,
							  Q_ARG(bool, true));
			return;
		}
	}

	/* ------------------------------------ */
	/* Update widgets/values if switching   */
	/* to/from enforced resolution/FPS      */

	ui->outputResolution->blockSignals(true);
	if (res_count) {
		ui->outputResolution->clear();
		ui->outputResolution->setEditable(false);

		int new_res_index = -1;

		for (size_t i = 0; i < res_count; i++) {
			obs_service_resolution val = res_list[i];
			QString str =
				QString("%1x%2").arg(QString::number(val.cx),
						     QString::number(val.cy));
			ui->outputResolution->addItem(str);

			if (val.cx == cx && val.cy == cy)
				new_res_index = (int)i;
		}

		ui->outputResolution->setCurrentIndex(new_res_index);
		if (!valid) {
			ui->outputResolution->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		QString baseRes = ui->baseResolution->currentText();
		int baseCX, baseCY;
		sscanf(QT_TO_UTF8(baseRes), "%dx%d", &baseCX, &baseCY);

		if (!ui->outputResolution->isEditable()) {
			RecreateOutputResolutionWidget();
			ui->outputResolution->blockSignals(true);
			ResetDownscales((uint32_t)baseCX, (uint32_t)baseCY,
					true);
			ui->outputResolution->setCurrentText(res);
		}
	}
	ui->outputResolution->blockSignals(false);

	if (max_fps) {
		for (int i = 0; i < ui->fpsCommon->count(); i++) {
			double com_fpsd;
			sscanf(QT_TO_UTF8(ui->fpsCommon->itemText(i)), "%lf",
			       &com_fpsd);

			if (com_fpsd > max_fpsd) {
				SetComboItemEnabled(ui->fpsCommon, i, false);
				continue;
			}
		}

		ui->fpsType->setCurrentIndex(0);
		ui->fpsCommon->setCurrentIndex(closest_fps_index);
		if (!valid) {
			ui->fpsType->setProperty("changed", true);
			ui->fpsCommon->setProperty("changed", true);
			videoChanged = true;
			EnableApplyButton(true);
		}
	} else {
		for (int i = 0; i < ui->fpsCommon->count(); i++)
			SetComboItemEnabled(ui->fpsCommon, i, true);
	}

	SetComboItemEnabled(ui->fpsType, 1, !max_fps);
	SetComboItemEnabled(ui->fpsType, 2, !max_fps);

	/* ------------------------------------ */

	lastIgnoreRecommended = (int)ignoreRecommended;
	lastServiceIdx = idx;
}

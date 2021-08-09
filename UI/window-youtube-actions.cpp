#include "window-basic-main.hpp"
#include "window-youtube-actions.hpp"

#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "youtube-api-wrappers.hpp"

#include <QDateTime>
#include <QDesktopServices>

const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";
const QString RepresentSchedulDateAndTimeFormat = "dddd, MMMM d, yyyy h:m";
const QString NormalStylesheet = "border: 1px solid black; border-radius: 5px;";
const QString SelectedStylesheet =
	"border: 2px solid black; border-radius: 5px;";
const QString IndexOfGamingCategory = "20";

OBSYoutubeActions::OBSYoutubeActions(QWidget *parent, Auth *auth)
	: QDialog(parent),
	  ui(new Ui::OBSYoutubeActions),
	  apiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth)),
	  workerThread(new WorkerThread(apiYouTube))
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->setupUi(this);

	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Public"),
				"public");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Unlisted"),
				"unlisted");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Private"),
				"private");

	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Normal"),
				"normal");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Low"), "low");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.UltraLow"),
				"ultraLow");

	ui->checkAutoStart->setEnabled(false);
	ui->checkAutoStop->setEnabled(false);

	UpdateOkButtonStatus();

	connect(ui->title, &QLineEdit::textChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->privacyBox, &QComboBox::currentTextChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->yesMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->notMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->tabWidget, &QTabWidget::currentChanged, this,
		[&](int) { this->UpdateOkButtonStatus(); });
	connect(ui->pushButton, &QPushButton::clicked, this,
		&OBSYoutubeActions::OpenYouTubeDashboard);

	connect(ui->helpAutoStartStop, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });
	connect(ui->help360Video, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });
	connect(ui->helpMadeForKids, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });

	ui->scheduledTime->setVisible(false);
	connect(ui->checkScheduledLater, &QCheckBox::stateChanged, this,
		[&](int state) {
			ui->scheduledTime->setVisible(state);
			if (state) {
				ui->checkAutoStart->setEnabled(true);
				ui->checkAutoStop->setEnabled(true);

				ui->checkAutoStart->setChecked(false);
				ui->checkAutoStop->setChecked(false);
			} else {
				ui->checkAutoStart->setEnabled(false);
				ui->checkAutoStop->setEnabled(false);

				ui->checkAutoStart->setChecked(true);
				ui->checkAutoStop->setChecked(true);
			}
			UpdateOkButtonStatus();
		});

	ui->scheduledTime->setDateTime(QDateTime::currentDateTime());

	if (!apiYouTube) {
		blog(LOG_DEBUG, "YouTube API auth NOT found.");
		Cancel();
		return;
	}
	ChannelDescription channel;
	if (!apiYouTube->GetChannelDescription(channel)) {
		blog(LOG_DEBUG, "Could not get channel description.");
		ShowErrorDialog(
			parent,
			apiYouTube->GetLastError().isEmpty()
				? QTStr("YouTube.Actions.Error.General")
				: QTStr("YouTube.Actions.Error.Text")
					  .arg(apiYouTube->GetLastError()));
		Cancel();
		return;
	}
	this->setWindowTitle(channel.title);

	QVector<CategoryDescription> category_list;
	if (!apiYouTube->GetVideoCategoriesList(
		    channel.country, channel.language, category_list)) {
		blog(LOG_DEBUG, "Could not get video category for country; %s.",
		     channel.country.toStdString().c_str());
		ShowErrorDialog(
			parent,
			apiYouTube->GetLastError().isEmpty()
				? QTStr("YouTube.Actions.Error.General")
				: QTStr("YouTube.Actions.Error.Text")
					  .arg(apiYouTube->GetLastError()));
		Cancel();
		return;
	}
	for (auto &category : category_list) {
		ui->categoryBox->addItem(category.title, category.id);
		if (category.id == IndexOfGamingCategory) {
			ui->categoryBox->setCurrentText(category.title);
		}
	}

	connect(ui->okButton, &QPushButton::clicked, this,
		&OBSYoutubeActions::InitBroadcast);
	connect(ui->cancelButton, &QPushButton::clicked, this, [&]() {
		blog(LOG_DEBUG, "YouTube live broadcast creation cancelled.");
		// Close the dialog.
		Cancel();
	});

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(
		QString(), Qt::FindDirectChildrenOnly));

	connect(workerThread, &WorkerThread::failed, this, &QDialog::reject);

	connect(workerThread, &WorkerThread::new_item, this,
		[&](const QString &title, const QString &dateTimeString,
		    const QString &broadcast, bool astart, bool astop) {
			ClickableLabel *label = new ClickableLabel();
			label->setStyleSheet(NormalStylesheet);
			label->setTextFormat(Qt::RichText);
			label->setText(
				QString("<big>%1 %2</big><br/>%3 %4")
					.arg(title,
					     QTStr("YouTube.Actions.Stream"),
					     QTStr("YouTube.Actions.Stream.ScheduledFor"),
					     dateTimeString));
			label->setAlignment(Qt::AlignHCenter);
			label->setMargin(4);

			connect(label, &ClickableLabel::clicked, this,
				[&, label, broadcast, astart, astop]() {
					for (QWidget *i :
					     ui->scrollAreaWidgetContents->findChildren<
						     QWidget *>(
						     QString(),
						     Qt::FindDirectChildrenOnly))
						i->setStyleSheet(
							NormalStylesheet);
					label->setStyleSheet(
						SelectedStylesheet);

					this->selectedBroadcast = broadcast;
					this->autostart = astart;
					this->autostop = astop;
					UpdateOkButtonStatus();
				});
			ui->scrollAreaWidgetContents->layout()->addWidget(
				label);
		});
	workerThread->start();

#ifdef __APPLE__
	// MacOS theming issues
	this->resize(this->width() + 200, this->height() + 120);
#endif
	valid = true;
}

OBSYoutubeActions::~OBSYoutubeActions()
{
	workerThread->stop();
	workerThread->wait();

	delete workerThread;
}

void WorkerThread::run()
{
	if (!pending)
		return;
	json11::Json broadcasts;
	if (!apiYouTube->GetBroadcastsList(broadcasts, "")) {
		emit failed();
		return;
	}

	while (pending) {
		auto items = broadcasts["items"].array_items();
		for (auto item = items.begin(); item != items.end(); item++) {
			auto status = (*item)["status"]["lifeCycleStatus"]
					      .string_value();
			if (status == "created" || status == "ready") {
				auto title = QString::fromStdString(
					(*item)["snippet"]["title"]
						.string_value());
				auto scheduledStartTime = QString::fromStdString(
					(*item)["snippet"]["scheduledStartTime"]
						.string_value());
				auto broadcast = QString::fromStdString(
					(*item)["id"].string_value());
				auto astart = (*item)["contentDetails"]
						     ["enableAutoStart"]
							     .bool_value();
				auto astop = (*item)["contentDetails"]
						    ["enableAutoStop"]
							    .bool_value();

				auto utcDTime = QDateTime::fromString(
					scheduledStartTime,
					SchedulDateAndTimeFormat);
				// DateTime parser means that input datetime is a local, so we need to move it
				auto dateTime = utcDTime.addSecs(
					utcDTime.offsetFromUtc());
				auto dateTimeString = QLocale().toString(
					dateTime,
					QString("%1  %2").arg(
						QLocale().dateFormat(
							QLocale::LongFormat),
						QLocale().timeFormat(
							QLocale::ShortFormat)));

				emit new_item(title, dateTimeString, broadcast,
					      astart, astop);
			}
		}

		auto nextPageToken = broadcasts["nextPageToken"].string_value();
		if (nextPageToken.empty() || items.empty())
			break;
		else {
			if (!pending)
				return;
			if (!apiYouTube->GetBroadcastsList(
				    broadcasts,
				    QString::fromStdString(nextPageToken))) {
				emit failed();
				return;
			}
		}
	}

	emit ready();
}

void OBSYoutubeActions::UpdateOkButtonStatus()
{
	if (ui->tabWidget->currentIndex() == 0) {
		ui->okButton->setEnabled(
			!ui->title->text().isEmpty() &&
			!ui->privacyBox->currentText().isEmpty() &&
			(ui->yesMakeForKids->isChecked() ||
			 ui->notMakeForKids->isChecked()));
		if (ui->checkScheduledLater->checkState() == Qt::Checked) {
			ui->okButton->setText(
				QTStr("YouTube.Actions.Create_Save"));
		} else {
			ui->okButton->setText(
				QTStr("YouTube.Actions.Create_GoLive"));
		}

		ui->pushButton->setVisible(false);
	} else {
		ui->okButton->setEnabled(!selectedBroadcast.isEmpty());
		ui->okButton->setText(QTStr("YouTube.Actions.Choose_GoLive"));

		ui->pushButton->setVisible(true);
	}
}

bool OBSYoutubeActions::StreamNowAction(YoutubeApiWrappers *api,
					StreamDescription &stream)
{
	YoutubeApiWrappers *apiYouTube = api;
	BroadcastDescription broadcast = {};
	UiToBroadcast(broadcast);
	// stream now is always autostart/autostop
	broadcast.auto_start = true;
	broadcast.auto_stop = true;

	blog(LOG_DEBUG, "Scheduled date and time: %s",
	     broadcast.schedul_date_time.toStdString().c_str());
	if (!apiYouTube->InsertBroadcast(broadcast)) {
		blog(LOG_DEBUG, "No broadcast created.");
		return false;
	}
	stream = {"", "", "OBS Studio Video Stream", ""};
	if (!apiYouTube->InsertStream(stream)) {
		blog(LOG_DEBUG, "No stream created.");
		return false;
	}
	if (!apiYouTube->BindStream(broadcast.id, stream.id)) {
		blog(LOG_DEBUG, "No stream binded.");
		return false;
	}
	if (!apiYouTube->SetVideoCategory(broadcast.id, broadcast.title,
					  broadcast.description,
					  broadcast.category.id)) {
		blog(LOG_DEBUG, "No category set.");
		return false;
	}
	return true;
}

bool OBSYoutubeActions::StreamLaterAction(YoutubeApiWrappers *api)
{
	YoutubeApiWrappers *apiYouTube = api;
	BroadcastDescription broadcast = {};
	UiToBroadcast(broadcast);

	// DateTime parser means that input datetime is a local, so we need to move it
	auto dateTime = ui->scheduledTime->dateTime();
	auto utcDTime = dateTime.addSecs(-dateTime.offsetFromUtc());
	broadcast.schedul_date_time =
		utcDTime.toString(SchedulDateAndTimeFormat);

	blog(LOG_DEBUG, "Scheduled date and time: %s",
	     broadcast.schedul_date_time.toStdString().c_str());
	if (!apiYouTube->InsertBroadcast(broadcast)) {
		blog(LOG_DEBUG, "No broadcast created.");
		return false;
	}
	return true;
}

bool OBSYoutubeActions::ChooseAnEventAction(YoutubeApiWrappers *api,
					    StreamDescription &stream,
					    bool start)
{
	YoutubeApiWrappers *apiYouTube = api;

	std::string boundStreamId;
	{
		json11::Json json;
		if (!apiYouTube->FindBroadcast(selectedBroadcast, json)) {
			blog(LOG_DEBUG, "No broadcast found.");
			return false;
		}

		auto item = json["items"].array_items()[0];
		auto boundStreamId =
			item["contentDetails"]["boundStreamId"].string_value();
	}

	stream.id = boundStreamId.c_str();
	json11::Json json;
	if (!stream.id.isEmpty() && apiYouTube->FindStream(stream.id, json)) {
		auto item = json["items"].array_items()[0];
		auto streamName = item["cdn"]["streamName"].string_value();
		auto title = item["snippet"]["title"].string_value();
		auto description =
			item["snippet"]["description"].string_value();

		stream.name = streamName.c_str();
		stream.title = title.c_str();
		stream.description = description.c_str();
	} else {
		stream = {"", "", "OBS Studio Video Stream", ""};
		if (!apiYouTube->InsertStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}
		if (!apiYouTube->BindStream(selectedBroadcast, stream.id)) {
			blog(LOG_DEBUG, "No stream binded.");
			return false;
		}
	}

	if (start)
		api->StartBroadcast(selectedBroadcast);
	return true;
}

void OBSYoutubeActions::ShowErrorDialog(QWidget *parent, QString text)
{
	QMessageBox dlg(parent);
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(QTStr("YouTube.Actions.Error.Title"));
	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setIcon(QMessageBox::Warning);
	dlg.setStandardButtons(QMessageBox::StandardButton::Ok);
	dlg.exec();
}

void OBSYoutubeActions::InitBroadcast()
{
	StreamDescription stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() &
			      ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(QTStr("YouTube.Actions.Notify.Title"));
	msgBox.setText(QTStr("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			if (ui->checkScheduledLater->isChecked()) {
				success = this->StreamLaterAction(apiYouTube);
			} else {
				success = this->StreamNowAction(apiYouTube,
								stream);
			}
		} else {
			success = this->ChooseAnEventAction(apiYouTube, stream,
							    this->autostart);
		};
		QMetaObject::invokeMethod(&msgBox, "accept",
					  Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(CreateQThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
		if (ui->tabWidget->currentIndex() == 0) {
			// Stream later usecase.
			if (ui->checkScheduledLater->isChecked()) {
				QMessageBox msg(this);
				msg.setWindowTitle(QTStr(
					"YouTube.Actions.EventCreated.Title"));
				msg.setText(QTStr(
					"YouTube.Actions.EventCreated.Text"));
				msg.setStandardButtons(QMessageBox::Ok);
				msg.exec();
				// Close dialog without start streaming.
				Cancel();
			} else {
				// Stream now usecase.
				blog(LOG_DEBUG, "New valid stream: %s",
				     QT_TO_UTF8(stream.name));
				emit ok(QT_TO_UTF8(stream.id),
					QT_TO_UTF8(stream.name), true, true);
				Accept();
			}
		} else {
			// Stream to precreated broadcast usecase.
			emit ok(QT_TO_UTF8(stream.id), QT_TO_UTF8(stream.name),
				autostart, autostop);
			Accept();
		}
	} else {
		// Fail.
		auto last_error = apiYouTube->GetLastError();
		if (last_error.isEmpty()) {
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
		}
		ShowErrorDialog(
			this, QTStr("YouTube.Actions.Error.NoBroadcastCreated")
				      .arg(last_error));
	}
}

void OBSYoutubeActions::UiToBroadcast(BroadcastDescription &broadcast)
{
	broadcast.title = ui->title->text();
	broadcast.description = ui->description->text();
	broadcast.privacy = ui->privacyBox->currentData().toString();
	broadcast.category.title = ui->categoryBox->currentText();
	broadcast.category.id = ui->categoryBox->currentData().toString();
	broadcast.made_for_kids = ui->yesMakeForKids->isChecked();
	broadcast.latency = ui->latencyBox->currentData().toString();
	broadcast.auto_start = ui->checkAutoStart->isChecked();
	broadcast.auto_stop = ui->checkAutoStop->isChecked();
	broadcast.dvr = ui->checkDVR->isChecked();
	broadcast.schedul_for_later = ui->checkScheduledLater->isChecked();
	broadcast.projection = ui->check360Video->isChecked() ? "360"
							      : "rectangular";
	// Current time by default.
	broadcast.schedul_date_time = QDateTime::currentDateTimeUtc().toString(
		SchedulDateAndTimeFormat);
}

void OBSYoutubeActions::OpenYouTubeDashboard()
{
	ChannelDescription channel;
	if (!apiYouTube->GetChannelDescription(channel)) {
		blog(LOG_DEBUG, "Could not get channel description.");
		ShowErrorDialog(
			this,
			apiYouTube->GetLastError().isEmpty()
				? QTStr("YouTube.Actions.Error.General")
				: QTStr("YouTube.Actions.Error.Text")
					  .arg(apiYouTube->GetLastError()));
		return;
	}

	//https://studio.youtube.com/channel/UCA9bSfH3KL186kyiUsvi3IA/videos/live?filter=%5B%5D&sort=%7B%22columnType%22%3A%22date%22%2C%22sortOrder%22%3A%22DESCENDING%22%7D
	QString uri =
		QString("https://studio.youtube.com/channel/%1/videos/live?filter=[]&sort={\"columnType\"%3A\"date\"%2C\"sortOrder\"%3A\"DESCENDING\"}")
			.arg(channel.id);
	QDesktopServices::openUrl(uri);
}

void OBSYoutubeActions::Cancel()
{
	workerThread->stop();
	reject();
}
void OBSYoutubeActions::Accept()
{
	workerThread->stop();
	accept();
}

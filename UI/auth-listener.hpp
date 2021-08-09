#pragma once

#include <QObject>
#include <QtNetwork/QTcpServer>

class AuthListener : public QObject {
	Q_OBJECT

	QTcpServer *server;

signals:
	void ok(const QString &code);
	void fail();

protected:
	void NewConnection();

public:
	explicit AuthListener(QObject *parent = 0);
	quint16 GetPort();
};

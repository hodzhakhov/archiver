#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QHttpMultiPart>

class NetworkWorker : public QObject {
    Q_OBJECT

public:
    NetworkWorker(QObject *parent = nullptr);
    ~NetworkWorker();

    public slots:
        void processRequest(const QNetworkRequest &request, const QByteArray &data = QByteArray());
        void processMultipartRequest(const QNetworkRequest &request, QHttpMultiPart *multipart);
    void cancelRequest();

    signals:
        void dataReceived(const QByteArray &data);
    void finished();
    void errorOccurred(const QString &error);

    private slots:
        void onReadyRead();
    void onFinished();

private:
    QScopedPointer<QNetworkAccessManager, QScopedPointerDeleter<QNetworkAccessManager>> m_qnam;
    QScopedPointer<QNetworkReply, QScopedPointerDeleter<QNetworkReply>> m_reply;
    QHttpMultiPart *m_currentMultipart;
};
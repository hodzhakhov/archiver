#pragma once

#include "network_worker.h"

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QListWidget>
#include <QProgressBar>
#include <QGroupBox>
#include <QRadioButton>
#include <QTabWidget>
#include <QAbstractItemView>
#include <QHttpMultiPart>

class QNetworkRequest;

struct ExtractedFile {
    QString filename;
    QByteArray data;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QHttpMultiPart* createCompressMultipart() const;
    QByteArray createExtractData() const;

protected:
    virtual QString getSaveFileName(const QString& caption, const QString& dir, const QString& filter) {
        return QFileDialog::getSaveFileName(this, caption, dir, filter);
    }
    virtual void showWarning(const QString& title, const QString& text) {
        QMessageBox::warning(this, title, text);
    }
    virtual void showCritical(const QString& title, const QString& text) {
        QMessageBox::critical(this, title, text);
    }
    virtual void showInformation(const QString& title, const QString& text) {
        QMessageBox::information(this, title, text);
    }
    
    virtual void onDataReceived(const QByteArray &data);
    virtual void onRequestFinished();
    virtual void onErrorOccurred(const QString &error);
    
    QByteArray responseData;
    bool requestSuccessful;
    bool loadingFormats;

    signals:
        void sendNetworkRequest(const QNetworkRequest &request, const QByteArray &data);
        void sendMultipartRequest(const QNetworkRequest &request, QHttpMultiPart *multipart);
    void cancelNetworkRequest();

    private slots:
        void removeSelectedFile();
    void browseFiles();
    void browseDirectories();
    void browseArchiveFile();
    void sendCompressRequest();
    void sendExtractRequest();
    void cancelRequest();
    void onOperationTabChanged(int index);
    void loadSupportedFormats();
    void onFormatsReceived(const QByteArray &data);
    void handleMultipartExtractResponse(const QByteArray &data, const QString &boundary);

protected:
    QList<ExtractedFile> parseMultipartData(const QByteArray &data, const QString &boundary);

private:
    QTabWidget *operationTabs;
    
    QWidget *compressTab;
    QLineEdit *archiveNameEdit;
    QComboBox *formatComboBox;
    QListWidget *filesList;
    QPushButton *addFileButton;
    QPushButton *addDirectoryButton;
    QPushButton *removeFileButton;
    QPushButton *compressButton;
    
    QWidget *extractTab;
    QLineEdit *archiveFileEdit;
    QPushButton *browseArchiveButton;
    QLineEdit *extractPathEdit;
    QPushButton *browseExtractPathButton;
    QPushButton *extractButton;
    
    QPushButton *cancelButton;
    QProgressBar *progressBar;
    QThread *networkThread;
    NetworkWorker *worker;

    void setupUi();
    void setupCompressTab();
    void setupExtractTab();
    void addFilesToList(const QStringList& files);
    void addDirectoryToList(const QString& directory);
};
#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include "../src/main_window.h"

class MockNetworkReply : public QNetworkReply {
public:
    MockNetworkReply(QObject *parent = nullptr) : QNetworkReply(parent) {}
    
    void setHttpStatusCode(int code) {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, code);
    }
    
    void setRawData(const QByteArray &data) {
        rawData = data;
        open(ReadOnly);
    }
    
    qint64 readData(char *data, qint64 maxSize) override {
        qint64 len = qMin(maxSize, qint64(rawData.size()));
        memcpy(data, rawData.constData(), len);
        rawData.remove(0, len);
        return len;
    }
    
    qint64 bytesAvailable() const override {
        return rawData.size() + QIODevice::bytesAvailable();
    }
    
    void emitFinished() {
        emit finished();
    }
    
    void abort() override {
        setError(OperationCanceledError, "Operation canceled");
        emit finished();
    }

private:
    QByteArray rawData;
};

class TestMainWindow : public MainWindow {
public:
    TestMainWindow(QWidget *parent = nullptr) : MainWindow(parent), saveFileName("") {}
    
    QString getSaveFileName(const QString&, const QString& dir, const QString&) override {
        return saveFileName.isEmpty() ? dir : saveFileName;
    }
    
    void showWarning(const QString& title, const QString& text) override {
        lastMessage = {title, text, "warning"};
    }
    
    void showCritical(const QString& title, const QString& text) override {
        lastMessage = {title, text, "critical"};
    }
    
    void showInformation(const QString& title, const QString& text) override {
        lastMessage = {title, text, "information"};
    }
    
    void testOnDataReceived(const QByteArray &data) {
        loadingFormats = false;
        onDataReceived(data);
    }
    
    void testOnRequestFinished() {
        loadingFormats = false;
        onRequestFinished();
    }
    
    void testOnErrorOccurred(const QString &error) {
        loadingFormats = false;
        onErrorOccurred(error);
    }
    
    void setLoadingFormats(bool loading) {
        loadingFormats = loading;
    }
    
    void setRequestSuccessful(bool successful) {
        requestSuccessful = successful;
    }
    
    QList<ExtractedFile> testParseMultipartData(const QByteArray &data, const QString &boundary) {
        return parseMultipartData(data, boundary);
    }
    
    struct Message {
        QString title;
        QString text;
        QString type;
    };
    
    Message lastMessage;
    QString saveFileName;
};

class TestMainWindowTests : public QObject {
    Q_OBJECT

private slots:
    void cleanup() {
        QCoreApplication::processEvents();
    }

    void testUiInitialization() {
        TestMainWindow w;
        QTest::qWait(100);

        QTabWidget *operationTabs = w.findChild<QTabWidget*>();
        QVERIFY2(operationTabs, "Operation tabs not found");
        QCOMPARE(operationTabs->count(), 2);

        QLineEdit *archiveNameEdit = w.findChild<QLineEdit*>("archiveNameEdit");
        QVERIFY2(archiveNameEdit, "Archive name edit not found");
        QCOMPARE(archiveNameEdit->text(), QString("archive.zip"));

        QComboBox *formatComboBox = w.findChild<QComboBox*>("formatComboBox");
        QVERIFY2(formatComboBox, "Format combo box not found");
        QVERIFY(formatComboBox->count() > 0);

        QListWidget *filesList = w.findChild<QListWidget*>("filesList");
        QVERIFY2(filesList, "Files list not found");
        QCOMPARE(filesList->count(), 0);

        QPushButton *addFileButton = w.findChild<QPushButton*>("addFileButton");
        QPushButton *addDirectoryButton = w.findChild<QPushButton*>("addDirectoryButton");
        QPushButton *removeFileButton = w.findChild<QPushButton*>("removeFileButton");
        QPushButton *compressButton = w.findChild<QPushButton*>("compressButton");

        QVERIFY2(addFileButton, "Add file button not found");
        QVERIFY2(addDirectoryButton, "Add directory button not found");
        QVERIFY2(removeFileButton, "Remove file button not found");
        QVERIFY2(compressButton, "Compress button not found");

        QLineEdit *archiveFileEdit = w.findChild<QLineEdit*>("archiveFileEdit");
        QLineEdit *extractPathEdit = w.findChild<QLineEdit*>("extractPathEdit");
        QPushButton *browseArchiveButton = w.findChild<QPushButton*>("browseArchiveButton");
        QPushButton *browseExtractPathButton = w.findChild<QPushButton*>("browseExtractPathButton");
        QPushButton *extractButton = w.findChild<QPushButton*>("extractButton");

        QVERIFY2(archiveFileEdit, "Archive file edit not found");
        QVERIFY2(extractPathEdit, "Extract path edit not found");
        QVERIFY2(browseArchiveButton, "Browse archive button not found");
        QVERIFY2(browseExtractPathButton, "Browse extract path button not found");
        QVERIFY2(extractButton, "Extract button not found");
    }

    void testCreateCompressMultipart() {
        TestMainWindow w;
        
        QLineEdit *archiveNameEdit = w.findChild<QLineEdit*>("archiveNameEdit");
        QComboBox *formatComboBox = w.findChild<QComboBox*>("formatComboBox");
        QListWidget *filesList = w.findChild<QListWidget*>("filesList");
        
        archiveNameEdit->setText("test.zip");
        formatComboBox->setCurrentText("zip");
        
        QTemporaryFile tempFile1;
        tempFile1.open();
        tempFile1.write("Hello, World!");
        QString file1Path = tempFile1.fileName();
        tempFile1.close();
        
        QTemporaryFile tempFile2;
        tempFile2.open();
        tempFile2.write("Test content");
        QString file2Path = tempFile2.fileName();
        tempFile2.close();
        
        QListWidgetItem *item1 = new QListWidgetItem("file1.txt");
        item1->setData(Qt::UserRole, file1Path);
        filesList->addItem(item1);
        
        QListWidgetItem *item2 = new QListWidgetItem("dir/file2.txt");
        item2->setData(Qt::UserRole, file2Path);
        filesList->addItem(item2);
        
        QHttpMultiPart *multipart = w.createCompressMultipart();
        QVERIFY(multipart != nullptr);
        
        delete multipart;
    }

    void testCreateExtractData() {
        TestMainWindow w;
        
        QLineEdit *archiveFileEdit = w.findChild<QLineEdit*>("archiveFileEdit");
        
        QTemporaryFile tempArchive;
        tempArchive.open();
        tempArchive.write("PK\003\004fake zip archive data");
        QString archivePath = tempArchive.fileName();
        tempArchive.close();
        
        archiveFileEdit->setText(archivePath);
        
        QByteArray data = w.createExtractData();
        QVERIFY(!data.isEmpty());
        QVERIFY(data.contains("PK"));
        QVERIFY(data.contains("fake zip archive data"));
    }

    void testCompressValidation() {
        TestMainWindow w;
        
        QPushButton *compressButton = w.findChild<QPushButton*>("compressButton");
        QLineEdit *archiveNameEdit = w.findChild<QLineEdit*>("archiveNameEdit");
        QListWidget *filesList = w.findChild<QListWidget*>("filesList");
        
        archiveNameEdit->setText("");
        QTest::mouseClick(compressButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(w.lastMessage.type, QString("warning"));
        QVERIFY(w.lastMessage.text.contains("пустым"));
        
        archiveNameEdit->setText("test.zip");
        w.lastMessage = {};
        QTest::mouseClick(compressButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(w.lastMessage.type, QString("warning"));
        QVERIFY(w.lastMessage.text.contains("файл"));
    }

    void testExtractValidation() {
        TestMainWindow w;
        
        QTabWidget *operationTabs = w.findChild<QTabWidget*>();
        operationTabs->setCurrentIndex(1);
        
        QPushButton *extractButton = w.findChild<QPushButton*>("extractButton");
        QLineEdit *archiveFileEdit = w.findChild<QLineEdit*>("archiveFileEdit");
        
        archiveFileEdit->setText("");
        QTest::mouseClick(extractButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(w.lastMessage.type, QString("warning"));
        
        archiveFileEdit->setText("/nonexistent/file.zip");
        w.lastMessage = {};
        QTest::mouseClick(extractButton, Qt::LeftButton);
        QTest::qWait(100);
        QCOMPARE(w.lastMessage.type, QString("warning"));
        QVERIFY(w.lastMessage.text.contains("существует"));
    }

    void testSupportedFormatsLoading() {
        TestMainWindow w;
        QComboBox *formatComboBox = w.findChild<QComboBox*>("formatComboBox");
        
        QVERIFY(formatComboBox->count() > 0);
        
        QStringList items;
        for (int i = 0; i < formatComboBox->count(); ++i) {
            items << formatComboBox->itemText(i);
        }
        
        QVERIFY(items.contains("zip"));
        QVERIFY(items.contains("tar.gz") || items.contains("tar.bz2") || items.contains("7z"));
        
        w.setLoadingFormats(true);
        QByteArray formatsResponse = R"({"supported_formats": ["zip", "tar.gz", "tar.bz2", "7z"]})";
        w.testOnDataReceived(formatsResponse);
        w.testOnRequestFinished();
        
        QVERIFY(formatComboBox->count() >= 4);
    }

    void testHandleCompressSuccess() {
        TestMainWindow w;
        w.saveFileName = "/tmp/test_archive.zip";
        
        QTabWidget *operationTabs = w.findChild<QTabWidget*>();
        operationTabs->setCurrentIndex(0);
        
        QByteArray fakeArchiveData = "fake archive binary data";
        w.testOnDataReceived(fakeArchiveData);
        w.testOnRequestFinished();
        
        QCOMPARE(w.lastMessage.type, QString("information"));
        QVERIFY(w.lastMessage.text.contains("успешно"));
        
        QVERIFY(QFile::exists("/tmp/test_archive.zip"));
        
        QFile::remove("/tmp/test_archive.zip");
    }

    void testHandleExtractSuccess() {
        TestMainWindow w;
        
        QTabWidget *operationTabs = w.findChild<QTabWidget*>();
        operationTabs->setCurrentIndex(1);
        
        QLineEdit *extractPathEdit = w.findChild<QLineEdit*>("extractPathEdit");
        QTemporaryDir tempDir;
        extractPathEdit->setText(tempDir.path());
        
        QString boundary = "----CustomBoundary";
        QString multipartData = QString(
            "--%1\r\n"
            "content-disposition: form-data; name=\"file\"; filename=\"file1.txt\"\r\n"
            "content-type: application/octet-stream\r\n"
            "\r\n"
            "Hello World!\r\n"
            "--%1\r\n"
            "content-disposition: form-data; name=\"file\"; filename=\"subdir/file2.txt\"\r\n"
            "content-type: application/octet-stream\r\n"
            "\r\n"
            "Test content in subdirectory\r\n"
            "--%1--\r\n"
        ).arg(boundary);
        
        QByteArray multipartBytes = multipartData.toUtf8();
        
        w.testOnDataReceived(multipartBytes);
        w.testOnRequestFinished();
        
        QCOMPARE(w.lastMessage.type, QString("information"));
        QVERIFY(w.lastMessage.text.contains("Извлечено"));
        QVERIFY(w.lastMessage.text.contains("2"));
        
        QVERIFY(QFile::exists(tempDir.path() + "/file1.txt"));
        QVERIFY(QFile::exists(tempDir.path() + "/subdir/file2.txt"));
        
        QFile file1(tempDir.path() + "/file1.txt");
        file1.open(QIODevice::ReadOnly);
        QCOMPARE(file1.readAll(), QByteArray("Hello World!"));
        file1.close();
        
        QFile file2(tempDir.path() + "/subdir/file2.txt");
        file2.open(QIODevice::ReadOnly);
        QCOMPARE(file2.readAll(), QByteArray("Test content in subdirectory"));
        file2.close();
    }

    void testHandleNetworkError() {
        TestMainWindow w;
        
        QString errorMessage = "Connection failed";
        w.testOnErrorOccurred(errorMessage);
        
        QCOMPARE(w.lastMessage.type, QString("critical"));
        QVERIFY(w.lastMessage.text.contains(errorMessage));
    }

    void testCancelRequest() {
        TestMainWindow w;
        
        QPushButton *compressButton = w.findChild<QPushButton*>("compressButton");
        QPushButton *extractButton = w.findChild<QPushButton*>("extractButton");
        QPushButton *cancelButton = w.findChild<QPushButton*>("cancelButton");
        QProgressBar *progressBar = w.findChild<QProgressBar*>();
        
        compressButton->setText("Создание архива...");
        compressButton->setEnabled(false);
        progressBar->setVisible(true);
        cancelButton->setVisible(true);
        
        QTest::mouseClick(cancelButton, Qt::LeftButton);
        QTest::qWait(100);
        
        QCOMPARE(compressButton->text(), QString("Создать архив"));
        QVERIFY(compressButton->isEnabled());
        QCOMPARE(extractButton->text(), QString("Извлечь архив"));
        QVERIFY(extractButton->isEnabled());
        QVERIFY(!progressBar->isVisible());
        QVERIFY(!cancelButton->isVisible());
        
        QCOMPARE(w.lastMessage.type, QString("information"));
        QVERIFY(w.lastMessage.text.contains("отменен"));
    }

    void testTabSwitching() {
        TestMainWindow w;
        
        QTabWidget *operationTabs = w.findChild<QTabWidget*>();
        QPushButton *cancelButton = w.findChild<QPushButton*>("cancelButton");
        QProgressBar *progressBar = w.findChild<QProgressBar*>();
        
        progressBar->setVisible(true);
        cancelButton->setVisible(true);
        
        operationTabs->setCurrentIndex(1);
        QTest::qWait(100);
        
        QVERIFY(!progressBar->isVisible());
        QVERIFY(!cancelButton->isVisible());
    }

    void testParseMultipartData() {
        TestMainWindow w;
        
        QString boundary = "----TestBoundary";
        QString multipartData = QString(
            "--%1\r\n"
            "content-disposition: form-data; name=\"file\"; filename=\"test1.txt\"\r\n"
            "content-type: text/plain\r\n"
            "\r\n"
            "File 1 content\r\n"
            "--%1\r\n"
            "content-disposition: form-data; name=\"file\"; filename=\"dir/test2.txt\"\r\n"
            "content-type: text/plain\r\n"
            "\r\n"
            "File 2 content\r\n"
            "--%1--\r\n"
        ).arg(boundary);
        
        QByteArray multipartBytes = multipartData.toUtf8();
        QList<ExtractedFile> files = w.testParseMultipartData(multipartBytes, boundary);
        
        QCOMPARE(files.size(), 2);
        
        QCOMPARE(files[0].filename, QString("test1.txt"));
        QCOMPARE(files[0].data, QByteArray("File 1 content"));
        
        QCOMPARE(files[1].filename, QString("dir/test2.txt"));
        QCOMPARE(files[1].data, QByteArray("File 2 content"));
    }

    void testMultipartEmptyBoundary() {
        TestMainWindow w;
        
        QString boundary = "----EmptyBoundary";
        QString multipartData = QString("--%1--\r\n").arg(boundary);
        
        QByteArray multipartBytes = multipartData.toUtf8();
        QList<ExtractedFile> files = w.testParseMultipartData(multipartBytes, boundary);
        
        QCOMPARE(files.size(), 0);
    }

private:
    QApplication *app = nullptr;
};

QTEST_MAIN(TestMainWindowTests)
#include "main_window_test.moc"
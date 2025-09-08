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

    void testCreateCompressJsonBody() {
        TestMainWindow w;
        
        QLineEdit *archiveNameEdit = w.findChild<QLineEdit*>("archiveNameEdit");
        QComboBox *formatComboBox = w.findChild<QComboBox*>("formatComboBox");
        QListWidget *filesList = w.findChild<QListWidget*>("filesList");
        
        archiveNameEdit->setText("test.zip");
        formatComboBox->setCurrentText("zip");
        
        QListWidgetItem *item1 = new QListWidgetItem("file1.txt");
        item1->setData(Qt::UserRole, "/tmp/file1.txt");
        filesList->addItem(item1);
        
        QListWidgetItem *item2 = new QListWidgetItem("dir/file2.txt");
        item2->setData(Qt::UserRole, "/tmp/dir/file2.txt");
        filesList->addItem(item2);
        
        QTemporaryFile tempFile1("/tmp/file1.txt");
        tempFile1.open();
        tempFile1.write("Hello, World!");
        tempFile1.close();
        
        QTemporaryFile tempFile2("/tmp/dir_file2.txt");
        tempFile2.open();
        tempFile2.write("Test content");
        tempFile2.close();
        
        item1->setData(Qt::UserRole, tempFile1.fileName());
        item2->setData(Qt::UserRole, tempFile2.fileName());
        
        QJsonObject json = w.createCompressJsonBody();
        
        QCOMPARE(json["operation"].toString(), QString("compress"));
        QCOMPARE(json["format"].toString(), QString("zip"));
        QCOMPARE(json["archive_name"].toString(), QString("test.zip"));
        
        QJsonArray files = json["files"].toArray();
        QCOMPARE(files.size(), 2);
        
        QJsonObject file1 = files[0].toObject();
        QJsonObject file2 = files[1].toObject();
        
        QCOMPARE(file1["name"].toString(), QString("file1.txt"));
        QCOMPARE(file2["name"].toString(), QString("dir/file2.txt"));
        
        QVERIFY(!file1["content"].toString().isEmpty());
        QVERIFY(!file2["content"].toString().isEmpty());
    }

    void testCreateExtractJsonBody() {
        TestMainWindow w;
        
        QLineEdit *archiveFileEdit = w.findChild<QLineEdit*>("archiveFileEdit");
        QLineEdit *extractPathEdit = w.findChild<QLineEdit*>("extractPathEdit");
        
        QTemporaryFile tempArchive;
        tempArchive.open();
        tempArchive.write("fake archive data");
        tempArchive.close();
        
        archiveFileEdit->setText(tempArchive.fileName() + ".zip");
        extractPathEdit->setText("/tmp/extracted");
        
        QFile archiveFile(tempArchive.fileName() + ".zip");
        archiveFile.open(QIODevice::WriteOnly);
        archiveFile.write("fake archive data");
        archiveFile.close();
        
        QJsonObject json = w.createExtractJsonBody();
        
        QCOMPARE(json["operation"].toString(), QString("extract"));
        QCOMPARE(json["format"].toString(), QString("zip"));
        QCOMPARE(json["extract_path"].toString(), QString("/tmp/extracted"));
        QVERIFY(!json["archive_data"].toString().isEmpty());
        
        archiveFile.remove();
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

    void testFormatDetection() {
        TestMainWindow w;
        QLineEdit *archiveFileEdit = w.findChild<QLineEdit*>("archiveFileEdit");
        
        QTemporaryFile zipFile;
        zipFile.setFileTemplate("/tmp/test_XXXXXX.zip");
        zipFile.open();
        zipFile.write("test");
        zipFile.close();
        
        QTemporaryFile tarGzFile;
        tarGzFile.setFileTemplate("/tmp/test_XXXXXX.tar.gz");
        tarGzFile.open();
        tarGzFile.write("test");
        tarGzFile.close();
        
        QTemporaryFile tarBz2File;
        tarBz2File.setFileTemplate("/tmp/test_XXXXXX.tar.bz2");
        tarBz2File.open();
        tarBz2File.write("test");
        tarBz2File.close();
        
        QTemporaryFile sevenZFile;
        sevenZFile.setFileTemplate("/tmp/test_XXXXXX.7z");
        sevenZFile.open();
        sevenZFile.write("test");
        sevenZFile.close();
        
        archiveFileEdit->setText(zipFile.fileName());
        QJsonObject zipJson = w.createExtractJsonBody();
        QCOMPARE(zipJson["format"].toString(), QString("zip"));
        
        archiveFileEdit->setText(tarGzFile.fileName());
        QJsonObject tarGzJson = w.createExtractJsonBody();
        QCOMPARE(tarGzJson["format"].toString(), QString("tar.gz"));
        
        archiveFileEdit->setText(tarBz2File.fileName());
        QJsonObject tarBz2Json = w.createExtractJsonBody();
        QCOMPARE(tarBz2Json["format"].toString(), QString("tar.bz2"));
        
        archiveFileEdit->setText(sevenZFile.fileName());
        QJsonObject sevenZJson = w.createExtractJsonBody();
        QCOMPARE(sevenZJson["format"].toString(), QString("7z"));
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
        extractPathEdit->setText("/tmp/extracted");
        
        QJsonObject response;
        response["success"] = true;
        response["extracted_files_count"] = 3;
        response["total_extracted_size"] = 1024;
        
        QJsonArray files;
        QJsonObject file1;
        file1["name"] = "file1.txt";
        file1["size"] = 512;
        files.append(file1);
        
        QJsonObject file2;
        file2["name"] = "file2.txt";
        file2["size"] = 512;
        files.append(file2);
        
        response["files"] = files;
        
        QJsonDocument doc(response);
        QByteArray jsonData = doc.toJson();
        
        w.testOnDataReceived(jsonData);
        w.testOnRequestFinished();
        
        QCOMPARE(w.lastMessage.type, QString("information"));
        QVERIFY(w.lastMessage.text.contains("успешно"));
        QVERIFY(w.lastMessage.text.contains("3"));
        QVERIFY(w.lastMessage.text.contains("/tmp/extracted"));
        QVERIFY(!w.lastMessage.text.contains("file1.txt"));
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

private:
    QApplication *app = nullptr;
};

QTEST_MAIN(TestMainWindowTests)
#include "main_window_test.moc"
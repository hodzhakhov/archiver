#include "main_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QApplication>
#include <QDirIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkThread(new QThread(this)), worker(new NetworkWorker()), 
      requestSuccessful(false), loadingFormats(false) {
    worker->moveToThread(networkThread);
    connect(networkThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MainWindow::destroyed, networkThread, &QThread::quit);
    connect(this, &MainWindow::sendNetworkRequest, worker, &NetworkWorker::processRequest);
    connect(this, &MainWindow::cancelNetworkRequest, worker, &NetworkWorker::cancelRequest);
    connect(worker, &NetworkWorker::dataReceived, this, &MainWindow::onDataReceived);
    connect(worker, &NetworkWorker::finished, this, &MainWindow::onRequestFinished);
    connect(worker, &NetworkWorker::errorOccurred, this, &MainWindow::onErrorOccurred);
    networkThread->start();

    setupUi();
    
    connect(operationTabs, &QTabWidget::currentChanged, this, &MainWindow::onOperationTabChanged);
    
    connect(addFileButton, &QPushButton::clicked, this, &MainWindow::browseFiles);
    connect(addDirectoryButton, &QPushButton::clicked, this, &MainWindow::browseDirectories);
    connect(removeFileButton, &QPushButton::clicked, this, &MainWindow::removeSelectedFile);
    connect(compressButton, &QPushButton::clicked, this, &MainWindow::sendCompressRequest);
    
    connect(browseArchiveButton, &QPushButton::clicked, this, &MainWindow::browseArchiveFile);
    connect(browseExtractPathButton, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку для извлечения");
        if (!dir.isEmpty()) {
            extractPathEdit->setText(dir);
        }
    });
    connect(extractButton, &QPushButton::clicked, this, &MainWindow::sendExtractRequest);
    
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::cancelRequest);
    
    loadSupportedFormats();
}

MainWindow::~MainWindow() {
    networkThread->quit();
    networkThread->wait();
}

void MainWindow::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    operationTabs = new QTabWidget(this);
    
    setupCompressTab();
    operationTabs->addTab(compressTab, "Сжатие");
    
    setupExtractTab();
    operationTabs->addTab(extractTab, "Извлечение");
    
    mainLayout->addWidget(operationTabs);

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    mainLayout->addWidget(progressBar);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelButton = new QPushButton("Отмена", this);
    cancelButton->setObjectName("cancelButton");
    cancelButton->setVisible(false);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);

    setWindowTitle("Архивный клиент");
    resize(800, 600);
}

void MainWindow::setupCompressTab() {
    compressTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(compressTab);

    QGroupBox *archiveGroup = new QGroupBox("Параметры архива", compressTab);
    QGridLayout *archiveLayout = new QGridLayout(archiveGroup);

    archiveLayout->addWidget(new QLabel("Имя архива:"), 0, 0);
    archiveNameEdit = new QLineEdit("archive.zip", archiveGroup);
    archiveNameEdit->setObjectName("archiveNameEdit");
    archiveLayout->addWidget(archiveNameEdit, 0, 1);

    archiveLayout->addWidget(new QLabel("Формат:"), 1, 0);
    formatComboBox = new QComboBox(archiveGroup);
    formatComboBox->setObjectName("formatComboBox");
    formatComboBox->addItems({"zip", "tar.gz", "tar.bz2", "7z"});
    archiveLayout->addWidget(formatComboBox, 1, 1);

    layout->addWidget(archiveGroup);

    QGroupBox *filesGroup = new QGroupBox("Файлы для архивирования", compressTab);
    QVBoxLayout *filesLayout = new QVBoxLayout(filesGroup);

    filesList = new QListWidget(filesGroup);
    filesList->setObjectName("filesList");
    filesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    filesLayout->addWidget(filesList);

    QHBoxLayout *filesButtonLayout = new QHBoxLayout();
    addFileButton = new QPushButton("Добавить файлы", filesGroup);
    addFileButton->setObjectName("addFileButton");
    addDirectoryButton = new QPushButton("Добавить папку", filesGroup);
    addDirectoryButton->setObjectName("addDirectoryButton");
    removeFileButton = new QPushButton("Удалить выбранные", filesGroup);
    removeFileButton->setObjectName("removeFileButton");

    filesButtonLayout->addWidget(addFileButton);
    filesButtonLayout->addWidget(addDirectoryButton);
    filesButtonLayout->addWidget(removeFileButton);
    filesButtonLayout->addStretch();

    filesLayout->addLayout(filesButtonLayout);
    layout->addWidget(filesGroup);

    QHBoxLayout *compressLayout = new QHBoxLayout();
    compressLayout->addStretch();
    compressButton = new QPushButton("Создать архив", compressTab);
    compressButton->setObjectName("compressButton");
    compressButton->setMinimumWidth(150);
    compressLayout->addWidget(compressButton);
    layout->addLayout(compressLayout);
}

void MainWindow::setupExtractTab() {
    extractTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(extractTab);

    QGroupBox *archiveGroup = new QGroupBox("Архивный файл", extractTab);
    QGridLayout *archiveLayout = new QGridLayout(archiveGroup);

    archiveLayout->addWidget(new QLabel("Файл архива:"), 0, 0);
    archiveFileEdit = new QLineEdit(archiveGroup);
    archiveFileEdit->setObjectName("archiveFileEdit");
    archiveLayout->addWidget(archiveFileEdit, 0, 1);

    browseArchiveButton = new QPushButton("Обзор...", archiveGroup);
    browseArchiveButton->setObjectName("browseArchiveButton");
    archiveLayout->addWidget(browseArchiveButton, 0, 2);

    layout->addWidget(archiveGroup);

    QGroupBox *extractGroup = new QGroupBox("Путь для извлечения", extractTab);
    QGridLayout *extractLayout = new QGridLayout(extractGroup);

    extractLayout->addWidget(new QLabel("Папка:"), 0, 0);
    extractPathEdit = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation), extractGroup);
    extractPathEdit->setObjectName("extractPathEdit");
    extractLayout->addWidget(extractPathEdit, 0, 1);

    browseExtractPathButton = new QPushButton("Обзор...", extractGroup);
    browseExtractPathButton->setObjectName("browseExtractPathButton");
    extractLayout->addWidget(browseExtractPathButton, 0, 2);

    layout->addWidget(extractGroup);

    layout->addStretch();

    QHBoxLayout *extractButtonLayout = new QHBoxLayout();
    extractButtonLayout->addStretch();
    extractButton = new QPushButton("Извлечь архив", extractTab);
    extractButton->setObjectName("extractButton");
    extractButton->setMinimumWidth(150);
    extractButtonLayout->addWidget(extractButton);
    layout->addLayout(extractButtonLayout);
}

QJsonObject MainWindow::createCompressJsonBody() const {
    QJsonObject json;
    json["operation"] = "compress";
    json["format"] = formatComboBox->currentText();
    json["archive_name"] = archiveNameEdit->text();

    QJsonArray files;
    for (int i = 0; i < filesList->count(); ++i) {
        QListWidgetItem *item = filesList->item(i);
        QString filePath = item->data(Qt::UserRole).toString();
        
        QJsonObject file;
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.isFile()) {
            file["name"] = item->text();
            file["content"] = encodeFileToBase64(filePath);
            files.append(file);
        }
    }
    json["files"] = files;

    return json;
}

QJsonObject MainWindow::createExtractJsonBody() const {
    QJsonObject json;
    json["operation"] = "extract";
    
    QString archiveFile = archiveFileEdit->text();
    QFileInfo fileInfo(archiveFile);
    QString extension = fileInfo.suffix().toLower();
    
    QString format = "zip";
    if (extension == "zip") format = "zip";
    else if (archiveFile.endsWith(".tar.gz")) format = "tar.gz";
    else if (archiveFile.endsWith(".tar.bz2")) format = "tar.bz2";
    else if (extension == "7z") format = "7z";
    
    json["format"] = format;
    json["archive_data"] = encodeFileToBase64(archiveFile);
    
    if (!extractPathEdit->text().isEmpty()) {
        json["extract_path"] = extractPathEdit->text();
    }

    return json;
}

void MainWindow::browseFiles() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы для архивирования");
    if (!files.isEmpty()) {
        addFilesToList(files);
    }
}

void MainWindow::browseDirectories() {
    QString directory = QFileDialog::getExistingDirectory(this, "Выберите папку для архивирования");
    if (!directory.isEmpty()) {
        addDirectoryToList(directory);
    }
}

void MainWindow::browseArchiveFile() {
    QString file = QFileDialog::getOpenFileName(this, "Выберите архивный файл", 
        "", "Archives (*.zip *.tar.gz *.tar.bz2 *.7z)");
    if (!file.isEmpty()) {
        archiveFileEdit->setText(file);
    }
}

void MainWindow::addFilesToList(const QStringList& files) {
    for (const QString& file : files) {
        QFileInfo fileInfo(file);
        if (fileInfo.exists() && fileInfo.isFile()) {
            QListWidgetItem *item = new QListWidgetItem(fileInfo.fileName());
            item->setData(Qt::UserRole, file);
            item->setToolTip(file);
            filesList->addItem(item);
        }
    }
}

void MainWindow::addDirectoryToList(const QString& directory) {
    QFileInfo dirInfo(directory);
    QString dirName = dirInfo.baseName();
    
    QDirIterator iterator(directory, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        QString file = iterator.next();
        QFileInfo fileInfo(file);
        
        QString relativePath = QDir(directory).relativeFilePath(file);
        QString archivePath = dirName + "/" + relativePath;
        
        QListWidgetItem *item = new QListWidgetItem(archivePath);
        item->setData(Qt::UserRole, file);
        item->setToolTip(file);
        filesList->addItem(item);
    }
}

void MainWindow::removeSelectedFile() {
    QList<QListWidgetItem*> selectedItems = filesList->selectedItems();
    for (QListWidgetItem* item : selectedItems) {
        delete item;
    }
}

void MainWindow::sendCompressRequest() {
    if (archiveNameEdit->text().isEmpty()) {
        showWarning("Ошибка ввода", "Имя архива не может быть пустым.");
        return;
    }
    if (filesList->count() == 0) {
        showWarning("Ошибка ввода", "Добавьте хотя бы один файл для архивирования.");
        return;
    }

    QJsonObject json = createCompressJsonBody();
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson();

    QNetworkRequest request(QUrl("http://localhost:8080/archive"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    responseData.clear();
    requestSuccessful = false;
    emit sendNetworkRequest(request, jsonData);

    compressButton->setText("Создание архива...");
    compressButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);
    cancelButton->setVisible(true);
}

void MainWindow::sendExtractRequest() {
    if (archiveFileEdit->text().isEmpty()) {
        showWarning("Ошибка ввода", "Выберите архивный файл.");
            return;
        }
    if (!QFile::exists(archiveFileEdit->text())) {
        showWarning("Ошибка ввода", "Архивный файл не существует.");
        return;
    }

    QJsonObject json = createExtractJsonBody();
    QJsonDocument doc(json);
    QByteArray jsonData = doc.toJson();

    QNetworkRequest request(QUrl("http://localhost:8080/archive"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    responseData.clear();
    requestSuccessful = false;
    emit sendNetworkRequest(request, jsonData);

    extractButton->setText("Извлечение...");
    extractButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);
    cancelButton->setVisible(true);
}

void MainWindow::cancelRequest() {
    emit cancelNetworkRequest();

    compressButton->setText("Создать архив");
    compressButton->setEnabled(true);
    extractButton->setText("Извлечь архив");
    extractButton->setEnabled(true);
    
    progressBar->setVisible(false);
    cancelButton->setVisible(false);
    requestSuccessful = false;
    responseData.clear();
    showInformation("Отменено", "Запрос был отменен.");
}

void MainWindow::onDataReceived(const QByteArray &data) {
    if (loadingFormats) {
        onFormatsReceived(data);
        return;
    }
    responseData.append(data);
    requestSuccessful = true;
}

void MainWindow::onRequestFinished() {
    if (loadingFormats) {
        loadingFormats = false;
        return;
    }
    
    compressButton->setText("Создать архив");
    compressButton->setEnabled(true);
    extractButton->setText("Извлечь архив");
    extractButton->setEnabled(true);
    
    progressBar->setVisible(false);
    cancelButton->setVisible(false);

    if (!requestSuccessful || responseData.isEmpty()) {
        responseData.clear();
        return;
    }

    int currentTab = operationTabs->currentIndex();
    if (currentTab == 0) {
        QString fileName = getSaveFileName("Сохранить архив", archiveNameEdit->text(), 
            "Archive Files (*." + formatComboBox->currentText() + ")");
        if (!fileName.isEmpty()) {
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(responseData);
        file.close();
                showInformation("Успех", "Архив успешно создан и сохранен!");
            } else {
                showCritical("Ошибка файла", "Не удалось сохранить архив: " + file.errorString());
            }
        }
    } else {
        try {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();
            
            if (obj["success"].toBool()) {
                int filesCount = obj["extracted_files_count"].toInt();
                QString extractPath = extractPathEdit->text();
                if (extractPath.isEmpty()) {
                    extractPath = "папку по умолчанию";
                }
                
                QString message = QString("Архив успешно извлечен!\n\nФайлов: %1\nПуть: %2")
                    .arg(filesCount)
                    .arg(extractPath);
                
                showInformation("Успех", message);
    } else {
                showCritical("Ошибка", "Не удалось извлечь архив.");
            }
        } catch (...) {
            showCritical("Ошибка", "Неверный ответ сервера.");
        }
    }

    responseData.clear();
}

void MainWindow::onErrorOccurred(const QString &error) {
    if (loadingFormats) {
        loadingFormats = false;
        return;
    }
    
    compressButton->setText("Создать архив");
    compressButton->setEnabled(true);
    extractButton->setText("Извлечь архив");
    extractButton->setEnabled(true);
    
    progressBar->setVisible(false);
    cancelButton->setVisible(false);
    requestSuccessful = false;
    showCritical("Сетевая ошибка", error);
    responseData.clear();
}

void MainWindow::onOperationTabChanged(int index) {
    Q_UNUSED(index)
    if (progressBar->isVisible()) {
        cancelRequest();
    }
}

void MainWindow::loadSupportedFormats() {
    loadingFormats = true;
    QNetworkRequest request(QUrl("http://localhost:8080/formats"));
    emit sendNetworkRequest(request, QByteArray());
}

void MainWindow::onFormatsReceived(const QByteArray &data) {
    try {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject obj = doc.object();
        QJsonArray formats = obj["supported_formats"].toArray();
        
        formatComboBox->clear();
        for (const QJsonValue& format : formats) {
            formatComboBox->addItem(format.toString());
        }
        
        if (formatComboBox->count() > 0) {
            formatComboBox->setCurrentIndex(0);
        }
    } catch (...) {
    }
}

QString MainWindow::encodeFileToBase64(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    
    QByteArray fileData = file.readAll();
    return fileData.toBase64();
}
// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "trophy_viewer.h"

TrophyViewer::TrophyViewer(QString trophyPath, QString gameTrpPath) : QMainWindow() {
    this->setWindowTitle("Trophy Viewer");
    this->setAttribute(Qt::WA_DeleteOnClose);
    tabWidget = new QTabWidget(this);
    gameTrpPath_ = gameTrpPath;
    headers << "Trophy"
            << "Name"
            << "Description"
            << "ID"
            << "Hidden"
            << "Type"
            << "PID";
    PopulateTrophyWidget(trophyPath);
}

void TrophyViewer::PopulateTrophyWidget(QString title) {
    QString trophyDir = QDir::currentPath() + "/user/game_data/" + title + "/TrophyFiles";
    QDir dir(trophyDir);
    if (!dir.exists()) {
        std::filesystem::path path(gameTrpPath_.toStdString());
#ifdef _WIN64
        path = std::filesystem::path(gameTrpPath_.toStdWString());
#endif
        if (!trp.Extract(path))
            return;
    }
    QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (dirList.isEmpty())
        return;

    for (const QFileInfo& dirInfo : dirList) {
        QString tabName = dirInfo.fileName();
        QString trpDir = trophyDir + "/" + tabName;

        QString iconsPath = trpDir + "/Icons";
        QDir iconsDir(iconsPath);
        QFileInfoList iconDirList = iconsDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        std::vector<QImage> icons;

        for (const QFileInfo& iconInfo : iconDirList) {
            QImage icon =
                QImage(iconInfo.absoluteFilePath())
                    .scaled(QSize(128, 128), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            icons.push_back(icon);
        }

        QStringList trpId;
        QStringList trpHidden;
        QStringList trpType;
        QStringList trpPid;
        QStringList trophyNames;
        QStringList trophyDetails;

        QString xmlPath = trpDir + "/Xml/TROP.XML";
        QFile file(xmlPath);
        if (!file.open(QFile::ReadOnly | QFile::Text)) {
            return;
        }

        QXmlStreamReader reader(&file);

        while (!reader.atEnd() && !reader.hasError()) {
            reader.readNext();
            if (reader.isStartElement() && reader.name().toString() == "trophy") {
                trpId.append(reader.attributes().value("id").toString());
                trpHidden.append(reader.attributes().value("hidden").toString());
                trpType.append(reader.attributes().value("ttype").toString());
                trpPid.append(reader.attributes().value("pid").toString());
            }

            if (reader.name().toString() == "name" && !trpId.isEmpty()) {
                trophyNames.append(reader.readElementText());
            }

            if (reader.name().toString() == "detail" && !trpId.isEmpty()) {
                trophyDetails.append(reader.readElementText());
            }
        }
        QTableWidget* tableWidget = new QTableWidget(this);
        tableWidget->setShowGrid(false);
        tableWidget->setColumnCount(7);
        tableWidget->setHorizontalHeaderLabels(headers);
        tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        tableWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tableWidget->horizontalHeader()->setStretchLastSection(true);
        tableWidget->verticalHeader()->setVisible(false);
        tableWidget->setRowCount(icons.size());
        for (int row = 0; auto& icon : icons) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setData(Qt::DecorationRole, icon);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            tableWidget->setItem(row, 0, item);
            if (!trophyNames.isEmpty() && !trophyDetails.isEmpty()) {
                SetTableItem(tableWidget, row, 1, trophyNames[row]);
                SetTableItem(tableWidget, row, 2, trophyDetails[row]);
                SetTableItem(tableWidget, row, 3, trpId[row]);
                SetTableItem(tableWidget, row, 4, trpHidden[row]);
                SetTableItem(tableWidget, row, 5, GetTrpType(trpType[row].at(0)));
                SetTableItem(tableWidget, row, 6, trpPid[row]);
            }
            tableWidget->verticalHeader()->resizeSection(row, icon.height());
            row++;
        }
        tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        int width = 16;
        for (int i = 0; i < 7; i++) {
            width += tableWidget->horizontalHeader()->sectionSize(i);
        }
        tableWidget->resize(width, 720);
        tabWidget->addTab(tableWidget,
                          tabName.insert(6, " ").replace(0, 1, tabName.at(0).toUpper()));
        this->resize(width + 20, 720);
    }
    this->setCentralWidget(tabWidget);
}

void TrophyViewer::SetTableItem(QTableWidget* parent, int row, int column, QString str) {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    QLabel* label = new QLabel(str);
    QTableWidgetItem* item = new QTableWidgetItem();
    label->setWordWrap(true);
    label->setStyleSheet("color: white; font-size: 15px; font-weight: bold;");

    // Create shadow effect
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(5);               // Set the blur radius of the shadow
    shadowEffect->setColor(QColor(0, 0, 0, 160)); // Set the color and opacity of the shadow
    shadowEffect->setOffset(2, 2);                // Set the offset of the shadow

    label->setGraphicsEffect(shadowEffect); // Apply shadow effect to the QLabel

    layout->addWidget(label);
    if (column != 1 && column != 2)
        layout->setAlignment(Qt::AlignCenter);
    widget->setLayout(layout);
    parent->setItem(row, column, item);
    parent->setCellWidget(row, column, widget);
}

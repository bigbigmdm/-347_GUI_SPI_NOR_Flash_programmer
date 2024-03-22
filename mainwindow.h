/*
 * Copyright (C) 2023 Mikhail Medvedev <e-ink-reader@yandex.ru>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <unistd.h>
#include "qhexedit.h"
#include "dialogsp.h"
#include "searchdialog.h"
#include "spi-op.h"
#include "spi_flash.h"
extern "C" {
   #include "ch347.h"
}


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:

    void receiveAddr(QString);
    void receiveAddr2(QString);


private slots:
    void on_pushButton_clicked();
    void on_pushButton_2_clicked();
    void on_comboBox_size_currentIndexChanged(int index);
    void on_comboBox_page_currentIndexChanged(int index);
    void on_comboBox_speed_currentIndexChanged(int index);
    void on_actionDetect_triggered();
    void on_actionSave_triggered();
    void on_actionErase_triggered();
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionOpen_triggered();
    void on_actionWrite_triggered();
    void on_actionRead_triggered();
    void on_actionExit_triggered();
    void on_comboBox_man_currentIndexChanged(int index);
    void on_comboBox_name_currentIndexChanged(const QString &arg1);
    void on_actionVerify_triggered();
    void on_pushButton_3_clicked();
    void on_actionSave_Part_triggered();
    void on_actionLoad_Part_triggered();
    void on_actionFind_Replace_triggered();

private:
    Ui::MainWindow *ui;
    int statusCH347;
    QByteArray chipData;
    uint32_t currentChipSize, currentNumBlocks, currentBlockSize;
    uint8_t currentSpeed;
    QString bytePrint(unsigned char z);
    QVector <QString> chType = {"SPI_FLASH","25_EEPROM","93_EEPROM","24_EEPROM","95_EEPROM"};
    struct chip_data {
        QString chipManuf;
        QString chipTypeTxt;
        QString chipName;
        uint8_t chipJedecIDMan;
        uint8_t chipJedecIDDev;
        uint8_t chipJedecIDCap;
        uint32_t chipSize;
        uint16_t blockSize;
        uint8_t chipTypeHex;
        uint8_t algorithmCode;
        int delay;
        uint8_t extend;
        uint8_t eeprom;
        uint8_t eepromPages;
        QString chipVCC;
    };
    chip_data chips[2000];
    int max_rec;
    QString fileName;
    QHexEdit *hexEdit;
    QString sizeConvert(int a);
    QString hexiAddr(uint32_t a);
    uint32_t hexToInt(QString str);
    void ch341StatusFlashing();
    QByteArray block;
    uint32_t blockStartAddr, blockLen;
};

#endif // MAINWINDOW_H

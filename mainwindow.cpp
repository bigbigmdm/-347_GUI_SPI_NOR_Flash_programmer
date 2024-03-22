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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QErrorMessage>
#include <QDragEnterEvent>
#include <QtGui>
#include "qhexedit.h"
#include "dialogsp.h"
#include "dialogrp.h"
#include <alloca.h>

#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    int index2;
    max_rec = 0;
    ui->setupUi(this);
    ui->statusBar->addPermanentWidget(ui->lStatus,0);
    ui->statusBar->addPermanentWidget(ui->eStatus,0);
    ui->statusBar->addPermanentWidget(ui->jLabel,0);
    ui->statusBar->addPermanentWidget(ui->jedecEdit,0);
    ui->progressBar->setValue(0);
    ui->comboBox_name->addItems({""});
    ui->comboBox_man->addItems({""});
    ui->comboBox_vcc->addItems({ " ", "3.3 V", "1.8 V", "5.0 V"});
    //0 = 20kHz; 1 = 100kHz, 2 = 400kHz, 3 = 750kHz
    //ui->comboBox_speed->addItem(" ", -1); //uncorrected value
    ui->comboBox_speed->addItem("750 khz", 3);
    ui->comboBox_speed->addItem("400 khz", 2);
    ui->comboBox_speed->addItem("100 khz", 1);
    ui->comboBox_speed->addItem("20 khz", 0);

    ui->comboBox_page->addItem(" ", 0);
    ui->comboBox_page->addItem("1", 1);
    ui->comboBox_page->addItem("2", 2);
    ui->comboBox_page->addItem("4", 4);
    ui->comboBox_page->addItem("8", 8);
    ui->comboBox_page->addItem("16", 16);
    ui->comboBox_page->addItem("32", 32);
    ui->comboBox_page->addItem("64", 64);
    ui->comboBox_page->addItem("128", 128);
    ui->comboBox_page->addItem("256", 256);
    ui->comboBox_page->addItem("512", 512);

    ui->comboBox_size->addItem(" ", 0);
    ui->comboBox_size->addItem("64 K", 64 * 1024);
    ui->comboBox_size->addItem("128 K", 128 * 1024);
    ui->comboBox_size->addItem("256 K", 256 * 1024);
    ui->comboBox_size->addItem("512 K", 512 * 1024);
    ui->comboBox_size->addItem("1 M", 1024 * 1024);
    ui->comboBox_size->addItem("2 M", 2048 * 1024);
    ui->comboBox_size->addItem("4 M", 4096 * 1024);
    ui->comboBox_size->addItem("8 M", 8192 * 1024);
    ui->comboBox_size->addItem("16 M", 16384 * 1024);
    ui->comboBox_size->addItem("32 M", 32768 * 1024);
    ui->comboBox_size->addItem("64 M", 65536 * 1024);
    currentChipSize = 0;
    currentNumBlocks = 0;
    currentBlockSize = 0;
    currentSpeed = 0;
    blockStartAddr = 0;
    blockLen = 0;
    // connect and status check
    //ch347_open();
    statusCH347 = SPIDeviceInit();
    qDebug() << "statusCH347=" << statusCH347;
    //statusCH341 = ch341Configure(0x1a86,0x5512); // VID 1a86 PID 5512 for CH341A
    ch341StatusFlashing();
    chipData.resize(256);
    for (int i=0; i < 256; i++)
    {
        chipData[i] = char(0xff);
    }

    //ch341SetStream(3);
    //if (statusCH347 == 1)
    SPIDeviceRelease();//ch341Release();
    hexEdit = new QHexEdit(ui->frame);
    hexEdit->setGeometry(0,0,ui->frame->width(),ui->frame->height());
     hexEdit->setData(chipData);
    //ui->pushButton->resize(165, ui->pushButton->height()); //change height // widget->setFixedWidth(165)
    //opening chip database file
    ui->statusBar->showMessage("Opening DAT file");
    QFile datfile("CH341a.Dat");
    QByteArray dataChips;
    if (!datfile.open(QIODevice::ReadOnly))
    {
        QMessageBox::about(this, "Error", "Error loading chip database file!");
        return;
    }
    dataChips = datfile.readAll();
    datfile.close();
    //parsing dat file
    ui->statusBar->showMessage("Parsing DAT file");
    //parsing qbytearray
    char txtBuf[0x30];
    int i, j, recNo, dataPoz, dataSize, delay;
    uint32_t chipSize;
    uint16_t blockSize;
    unsigned char tmpBuf;
    dataPoz = 0;
    recNo = 0;
    QStringList verticalHeader;
    dataSize = dataChips.length();
    while (dataPoz < dataSize)
    {
        for (j=0; j<0x30; j++)
             {
                 txtBuf[j] = 0;
             }
        j = 0;
             while ((j < 0x10) && (dataChips[recNo * 0x44 + j] != ',')) // ASCII data reading
             {
                 txtBuf[j] = dataChips[recNo * 0x44 + j];
                 j++;
             }
             if (txtBuf[1] == 0x00) break;
             chips[recNo].chipTypeTxt = QByteArray::fromRawData(txtBuf, 0x30);
         for (i=0; i<0x30; i++)
             {
                 txtBuf[i] = 0;
             }
         j++;
         i = 0;
         while ((i < 0x20) && (dataChips[recNo * 0x44 + j] != ',')) // ASCII data reading
         {
             txtBuf[i] = dataChips[recNo * 0x44 + j];
             j++;
             i++;
         }
             chips[recNo].chipManuf = QByteArray::fromRawData(txtBuf, 0x30);
             for (i=0; i<0x30; i++)
                 {
                     txtBuf[i] = 0;
                 }
             j++;
             i = 0;
             while ((i < 0x30) && (dataChips[recNo * 0x44 + j] != '\0')) // ASCII data reading
             {
                 txtBuf[i] = dataChips[recNo * 0x44 + j];
                 j++;
                 i++;
             }
             chips[recNo].chipName = QByteArray::fromRawData(txtBuf, 0x30);            
             chips[recNo].chipJedecIDMan = static_cast<uint8_t>(dataChips[recNo * 0x44 + 0x32]);
             chips[recNo].chipJedecIDDev = static_cast<uint8_t>(dataChips[recNo * 0x44 + 0x31]);
             chips[recNo].chipJedecIDCap = static_cast<uint8_t>(dataChips[recNo * 0x44 + 0x30]);
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x34]);
             chipSize = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x35]);
             chipSize = chipSize + tmpBuf * 256;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x36]);
             chipSize = chipSize + tmpBuf * 256 * 256;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x37]);
             chipSize = chipSize + tmpBuf * 256 * 256 * 256;
             chips[recNo].chipSize = chipSize;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x38]);
             blockSize = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x39]);
             blockSize = blockSize + tmpBuf * 256;
             chips[recNo].blockSize = blockSize;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x3a]);
             chips[recNo].chipTypeHex = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x3b]);
             chips[recNo].algorithmCode = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x3c]);
             delay = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x3d]);
             delay = delay + tmpBuf * 256;
             chips[recNo].delay = delay;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x3e]);
             chips[recNo].extend = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x40]);
             chips[recNo].eeprom = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x42]);
             chips[recNo].eepromPages = tmpBuf;
             tmpBuf = static_cast<unsigned char>(dataChips[recNo * 0x44 + 0x43]);
             if (tmpBuf == 0x00) chips[recNo].chipVCC = "3.3 V";
             if (tmpBuf == 0x01) chips[recNo].chipVCC = "1.8 V";
             if (tmpBuf == 0x02) chips[recNo].chipVCC = "5.0 V";
             dataPoz = dataPoz + 0x44; //next record
             verticalHeader.append(QString::number(recNo));
             recNo++;
    }
    max_rec = recNo;
    for (i = 0; i<max_rec; i++)
    {
        //replacing items to combobox Manufacture
        index2 = ui->comboBox_man->findText(chips[i].chipManuf);
                    if ( index2 == -1 ) ui->comboBox_man->addItem(chips[i].chipManuf);
    }
     ui->comboBox_man->setCurrentIndex(0);
     ui->statusBar->showMessage("");

}

MainWindow::~MainWindow()
{
    delete ui;
}
QString MainWindow::bytePrint(unsigned char z)
{
    unsigned char s;
    s = z / 16;
    if (s > 0x9) s = s + 0x37;
    else s = s + 0x30;
    z = z % 16;
    if (z > 0x9) z = z + 0x37;
    else z = z + 0x30;
    return QString(s) + QString(z);
}

void MainWindow::on_pushButton_clicked()
{
    statusCH347 = SPIDeviceInit();//ch341Configure(0x1a86,0x5512);
    ch341StatusFlashing();
    //ch341SetStream(currentSpeed);
    //Reading data from chip
    if ((currentNumBlocks > 0) && (currentBlockSize >0))
    {
    uint32_t addr = 0;
    uint32_t curBlock = 0;
    uint32_t j, k;
    int res;
    currentBlockSize = 64 * 1024;
    currentNumBlocks = currentChipSize / currentBlockSize;
    //progerssbar settings
    ui->progressBar->setRange(0, static_cast<int>(currentNumBlocks));
    ui->progressBar->setValue(0);
    //uint8_t buf[currentBlockSize];
    uint8_t *buf;
    buf = (uint8_t *)malloc(currentBlockSize);
    ui->pushButton->setStyleSheet("QPushButton{color:#fff;background-color:#f66;border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
    ui->statusBar->showMessage("Reading data from " + ui->comboBox_name->currentText());
    //Select bus speed
    //
    for (k = 0; k < currentNumBlocks; k++)
      {
         //int32_t ch341SpiRead(uint8_t *buf, uint32_t add, uint32_t len);
         res = FlashRead(curBlock * currentBlockSize, currentBlockSize, buf);//ch341SpiRead(buf, curBlock * currentBlockSize, currentBlockSize);
         qDebug() << "res=" << res;
         qDebug() << curBlock;
         // if res=-1 - error, stop
         if (statusCH347 != 1)
           {
             QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
             break;
           }
         if (res != 1)
           {
             QMessageBox::about(this, "Error", "Error reading sector " + QString::number(curBlock));
             break;
           }
         for (j = 0; j < currentBlockSize; j++)
            {
                  chipData[addr + j] = char(buf[addr + j - k * currentBlockSize]);
            }
         addr = addr + currentBlockSize;
         curBlock++;
         if (curBlock == 2) hexEdit->setData(chipData); //show buffer in hehedit while chip data is being loaded
         ui->progressBar->setValue(static_cast<int>(curBlock));
      }
    }
    else
    {
    //Not correct Number fnd size of blocks
     QMessageBox::about(this, "Error", "Before reading from chip please press 'Detect' button.");
    }
    hexEdit->setData(chipData);
    ui->statusBar->showMessage("");
    ui->progressBar->setValue(0);
    ui->pushButton->setStyleSheet("QPushButton{color:#fff;background-color:rgb(120, 183, 140);border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
    SPIDeviceRelease();//ch341Release();
}
QString MainWindow::sizeConvert(int a)
{
    QString rez;
    rez = "0";
    if (a < 1024) rez = QString::number(a) + " B";
    else if ((a < 1024 * 1024)) rez = QString::number(a/1024) + " K";
    else rez = QString::number(a/1024/1024) + " M";
    return rez;
}

void MainWindow::on_pushButton_2_clicked()
{
    //searching the connected chip in database
    statusCH347 = SPIDeviceInit();//ch341Configure(0x1a86,0x5512);
    ch341StatusFlashing();
    //ch341SetStream(currentSpeed);
    if (statusCH347 != 1)
      {
        QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
        return;
      }

    ui->pushButton_2->setStyleSheet("QPushButton {color:#fff;background-color:#f66;border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
    int i, index;
    // print JEDEC info
    struct JEDEC listJEDEC;
    //ui->sizeEdit->setText(QString::number(ch341SpiCapacity()));
    listJEDEC = GetJedecId();
    if ((listJEDEC.man == 0xff) && (listJEDEC.type == 0xff) && (listJEDEC.capacity == 0xff))
    {
        QMessageBox::about(this, "Error", "The chip is not connect or missing!");
        ui->pushButton_2->setStyleSheet("QPushButton{color:#fff;background-color:rgb(120, 183, 140);border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
        SPIDeviceRelease();//ch341Release();
        return;
    }
    ui->jedecEdit->setText(bytePrint(listJEDEC.man) + " " + bytePrint(listJEDEC.type) + " " + bytePrint(listJEDEC.capacity));
    for (i = 0; i< max_rec; i++)
    {
        if ((listJEDEC.man == chips[i].chipJedecIDMan) && (listJEDEC.type == chips[i].chipJedecIDDev) && (listJEDEC.capacity == chips[i].chipJedecIDCap))
        {            
            index = ui->comboBox_man->findText(chips[i].chipManuf);
                        if ( index != -1 )
                        { // -1 for not found
                           ui->comboBox_man->setCurrentIndex(index);
                        }
                        index = ui->comboBox_name->findText(chips[i].chipName);
                                    if ( index != -1 )
                                    { // -1 for not found
                                       ui->comboBox_name->setCurrentIndex(index);
                                    }

            index = ui->comboBox_size->findData(chips[i].chipSize);
            if ( index != -1 )
            { // -1 for not found
               ui->comboBox_size->setCurrentIndex(index);
            }
            index = ui->comboBox_page->findData(chips[i].blockSize);
            if ( index != -1 )
            { // -1 for not found
               ui->comboBox_page->setCurrentIndex(index);
            }
            index = ui->comboBox_vcc->findText(chips[i].chipVCC);
            if ( index != -1 )
            { // -1 for not found
               ui->comboBox_vcc->setCurrentIndex(index);
            }

            ui->pushButton_2->setStyleSheet("QPushButton{color:#fff;background-color:rgb(120, 183, 140);border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
            break;
        }
    }
    ui->pushButton_2->setStyleSheet("QPushButton{color:#fff;background-color:rgb(120, 183, 140);border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
    SPIDeviceRelease();//ch341Release();
}

void MainWindow::on_comboBox_size_currentIndexChanged(int index)
{
    //qDebug() <<"size="<< ui->comboBox_size->currentData().toInt() << " block_size=" << ui->comboBox_page->currentData().toInt();
    currentChipSize = ui->comboBox_size->currentData().toUInt();
    currentBlockSize = ui->comboBox_page->currentData().toUInt();
    if ((currentChipSize !=0) && (currentBlockSize!=0))
    {
        currentNumBlocks = currentChipSize / currentBlockSize;
        chipData.resize(static_cast<int>(currentChipSize));
        for (uint32_t i=0; i < currentChipSize; i++)
        {
            chipData[i] = char(0xff);
        }
        hexEdit->setData(chipData);
    }
    index = index + 0;
}

void MainWindow::on_comboBox_page_currentIndexChanged(int index)
{
    currentChipSize = ui->comboBox_size->currentData().toUInt();
    currentBlockSize = ui->comboBox_page->currentData().toUInt();
    if ((currentChipSize !=0) && (currentBlockSize!=0))
    {
        currentNumBlocks = currentChipSize / currentBlockSize;
        chipData.resize(static_cast<int>(currentChipSize));
        for (uint32_t i=0; i < currentChipSize; i++)
        {
            chipData[i] = char(0xff);
        }
        hexEdit->setData(chipData);
    }
    index = index + 0;
}

void MainWindow::on_comboBox_speed_currentIndexChanged(int index)
{
    if (ui->comboBox_speed->currentData().toInt() != -1)
    {
       currentSpeed =  static_cast<unsigned char>(ui->comboBox_speed->currentData().toInt());
       //ch341SetStream(currentSpeed);
    }
    index = index + 0;
}

void MainWindow::on_actionDetect_triggered()
{
   MainWindow::on_pushButton_2_clicked();
}

void MainWindow::on_actionSave_triggered()
{

    ui->statusBar->showMessage("Saving file");
    fileName = QFileDialog::getSaveFileName(this,
                                QString::fromUtf8("Save file"),
                                QDir::currentPath(),
                                "Data Images (*.bin);;All files (*.*)");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::about(this, "Error", "Error saving file!");
        return;
    }
    file.write(hexEdit->data());
    file.close();
}

void MainWindow::on_actionErase_triggered()
{
    statusCH347 = SPIDeviceInit();//ch341Configure(0x1a86,0x5512);
    ch341StatusFlashing();
    //ch341SetStream(currentSpeed);
    int i;
    if (statusCH347 != 1)
      {
        QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
        return;
      }
    ui->statusBar->showMessage("Erasing the " + ui->comboBox_name->currentText());
    ui->checkBox->setStyleSheet("QCheckBox{font-weight:600;}");
    ui->centralWidget->repaint();
    ui->progressBar->setRange(0, 100);
    ui->progressBar->setValue(50);
    FlashChipErase();//ch341EraseChip();
    //for (i=0; i < 120; i++)
    //{
    //   sleep(1);
       //qDebug() << "Status=" << ch341ReadStatus() << " " << i;
       //if (statusCH347() == 0) break;
    //}
    //qDebug() << "Status=" << ch341ReadStatus();
    ui->checkBox->setStyleSheet("");
    ui->statusBar->showMessage("");
    ui->progressBar->setValue(0);
    ui->centralWidget->repaint();
    SPIDeviceRelease();//ch341Release();
}

void MainWindow::on_actionUndo_triggered()
{
    hexEdit->undo();
}

void MainWindow::on_actionRedo_triggered()
{
    hexEdit->redo();
}

void MainWindow::on_actionOpen_triggered()
{
    ui->statusBar->showMessage("Opening file");
    fileName = QFileDialog::getOpenFileName(this,
                                QString::fromUtf8("Open file"),
                                QDir::currentPath(),
                                "Data Images (*.bin);;All files (*.*)");
    ui->statusBar->showMessage("Current file: " + fileName);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {

        return;
    }
    chipData = file.readAll();
    hexEdit->setData(chipData);
    file.close();
    ui->statusBar->showMessage("");
}

void MainWindow::on_actionWrite_triggered()
{
    //Writting data to chip
    currentBlockSize = 256;
    currentNumBlocks = currentChipSize / 256;
    statusCH347 = SPIDeviceInit();//ch341Configure(0x1a86,0x5512);
    ch341StatusFlashing();
    //ch341SetStream(currentSpeed);
    if (statusCH347 != 1)
      {
        QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
        return;
      }
    if ((currentNumBlocks > 0) && (currentBlockSize >0))
    {
    uint32_t addr = 0;
    uint32_t curBlock = 0;
    uint32_t j, k;
    int res;
    ui->statusBar->showMessage("Writing data to " + ui->comboBox_name->currentText());
    //progerssbar settings
    ui->progressBar->setRange(0, static_cast<int>(currentNumBlocks));
    ui->checkBox_2->setStyleSheet("QCheckBox{font-weight:800;}");
    chipData = hexEdit->data();
    //uint8_t buf[currentBlockSize];
    uint8_t *buf;
    buf = (uint8_t *)malloc(currentBlockSize);
    qDebug() << "CurrentBlockSize=" << currentBlockSize;
    for (k = 0; k < currentNumBlocks; k++)
      {

         for (j = 0; j < currentBlockSize; j++)
            {
               buf[addr + j - k * currentBlockSize] = static_cast<uint8_t>(chipData[addr + j]) ;
            }
         //int32_t ch341SpiWrite(uint8_t *buf, uint32_t add, uint32_t len)
         res = FlashSinglePageProgram(curBlock * currentBlockSize, buf, currentBlockSize);
         //res = FlashWrite(curBlock * currentBlockSize, buf, currentBlockSize);//ch341SpiWrite(buf, curBlock * currentBlockSize, currentBlockSize);
         qDebug() << "res=" << res;
         // if res=-1 - error, stop
         if (statusCH347 != 1)
           {
             QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
             break;
           }
         if (res != 1)
           {
             QMessageBox::about(this, "Error", "Error writing sector " + QString::number(curBlock));
             break;
           }
         addr = addr + currentBlockSize;
         curBlock++;
         ui->progressBar->setValue(static_cast<int>(curBlock));

      }
    }
    else
    {
    //Not correct Number fnd size of blocks
     QMessageBox::about(this, "Error", "Before reading from chip please press 'Detect' button.");
    }
    ui->progressBar->setValue(0);
    ui->checkBox_2->setStyleSheet("");
    ui->statusBar->showMessage("");
    SPIDeviceRelease();//ch341Release();
}

void MainWindow::on_actionRead_triggered()
{
    MainWindow::on_pushButton_clicked();
}

void MainWindow::on_actionExit_triggered()
{
    SPIDeviceRelease();//ch341Release();
    MainWindow::close();
}

void MainWindow::on_comboBox_man_currentIndexChanged(int index)
{
    int i, index2;
    QString txt="";
    if (max_rec > 0)
    {
       txt = ui->comboBox_man->currentText().toUtf8();
       ui->comboBox_name->clear();
       ui->comboBox_name->addItem("");
       for (i = 0; i<max_rec; i++)
       {
           //replacing items to combobox chip Name
           if (txt.compare(chips[i].chipManuf)==0)
           {
           index2 = ui->comboBox_name->findText(chips[i].chipName);
                    if ( index2 == -1 ) ui->comboBox_name->addItem(chips[i].chipName);
           }
       }
       ui->comboBox_name->setCurrentIndex(0);
       ui->comboBox_vcc->setCurrentIndex(0);
       ui->comboBox_page->setCurrentIndex(0);
       ui->comboBox_size->setCurrentIndex(0);
       ui->statusBar->showMessage("");
   }
   index = index + 0;
}

void MainWindow::on_comboBox_name_currentIndexChanged(const QString &arg1)
{
    int i, index;
    QString manName = ui->comboBox_man->currentText();
    if (arg1.compare("") !=0)
    {

       for (i = 0; i < max_rec; i++)
       {
           if ((manName.compare(chips[i].chipManuf)==0) && (arg1.compare(chips[i].chipName)==0))
           {
               index = ui->comboBox_size->findData(chips[i].chipSize);
               if ( index != -1 )
               { // -1 for not found
                  ui->comboBox_size->setCurrentIndex(index);
               }
               index = ui->comboBox_page->findData(chips[i].blockSize);
               if ( index != -1 )
               { // -1 for not found
                  ui->comboBox_page->setCurrentIndex(index);
               }
               index = ui->comboBox_vcc->findText(chips[i].chipVCC);
               if ( index != -1 )
               { // -1 for not found
                  ui->comboBox_vcc->setCurrentIndex(index);
               }
           }
       }

    }
}

void MainWindow::on_actionVerify_triggered()
{
    //Reading data from chip
    statusCH347 = SPIDeviceInit();//ch341Configure(0x1a86,0x5512);
    ch341StatusFlashing();
    //ch341SetStream(currentSpeed);
    if ((currentNumBlocks > 0) && (currentBlockSize >0))
    {
    uint32_t addr = 0;
    uint32_t curBlock = 0;
    uint32_t j, k;
    int  res;
    //progerssbar settings
    ui->progressBar->setRange(0, static_cast<int>(currentNumBlocks));
    ui->progressBar->setValue(0);
    //uint8_t buf[currentBlockSize];
    uint8_t *buf;
    buf = (uint8_t *)malloc(currentBlockSize);
    chipData = hexEdit->data();
    ui->checkBox_3->setStyleSheet("QCheckBox{font-weight:800;}");
    ui->statusBar->showMessage("Veryfing data from " + ui->comboBox_name->currentText());
    //Select bus speed
    //
    for (k = 0; k < currentNumBlocks; k++)
      {
         //int32_t ch341SpiRead(uint8_t *buf, uint32_t add, uint32_t len);
         res = FlashRead(curBlock * currentBlockSize, currentBlockSize, buf);//ch341SpiRead(buf, curBlock * currentBlockSize, currentBlockSize);
         // if res=-1 - error, stop
         if (statusCH347 != 1)
           {
             QMessageBox::about(this, "Error", "Programmer CH341a is not connected!");
             break;
           }
         if (res != 1)
           {
             QMessageBox::about(this, "Error", "Error reading sector " + QString::number(curBlock));
             break;
           }
         for (j = 0; j < currentBlockSize; j++)
            {
                 if (chipData[addr + j] != char(buf[addr + j - k * currentBlockSize]))
                 {
                     //error compare
                     QMessageBox::about(this, "Error", "Error comparing data!\nAddress:   " + hexiAddr(addr + j) + "\nBuffer: " + bytePrint(static_cast<unsigned char>(chipData[addr + j])) + "    Chip: " + bytePrint(static_cast<unsigned char>(buf[addr + j - k * currentBlockSize])));
                     ui->statusBar->showMessage("");
                     ui->checkBox_3->setStyleSheet("");
                     return;
                 }
            }
         addr = addr + currentBlockSize;
         curBlock++;
         ui->progressBar->setValue(static_cast<int>(curBlock));
      }
    }
    else
    {
    //Not correct Number fnd size of blocks
     QMessageBox::about(this, "Error", "Before reading from chip please press 'Detect' button.");
    }
    ui->statusBar->showMessage("");
    ui->progressBar->setValue(0);
    ui->checkBox_3->setStyleSheet("");
    SPIDeviceRelease();//ch341Release();
}
QString MainWindow::hexiAddr(uint32_t add)
{
 QString rez = "";
 uint8_t A,B,C,D;
 D = 0xFF & add;
 add = add >> 8;
 C = 0xFF & add;
 add = add >> 8;
 B = 0xFF & add;
 add = add >> 8;
 A = 0xFF & add;
 rez = bytePrint(A) + bytePrint(B) + bytePrint(C) + bytePrint(D);
 return rez;
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->pushButton_3->setStyleSheet("QPushButton{color:#fff;background-color:#f66;border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
    if (ui->checkBox->isChecked()) MainWindow::on_actionErase_triggered();
    if (ui->checkBox_2->isChecked()) MainWindow::on_actionWrite_triggered();
    if (ui->checkBox_3->isChecked()) MainWindow::on_actionVerify_triggered();
    ui->pushButton_3->setStyleSheet("QPushButton{color:#fff;background-color:rgb(120, 183, 140);border-radius: 20px;border: 2px solid #094065;border-radius:8px;font-weight:600;}");
}

void MainWindow::receiveAddr(QString addressData)
{
    uint32_t ee, blockEndAddr = 0;
    int e,t;
    QString endType;
    e = addressData.indexOf("-");
    t = addressData.length();
    blockStartAddr = 0;
    blockLen = 0;
    endType = addressData.mid(t - 1, 1);
    blockStartAddr = hexToInt(addressData.mid(0, e));
    if (endType.compare("*")==0)
    {
        blockEndAddr = hexToInt(addressData.mid(e + 1, t - e - 2));
        if (blockEndAddr < blockStartAddr)
        {
            QMessageBox::about(this, "Error", "The end address must be greater than the starting addres.");
            return;
        }
        blockLen = blockEndAddr - blockStartAddr;
    }
    else blockLen = hexToInt(addressData.mid(e + 1, t - e - 2));
    //qDebug() << blockStartAddr << " " << blockEndAddr << " " << blockLen;
    block.resize(static_cast<int>(blockLen));
    chipData = hexEdit->data();
    for (ee = 0; ee < blockLen; ee++)
    {
        block[ee] = chipData[ee + blockStartAddr];
    }
    ui->statusBar->showMessage("Saving block");
    fileName = QFileDialog::getSaveFileName(this,
                                QString::fromUtf8("Save block"),
                                QDir::currentPath(),
                                "Data Images (*.bin);;All files (*.*)");
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::about(this, "Error", "Error saving file!");
        return;
    }
    file.write(block);
    file.close();
}

void MainWindow::receiveAddr2(QString addressData)
{
    uint32_t ee;
    QString endType;
    blockStartAddr = 0;
    blockLen = 0;
    blockStartAddr = hexToInt(addressData);
    ui->statusBar->showMessage("Opening block");
    fileName = QFileDialog::getOpenFileName(this,
                                QString::fromUtf8("Open block"),
                                QDir::currentPath(),
                                "Data Images (*.bin);;All files (*.*)");
    ui->statusBar->showMessage("Current file: " + fileName);
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {

        return;
    }
    block = file.readAll();
    blockLen = static_cast<uint32_t>(block.size());
    chipData = hexEdit->data();
    if (blockStartAddr + blockLen > static_cast<uint32_t>(chipData.size()))
    {
        QMessageBox::about(this, "Error", "The end address out of image size!");
        return;
    }
    for (ee=0; ee < blockLen; ee++)
    {
        chipData[ee + blockStartAddr] = block[ee];
    }
    hexEdit->setData(chipData);
    file.close();
    ui->statusBar->showMessage("");
}

void MainWindow::on_actionSave_Part_triggered()
{
    DialogSP* savePartDialog = new DialogSP();
    savePartDialog->show();

    connect(savePartDialog, SIGNAL(sendAddr(QString)), this, SLOT(receiveAddr(QString)));
}

uint32_t MainWindow::hexToInt(QString str)
{
    unsigned char c;
    uint32_t len = static_cast<uint32_t>(str.length());
    QByteArray bstr = str.toLocal8Bit();
    if ((len > 0) && (len < 8))
    {
        uint32_t i, j = 1;
        uint32_t  addr = 0;
        for (i = len; i >0; i--)
        {
           c = static_cast<unsigned char>(bstr[i-1]);
           if ((c >= 0x30) && (c <=0x39)) addr =  addr + (c - 0x30) * j;
           if ((c >= 0x41) && (c <= 0x46)) addr = addr + (c - 0x37) * j;
           if ((c >= 0x61) && (c <= 0x66)) addr = addr + (c - 0x57) * j;
        j = j * 16;
        }
        return addr;
    }
    else return 0;
}

void MainWindow::on_actionLoad_Part_triggered()
{
    DialogRP* loadPartDialog = new DialogRP();
    loadPartDialog->show();

    connect(loadPartDialog, SIGNAL(sendAddr2(QString)), this, SLOT(receiveAddr2(QString)));
}

void MainWindow::on_actionFind_Replace_triggered()
{
    //DialogSP* savePartDialog = new DialogSP();
    //savePartDialog->show();
    SearchDialog* searchDialog = new SearchDialog(hexEdit);
    searchDialog->show();
}
void MainWindow::ch341StatusFlashing()
{
    if (statusCH347 == 1)
    {
        ui->eStatus->setText("Connected");
        ui->eStatus -> setStyleSheet("QLineEdit {border: 2px solid gray;border-radius: 5px;color:#000;background:#9f0;font-weight:600;border-style:inset;}");
    }
    else
    {
        ui->eStatus->setText("Not connected");
        ui->eStatus -> setStyleSheet("QLineEdit {border: 2px solid gray;border-radius: 5px;color:#fff;background:#f00;font-weight:600;border-style:inset;}");
    }
}

struct jedec
{
    uint8_t man;
    uint8_t dev;
    uint8_t cap;
};
//jedec GetJedecID();

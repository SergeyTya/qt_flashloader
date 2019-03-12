#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <iostream>
#include <QString>
#include <QSerialPortInfo>
#include <QThread>

#include <QFileDialog>
#include <QFile>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::get_SysPorts();
    if(ui->comboBox_PortName->count()>1)ui->comboBox_PortName->setCurrentIndex(1);
    ui->statusBar->showMessage("Hет соединения");

    ui->lineEdit_madr->setValidator( new QIntValidator(1, 255, this) );

    if(thread_bloader!= nullptr) return;
    thread_bloader= new QThread;//Создаем поток для порта платы
    bloader = new bootloader();//Создаем обьект по классу
    bloader->moveToThread(thread_bloader);//помешаем класс  в поток
    bloader->Port.moveToThread(thread_bloader);

    connect(thread_bloader, SIGNAL(started()), bloader, SLOT(process()));//Переназначения метода run
    connect(bloader, SIGNAL(close()), thread_bloader, SLOT(quit()));//Переназначение метода выход
    connect(thread_bloader, SIGNAL(finished()), bloader, SLOT(deleteLater()));//Удалить к чертям поток
    connect(bloader, SIGNAL(close()), thread_bloader, SLOT(deleteLater()));//Удалить к чертям поток
    connect(this, SIGNAL(connect_(QString, int, int)), bloader, SLOT(connect_(QString, int, int))); //
    connect(bloader, SIGNAL(writelog(QString, QString)), this, SLOT(writelog(QString, QString))); // лог

    thread_bloader->start();

}

void MainWindow::get_SysPorts()
{
    if(ui->comboBox_PortName->count() > 1 ) for(int i=1; i< ui->comboBox_PortName->count()+1 ; i++) ui->comboBox_PortName->removeItem(i); //delete rows
    const auto infos = QSerialPortInfo::availablePorts(); // get avaliable ports
    if(infos.size()==0){ ui->comboBox_PortName->setCurrentIndex(0); return;};
    for (const QSerialPortInfo &info : infos) ui->comboBox_PortName->addItem(info.portName()); // add new ports
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_Connect_clicked()
{
  /*
    if(ui->comboBox_PortName->currentIndex()==0) {qDebug()<<"port is'n selected"; return;};
    if(bloader->Port.isOpen()) return;
    connect_(ui->comboBox_PortName->currentText(), ui->comboBox_PortSpeed->currentText().toInt(), ui->lineEdit_madr->text().toInt());
    */

   bloader->Filename = QFileDialog::getOpenFileName( this,QString("Открыть файл"), QString(),QString("Файл прошивки (*.hex)"));
   connect_(ui->comboBox_PortName->currentText(), ui->comboBox_PortSpeed->currentText().toInt(), ui->lineEdit_madr->text().toInt());

}


void MainWindow::on_comboBox_PortName_activated(int index)
{
    if(index==0){
    MainWindow::get_SysPorts();
    ui->comboBox_PortName->showPopup();
    };
}

void MainWindow::writelog(QString msg, QString src)
{
    ui->consol->textCursor().insertText(src+"> "+msg+'\r'); // Вывод текста в консоль
    ui->consol->moveCursor(QTextCursor::End);//Scroll
};



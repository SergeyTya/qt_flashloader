#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <iostream>
#include <QString>
#include <QSerialPortInfo>
#include <QThread>
#include <QTextBlock>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    MainWindow::get_SysPorts();
    if(ui->comboBox_PortName->count()>1)ui->comboBox_PortName->setCurrentIndex(1);
    ui->statusBar->showMessage("выберите действие");

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
    connect(this, SIGNAL(connect_(QString, int, int, int)), bloader, SLOT(connect_(QString, int, int, int))); //
    connect(bloader, SIGNAL(writelog(QString, QString, bool)), this, SLOT(writelog(QString, QString,bool))); // лог
   // connect(bloader, SIGNAL(setStatus(QString)), this, SLOT(on_bootloader_Status_changed(QString)));

    thread_bloader->start();

    QString dtr =__TIMESTAMP__;
    this->setWindowTitle("Загрузчик МПЧ " + dtr );

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


void MainWindow::on_comboBox_PortName_activated(int index)
{
    if(index==0){
    MainWindow::get_SysPorts();
    ui->comboBox_PortName->showPopup();
    };
}

void MainWindow::writelog(QString msg, QString src, bool del)
{
    if(del==true)
    {
        int lineToDelete = ui->consol->document()->lineCount()-2;
        QTextBlock b =ui->consol->document()->findBlockByLineNumber(lineToDelete);
        if (b.isValid())
        {
            QTextCursor cursor(b);
            cursor.select(QTextCursor::BlockUnderCursor);
            cursor.removeSelectedText();
        };
    };
    if(msg=="")return;
    ui->consol->textCursor().insertText(src+" "+msg+'\r'); // Вывод текста в консоль
    ui->consol->moveCursor(QTextCursor::End);//Scroll
};



void MainWindow::on_pushButton_compare_clicked()
{
    bloader->Filename = nullptr;
    bloader->Filename = QFileDialog::getOpenFileName( this,QString("Открыть файл"), QString(),QString("Файл прошивки (*.hex)"));
    if(bloader->Filename == nullptr) return;

    connect_(ui->comboBox_PortName->currentText(),
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
            this->LDR_CMD::CMPR);

};

void MainWindow::on_pushButton_test_clicked()
{
    connect_(ui->comboBox_PortName->currentText(),
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
             this->LDR_CMD::LTST);
}

void MainWindow::on_pushButton_reset_clicked()
{
    connect_(ui->comboBox_PortName->currentText(),
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
            this->LDR_CMD::_RST);
}

void MainWindow::on_pushButton_search_clicked()
{
    QString name=ui->comboBox_PortName->currentText();

    connect_(name,
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
            this->LDR_CMD::SRCH);
}

void MainWindow::on_pushButton_break_clicked()
{
    if(!bloader->blnBreakReq && bloader->busy)
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Внимание!");
        msgBox.setInformativeText("Отменить текущее действие?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setDefaultButton(QMessageBox::Ok);
        int res = msgBox.exec();
        if (res == QMessageBox::Ok) bloader->blnBreakReq=true;
    }
    if(!bloader->busy)bloader->blnBreakReq=false;
}

/*
void MainWindow::on_bootloader_Status_changed(QString status)
{
    this->ui->statusBar->showMessage(status);
}*/

void MainWindow::on_pushButton_prog_clicked()
{
    if(bloader->busy) return;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Внимание!");
    msgBox.setInformativeText("Flash память будет перезаписана!\nНе отключайте питание до окончания процесса!\n\n\n Продолжить?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int res = msgBox.exec();
    if (res == QMessageBox::Ok) //нажата кнопка Ok
    {
        bloader->Filename = nullptr;
        bloader->Filename = QFileDialog::getOpenFileName( this,QString("Открыть файл"), QString(),QString("Файл прошивки (*.hex)"));
        if(bloader->Filename == nullptr) return;

        connect_(ui->comboBox_PortName->currentText(),
                 ui->comboBox_PortSpeed->currentText().toInt(),
                 ui->lineEdit_madr->text().toInt(),
                this->LDR_CMD::FLSH);
    }
    else //отмена
    {return;}

}

void MainWindow::on_pushButton_eraseflash_clicked()
{
    if(bloader->busy) return;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Внимание!");
    msgBox.setInformativeText("Flash память будет стерта!\n\n\n Продолжить?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int res = msgBox.exec();

    if (res == QMessageBox::Ok) //нажата кнопка Ok
    {
        connect_(ui->comboBox_PortName->currentText(),
                 ui->comboBox_PortSpeed->currentText().toInt(),
                 ui->lineEdit_madr->text().toInt(),
                 this->LDR_CMD::EFLH);
    }
    else //отмена
    {return;}
}

void MainWindow::on_pushButton_save_clicked()
{
    bloader->Filename = nullptr;
    bloader->Filename = QFileDialog::getSaveFileName(this,QString("Сохранить"), QString(),QString("Файл прошивки (*.hex)"));
    if(bloader->Filename == nullptr) return;

    connect_(ui->comboBox_PortName->currentText(),
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
            this->LDR_CMD::SAVE);
}

void MainWindow::on_pushButton_nattest_clicked()
{
    connect_(ui->comboBox_PortName->currentText(),
             ui->comboBox_PortSpeed->currentText().toInt(),
             ui->lineEdit_madr->text().toInt(),
             this->LDR_CMD::NLTS);
}

void MainWindow::on_pushButton_eraseprog_clicked()
{
    if(bloader->busy) return;

    QMessageBox msgBox;
    msgBox.setWindowTitle("Внимание!");
    msgBox.setInformativeText("Память настроек будет стерта!\n\n\n Продолжить?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int res = msgBox.exec();

    if (res == QMessageBox::Ok) //нажата кнопка Ok
    {
        connect_(ui->comboBox_PortName->currentText(),
                 ui->comboBox_PortSpeed->currentText().toInt(),
                 ui->lineEdit_madr->text().toInt(),
                 this->LDR_CMD::EPRM);
    }
    else //отмена
    {return;}
}

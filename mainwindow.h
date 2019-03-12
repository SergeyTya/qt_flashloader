#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QListView>
#include <QComboBox>
#include "bootloader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:

    void connect_(QString, int, int);

private slots:
    void get_SysPorts();
    void writelog(QString msg, QString src);
    void on_pushButton_Connect_clicked();
    void on_comboBox_PortName_activated(int index);

private:
    Ui::MainWindow *ui;
    QThread * thread_bloader = nullptr;
    bootloader * bloader = nullptr;

};

#endif // MAINWINDOW_H

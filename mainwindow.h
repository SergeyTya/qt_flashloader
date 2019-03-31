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

    void connect_(QString Port, int speed, int adr, int command);

private slots:
    void get_SysPorts();
    void writelog(QString, QString, bool);

    void on_comboBox_PortName_activated(int index);

    void on_pushButton_compare_clicked();

    void on_pushButton_test_clicked();

    void on_pushButton_reset_clicked();

    void on_pushButton_search_clicked();

    void on_pushButton_break_clicked();

  //  void on_bootloader_Status_changed(QString status);

    void on_pushButton_prog_clicked();

    void on_pushButton_eraseflash_clicked();

    void on_pushButton_save_clicked();

    void on_pushButton_nattest_clicked();

    void on_pushButton_eraseprog_clicked();

private:
    Ui::MainWindow *ui;
    QThread * thread_bloader = nullptr;
    bootloader * bloader = nullptr;

    enum  LDR_CMD{

        SRCH = 1,
        FLSH = 2,
        CMPR = 3,
        SAVE = 4,
        LTST = 5,
        EFLH = 6,
        EPRM = 7,
        _RST = 8,
        CHNL = 9,
        NLTS = 10
    };

};

#endif // MAINWINDOW_H

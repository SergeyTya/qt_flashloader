#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>
#include <QSerialPort>
#include "crc16.h"

#define _mBDEBUG(msg) { qDebug()<<"Загрузчик: "<< msg ; writelog(msg, " "); };

class bootloader : public QObject
{
    Q_OBJECT
public:
    explicit bootloader(QObject *parent = nullptr);
    ~bootloader();
    QSerialPort Port;
    QString Filename = ""; // имя файла

signals:
    void close();
    void error_(QString err);
    void writelog(QString msg, QString src);

public slots:
    void process();
    void handleError(QSerialPort::SerialPortError);
    void connect_(QString, int, int);


    int readHex(QString,QByteArray,int*);


private:
   int devAdr;
   int iFlashStartAdr=0; // базовый адрес прошивки из файла

   QByteArray  qbaFileHexStrg; // Данные из файла
   QByteArray  qbaDevHexStrg; // Данные cчитанные из устройства файла

   QByteArray sendModbusReq(char *, int );
   bool getModbusId();
   bool setModbusToBootCmd();
   bool getBootID();
   bool verifyHex();
   bool eraseMainFlash();
   bool eraseAddFlash();
   bool progFlash();
};

#endif // BOOTLOADER_H

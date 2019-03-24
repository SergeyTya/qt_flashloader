#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>
#include <QSerialPort>
#include "crc16.h"

#define _mBDEBUG(msg) { qDebug()<<"Загрузчик: "<< msg ;/* writelog(msg, " ", false);*/ };

class bootloader : public QObject
{
    Q_OBJECT
public:
    explicit bootloader(QObject *parent = nullptr);
    ~bootloader();
    QSerialPort Port;
    QString Filename = ""; // имя файла
    bool blnBreakReq = false;
    bool busy = false;

signals:
    void close();
    void error_(QString err);
    void writelog(QString, QString, bool);
    void setStatus(QString status);

public slots:
    void process();
    void handleError(QSerialPort::SerialPortError);
    void connect_(QString port, int speed, int adr, int command);


    int readHex(QString,QByteArray *,int*);
    int writeHex(QString,QByteArray,int);
    int readImage(QByteArray *, int, int);
    int compareImage(QByteArray,QByteArray,int);
    int writeImage(QByteArray image, int baseadr);
    int getBootID(QString *);
    int getBootID();
    int Reboot();
    int getNativeBoot();
    int eraseFlash();
    int eraseParam();

private:
   int devAdr;
   int iFlashStartAdr=0; // базовый адрес прошивки из файла

   QByteArray  qbaFileHexStrg; // Данные из файла
   QByteArray  qbaDevHexStrg; // Данные cчитанные из устройства файла

   QString loadehex;

   QByteArray sendModbusReq(char *, int );
   bool getModbusId(int adr);
   bool setModbusToBootCmd();

   bool clsCon(int);

   enum  resState{
       OK            =  0,
       OpenFileError = -1,
       LineStartError= -2,
       CRCError      = -3,
       BaseAdrEror   = -4,
       ShiftAdrError = -5,
       EndFileError  = -6,
       ImageEmpty    = -7,
       PortIsClose   = -8,
       TimeOut       = -9,
       canceled      = -10,
       FileNotFound  = -11,
       LoaderNotFound = -12,
       Done          = -13,
       EMPTY         =  255
   };


};

#endif // BOOTLOADER_H

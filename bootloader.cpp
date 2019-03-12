#include "bootloader.h"
#include <QDebug>
#include <QFileDialog>
#include <QFile>

bootloader::bootloader(QObject *parent) : QObject(parent)
{

}

bootloader::~bootloader()
{
    //_mBDEBUG("Поток закрыт")
    emit close();
}


void  bootloader::process()
{
   // _mBDEBUG("Запуск потока");
    connect(&Port,SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
}

void bootloader::connect_(QString name, int BaudRate , int adr){

 /*
   if(Port.isOpen())Port.close();

   Port.setPortName(name);
   Port.setBaudRate(BaudRate);
   Port.setDataBits(QSerialPort::Data8);
   Port.setParity(QSerialPort::NoParity);
   Port.setStopBits(QSerialPort::OneStop);
   Port.setFlowControl( QSerialPort::NoFlowControl);

   if (Port.open(QIODevice::ReadWrite)) {
      _mBDEBUG("Порт " + name + " открыт");
      } else {
      _mBDEBUG("Ошибка открытия порта " + name) return;
      };
  devAdr=adr;
   if(getModbusId()) setModbusToBootCmd();

   getBootID();

  Port.close();*/

   readHex(Filename, qbaFileHexStrg, &iFlashStartAdr);
}

bool bootloader::getModbusId()
{
    if(!Port.isOpen()) return false;
    _mBDEBUG("Modbus адрес " + QString::number(devAdr) + "... ");

   char * req = new char[255] {static_cast <char> (devAdr),0x2b,0xe,0x1,0x1,0,0};
   QByteArray res;

   res = sendModbusReq(req,5);
   delete[] req;

   if(res .count()==0) return false;
   QString str = res.mid(10,8)+" "+res.mid(32,10)+" "+res.mid(44,9);
   _mBDEBUG(str);
   return true;
}

bool bootloader::setModbusToBootCmd()
{
   if(!Port.isOpen()) return false;

   _mBDEBUG("Переход в загрузчик ...");
   char * req = new char[255] {static_cast <char> (devAdr),0x06,0x0,0x0,0x77,0x77};
   QByteArray res;
   res = sendModbusReq(req,6);
   delete[] req;
   if(res .count()==0) return false;
   return true;

}

bool bootloader::getBootID()
{
   if(!Port.isOpen()) return false;
   _mBDEBUG("ИД загрузчика ...");
   QByteArray req;
   req[0]='I';
   Port.write(req);
   if(!Port.waitForReadyRead(500)) return false;
   QString res = Port.readAll();
   _mBDEBUG(res);
    return true;
}

void bootloader::handleError(QSerialPort::SerialPortError error)
{

    if ( (Port.isOpen()) && (error != QSerialPort::NoError)){
        _mBDEBUG(Port.errorString());
       if(error!=QSerialPort::TimeoutError) Port.close();
    };

}

QByteArray bootloader::sendModbusReq(char * req, int len)
{
    crc16::usMBCRC16(req,len);
    Port.write(req ,len+2);
    if(!Port.waitForReadyRead(500)) return nullptr;
    return  Port.readAll();
}

int bootloader::readHex(QString Filename, QByteArray Storage, int* FlashStartAdr)
{
    bool endfile = false; //
    int count=0; // номер записи по счету
    int adrcount = 0; // текущий адрес полученный из номера записи
    int baseadr = 0; // базовый адрес
    int size = 0; // размер данных в одной записи
    int adr = 0;  //смещение адреса от базового
    int type = 0; // тип записи
    int crc=0;    // crc записи
    QByteArray str=""; // считанная строка
    int res = 0; //возвращаемый резльтат
    enum  resState{
        OK            =  0,
        OpenFileError = -1,
        LineStartError= -2,
        CRCError      = -3,
        BaseAdrEror   = -4,
        ShiftAdrError = -5,
        EndFileError  = -6,
    };

    Storage.clear();
    _mBDEBUG("Файл прошивки :");_mBDEBUG(Filename);
    QFile file (Filename);
    if(file.isOpen())file.close();
    if(!file.open(QIODevice::ReadOnly))
    {
        _mBDEBUG("Ошибка открытия файла!");
        file.close();
        return OpenFileError;
    };
    while(true){
/*читаю строку*/
        str= file.readLine();
        if(str.count()==0) break;
        if (str.indexOf(":") != 0)
        {
           // _mBDEBUG("Ошибка чтения 1");
            res=LineStartError;
            break;
        };
        size = str.mid(1,2).toInt(nullptr,16)+4; // +4 так как служебная информация
        QByteArray fulldata; //перевожу в числа
        crc=0;
        for(int i=0; i<size; i++)
        {
            fulldata.append( static_cast <char> (str.mid(1+i*2,2).toInt(nullptr,16)));
            crc+=fulldata[i];

        };
        crc =0xff&(0 - crc);
       //Проверка CRC
        if (crc != str.mid(3+(size-1)*2,2).toInt(nullptr,16))
        {
         //   _mBDEBUG("Ошибка crc");
            res=CRCError;
            break;
        };
       // заполняю атрибуты
        size = fulldata[0];
        type = fulldata[3];
        adr = (fulldata[1] << 8) + fulldata[2];
        //_mBDEBUG( "адрес - "+QString::number(adr,16));
/*Проверяю тип записи*/
        if (type == 1) //Конец файла
        {
           endfile = true;
           // дополняем до размера кратного 256
           while( (static_cast<int>(Storage.count()/256) )*256 !=Storage.count() )
           {
                Storage.append(static_cast<char>(0xFF));
           };
           _mBDEBUG("Сичтанно "+ QString::number(Storage.count())+" байт.");
           break;
        };
        if (type == 5) continue;// иногда попадается
        if (type == 4) //Задание базового адреса
        {
            baseadr = (fulldata[4] << 24) + (fulldata[5] << 16); //новый базовый адрес
            if(adrcount==0)//если в начале файла
            {
                adrcount=baseadr;
               *FlashStartAdr = baseadr;
            } else //если в середине файда
            {
               if(baseadr<adrcount)
               {
                  // _mBDEBUG("Ошибка целостности адреса 1");
                   res=BaseAdrEror;
                   break;
               };
               for(int i =0 ; i<(baseadr-adrcount); i++) // заполняю дырку
               {
                   //_mBDEBUG( "Пропуск "+ QString::number(i))
                   Storage.append(static_cast<char>(0xFF));
               };
               adrcount=baseadr;
              //_mBDEBUG( "Новое смещение строка "+QString::number(count)+".Новый адрес 0х"+ QString::number(baseadr, 16));
            };
            continue;
        };

/* Проверка пропущенных адресов*/
        int tmp= ((str.mid(3,4).toInt(nullptr,16))&0xFFFF)-(adrcount&0xFFFF);
        if( tmp!=0 )
        {
           // _mBDEBUG("ПРЕДУПРЕЖДЕНИЕ: Пропуск адреса в строке "+QString::number(count) );
           // _mBDEBUG("   Текущий адрес 0x"+QString::number(adrcount&0xFFFF,16)+". Адрес из файла 0x"+ str.mid(3,4));
           // _mBDEBUG("   Пропущено "+QString::number(tmp)+ " байт." );

            if(tmp<0)
            {
               // _mBDEBUG("Ошибка целостности адреса 2");
                res=ShiftAdrError;
                break;
            }
            for(int i =0 ; i<tmp; i++) //Добовление пропущенных байт
            {
                Storage.append(static_cast<char>(0xFF));
            };

            adrcount+=tmp;
        };
/*добавляем данные в буфер*/
        Storage.append(fulldata.mid(3,size));
        /*счетчик адреса*/
        adrcount+=size;
        /*счетчик строк*/
        count++;
    };
    file.close();
    /* если небыло последней записи - ошибка*/
    if (!endfile && res==OK)
    {
      //  _mBDEBUG("Ненайден конец файла");
        res=EndFileError;
    };
/* если завершено с ошибкой*/
    if(res!=OK)
    {
        _mBDEBUG("Ошибка чтения файла "+ QString::number(res)+".");
        Storage.clear();
        return res;
    };

    return Storage.count();
}

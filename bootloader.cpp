#include "bootloader.h"
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QThread>

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

bool bootloader::clsCon(int res)
{
    if(res<0)
    {
    if(Port.isOpen()) Port.close();
    this->busy=false;
    return true;
    }

    return false;
};

void bootloader::connect_(QString name, int BaudRate , int adr, int cmd){

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
    int res=0;

   if(this->busy) return;
   this->busy = true;

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
      _mBDEBUG("Ошибка открытия порта " + name);
      clsCon(-1);
      return;
      };
   devAdr=adr;

   switch (cmd) {
  case SRCH :
       this->setStatus("поиск устройства");
       writelog("Порт " +Port.portName()+" Скороть "+QString::number(Port.baudRate())+" бод.","",false);
       for (int i=this->devAdr;i<255;i++) {
           if(i!=this->devAdr)
           {
               writelog("","",true);
               writelog("","",true);
           }
           if(getModbusId(i)) break;
           if(blnBreakReq) {blnBreakReq=false;break;};
       }
       break;
  case FLSH :
        this->setStatus("прошивка");
       if( clsCon( readHex(Filename, &qbaFileHexStrg, &iFlashStartAdr)) ) break;
       if( getModbusId(devAdr)) setModbusToBootCmd();
       if( clsCon( getBootID() )) break ;
       if( clsCon( writeImage(qbaFileHexStrg,iFlashStartAdr)) ) break;
       if( clsCon( readImage(&this->qbaDevHexStrg, qbaFileHexStrg.count(),iFlashStartAdr))) break;
       clsCon( compareImage(this->qbaDevHexStrg,qbaFileHexStrg, qbaFileHexStrg.count())                );
       break;
  case CMPR :
       this->setStatus("сверка файла");
       if( clsCon( readHex(Filename, &qbaFileHexStrg, &iFlashStartAdr)) ) break;
       if( getModbusId(devAdr)) setModbusToBootCmd();
       if( clsCon( getBootID() )) break ;
       if( clsCon( readImage(&this->qbaDevHexStrg, qbaFileHexStrg.count(),iFlashStartAdr) ) ) break;
       clsCon( compareImage(this->qbaDevHexStrg,qbaFileHexStrg, qbaFileHexStrg.count())                );
       break;
  case SAVE :
       this->setStatus("сохранение образа прошивки");
       if(getModbusId(devAdr)) setModbusToBootCmd();
       if( clsCon( getBootID() )) break ;
       if( clsCon( readImage(&this->qbaDevHexStrg, qbaFileHexStrg.count(),iFlashStartAdr))) break;
       clsCon( writeHex(Filename,qbaFileHexStrg,iFlashStartAdr));
       break;
  case LTST :
       this->setStatus("тест загрузчика");
       if(getModbusId(devAdr)) setModbusToBootCmd();
        getBootID();
       break;
  case NLTS:
       this->setStatus("тест загрузчика");
       getNativeBoot();
       break;
  case EFLH :
       this->setStatus("стирание flahs памяти");
       break;
  case EPRM :
       this->setStatus("стирание памяти настроек");
       break;
  case _RST :
       this->setStatus("перезагрузка устройства");
       Reboot();
       break;
  case CHNL :
       break;
   };
   writelog("Завершен.","",false);
   this->setStatus("выберите действие");
   clsCon(-1);
}

bool bootloader::getModbusId(int adr)
{
   QByteArray  res;
   QString  str="";
   char req[10] = {static_cast <char> (adr),0x2b,0xe,0x1,0x1,0,0};

    if(!Port.isOpen()) return false;
   _mBDEBUG("Поиск Modbus устройства по адресу " + QString::number(adr) + "... ");

   writelog("Поиск Modbus устройства по адресу " + QString::number(adr) + "... ","",false);

   crc16::usMBCRC16(req,5);
   Port.readAll();
   Port.write(req ,5+2);
   Port.flush();
   while(Port.waitForReadyRead(1000)) {res.append(Port.readAll());};
   _mBDEBUG(res.length());
   if(res == nullptr) {writelog("Нет ответа.","",false);return false;};
   str = res.mid(10,8)+" "+res.mid(32,10)+" "+res.mid(44,9);
   _mBDEBUG(str);

   writelog("Найден [ "+str+" ]","",false);
   return true;
}

bool bootloader::setModbusToBootCmd()
{
   if(!Port.isOpen()) return false;

   _mBDEBUG("Переход в загрузчик ...");
  // writelog("Переход в загрузчик... ","",false);
   char * req = new char[255] {static_cast <char> (devAdr),0x06,0x0,0x0,0x77,0x77};
   QString res;
   res = sendModbusReq(req,6);
   delete[] req;
  // getBootID();
   return true;

}

int bootloader::getBootID(QString * id)
{
   if(!Port.isOpen()) return -1;
   _mBDEBUG("ИД загрузчика ...");
   writelog("Проверка загрузчика...","",false);
   QByteArray req;
   req[0]='I';
   Port.write(req);
   Port.flush();
   QString res="";
   while(Port.waitForReadyRead(300)) {QThread::msleep(10);res+=Port.readAll();};
   if(res==""){writelog("Нет ответа.","",false);return -1;};
   writelog("Определен [ "+ static_cast <QString>(res)+" ]" ,"",false);
   *id=res;
    _mBDEBUG(res);
    return 0;
}

int bootloader::getBootID()
{
   if(!Port.isOpen()) return -1;
   _mBDEBUG("ИД загрузчика ...");
   writelog("Проверка загрузчика...","",false);
   QByteArray req;
   req[0]='I';
   Port.write(req);
   Port.flush();
   QString res="";
   while(Port.waitForReadyRead(300)) res+=Port.readAll();
   if(res==""){writelog("Нет ответа.","",false);return -1;};
  // writelog("","",true);
   writelog("Определен [ "+ static_cast <QString>(res)+" ]" ,"",false);
   _mBDEBUG(res);
   return 0;
}

void bootloader::handleError(QSerialPort::SerialPortError error)
{

    if ( (Port.isOpen()) && (error != QSerialPort::NoError)){
        _mBDEBUG(Port.errorString());
       if(error==QSerialPort::TimeoutError)return;
        writelog(Port.errorString(),"",false);
        Port.close();
    };

}

QByteArray bootloader::sendModbusReq(char * req, int len)
{
    crc16::usMBCRC16(req,len);
    Port.readAll();
    Port.write(req ,len+2);
    Port.flush();
    QByteArray res;
    while(Port.waitForReadyRead(500)) res.append(Port.readAll());
    if(res.count()==0) return nullptr;
    return res;
}

int bootloader::readHex(QString Filename, QByteArray * image, int* FlashStartAdr)
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

    image->clear();
    _mBDEBUG("Файл прошивки :");_mBDEBUG(Filename);
    writelog("Файл прошивки: " +Filename,"", false);
    QFile file (Filename);
    if(file.isOpen())file.close();
    if(!file.open(QIODevice::ReadOnly))
    {
        _mBDEBUG("Ошибка открытия файла!");
         writelog("Ошибка открытия файла!","", false);
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
           while( (static_cast<int>(image->count()/256) )*256 !=image->count() )
           {
               image->append(static_cast<char>(0xFF));
           };
           _mBDEBUG("Сичтанно "+ QString::number(image->count())+" байт.");
           writelog("Сичтанно "+ QString::number(image->count())+" байт.","",false);
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
                   image->append(static_cast<char>(0xFF));
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
                image->append(static_cast<char>(0xFF));
            };

            adrcount+=tmp;
        };
/*добавляем данные в буфер*/
        image->append(fulldata.mid(4,(size) ));
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
        writelog("Ошибка чтения файла "+ QString::number(res)+".","",false);
        image->clear();
        return res;
    };

    return image->count();
}

int bootloader::writeHex(QString filename, QByteArray image, int flashStartAdr)
{
    enum  resState{
        OK            = 0,
        OpenFileError =-1,
        ImageEmpty    =-2,

    };
    int baseadr = 0; // размер данных в одной записи
    int adr = 0;  //смещение адреса от базового
    int count = 0; // тип записи
    int crc=0;    // crc записи
    QByteArray str=""; // считанная строка

    _mBDEBUG("Сохраняю в файл : ");_mBDEBUG(filename);
     writelog("Сохраняю в файл: " +Filename,"", false);
    QFile file (filename);
    if(file.isOpen())file.close();
    if(file.exists()) file.remove();
    if(!file.open(QIODevice::ReadWrite))
    {
        _mBDEBUG("Ошибка открытия файла!");
         writelog("Ошибка открытия файла!","", false);
        file.close();
        return OpenFileError;
    };

    if(image.count()<=0)
    {
        _mBDEBUG("Ошибка образ пустой");
        writelog("Ошибка образ пустой","", false);
        return ImageEmpty;
    };

    _mBDEBUG("Размер образа "+ QString::number(image.count()) + " байт.");
     writelog("Размер образа ","", false);

    for(int i=0; i< static_cast<int>(image.count()/16) ; i++ )
    {
        QByteArray linedata;

        if( (i==0 && baseadr!=flashStartAdr) || adr==0x10000) // смещения
        {

            if(i==0 && baseadr!=flashStartAdr)baseadr=flashStartAdr;


            if(adr==0x10000)
            {
                baseadr+=0x10000;
                adr=0;
            };

            linedata.append(static_cast <char> (0x2));
            linedata.append(static_cast <char> (0x0));
            linedata.append(static_cast <char> (0x0));
            linedata.append(static_cast <char> (0x4));
            linedata.append(static_cast <char>  (baseadr>>24));
            linedata.append(static_cast <char> ((baseadr>>16)&0xff));

           // qDebug()<<QString::number(baseadr,16);

            i--;

        }else{
             linedata.append(static_cast <char> (0x10));
             linedata.append(static_cast <char> ( (adr>>8)   ));
             linedata.append(static_cast <char> ( (adr&0x0ff) ));
             linedata.append(static_cast <char> (0x0));

             for(int k=0; k<16 ; k++)
             {
                 linedata.append(static_cast <char> ( image[k+count*16]));
             };

             adr+=16;
             count++;
        };

        //Crc
        for(int j=0;j<linedata.count(); j++)
        {
               crc+=linedata[j];
               if(static_cast<uchar> (linedata[j]) < 16 )str+="0";
               str+=QByteArray::number((static_cast<uint8_t>(linedata[j]))&0xff,16);

        };
        crc =0xff&(0 - crc);
        if(crc<=static_cast<int>(0xF))str+="0";
        str=":"+str+QByteArray::number(static_cast<uint8_t>(crc),16);
        crc=0;
        file.write(str+"\n");
        str="";
    };

    file.write(":00000001FF\n");

    file.flush();
    file.close();

    return OK;

}

int bootloader::compareImage(QByteArray image1,QByteArray image2, int size)
{
    for (int i=0;i<size;i++)
    {
     if(image1[i]!=image2[i])
     {
         writelog("Ошибка! Данные не совпали.","",false);
         return -1;
     };
    };
    writelog("Успешно! Данные совпали.","",false);
    return 0;
}

int bootloader::readImage(QByteArray * image, int size, int baseadr)
{
    enum  resState{
        OK          =  0,
        PortIsClose = -1,
        TimeOut  = -2,
        canceled = -3
    };

    if(!Port.isOpen()) { _mBDEBUG("Порт не открыт."); return PortIsClose;};
    image->clear();
    QByteArray req;
    req[0]='A';
    Port.write(req);
    Port.flush();
    req.clear();
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (baseadr >> 16));
    req.append(static_cast <char> (baseadr >> 24));
    Port.write(req);
    Port.flush();
    req.clear();
    while(Port.bytesAvailable()<1)if(!Port.waitForReadyRead(2500)) return TimeOut; ;
    Port.readAll();

    writelog("Считываю данные с устройства. Завершено 0%", "", false);

    for(int i=0; i<  static_cast<int>(size/256); i++)
    {
        req[0]='V';
        Port.write(req);
        req.clear();
        while(Port.bytesAvailable()!=256)if(!Port.waitForReadyRead(2500)) return TimeOut; ;
        QByteArray res = Port.readAll();
        image->append(res);
        res.clear();
        int progres= 100*(i+1)/static_cast<int>(size/256);
        QString str_log = "Считываю данные с устройства. Завершено " + QString::number( progres )+"%.";
        writelog(str_log, "", true);

        if(this->blnBreakReq) return canceled;

    };

return 0;
}

int bootloader::Reboot()
{
  if(!Port.isOpen()) { _mBDEBUG("Порт не открыт."); return -1;};
  QByteArray req;
  writelog("Перезагрузка...","",false);
  req[0]='R';
  Port.write(req);
  req.clear();
  while(Port.bytesAvailable()<1)if(!Port.waitForReadyRead(2500)) break ;
  QString res = Port.readAll();
  writelog("Перезагрузка ОК.","",true);
  QThread::sleep(1);
  getModbusId(devAdr);

  return 0;
};

int bootloader::getNativeBoot()
{
    enum  resState{
        OK          =  0,
        PortIsClose = -1,
        TimeOut  = -2,
        FileNotFound = -3,
        LoaderNotFound = -4,
    };

    QByteArray loaderImage;
    QString file;
    int baseadr=0;
    int brate=0;
    QByteArray req;
    QString res;

    writelog("Запуск загрузчика 1986ВЕ91Т.","", false);
    /*Файл загрузчика*/
    if(!Port.isOpen()) { _mBDEBUG("Порт не открыт."); return PortIsClose;};
    file= QDir::current().absolutePath()+"/loader.hex";
    this->readHex(file,&loaderImage, &baseadr);
    /* Синхронизация */
    brate =Port.baudRate();
    Port.setBaudRate(QSerialPort::Baud9600);
    req[0]=0;
    for (int i = 0; i < 512; i++) {Port.write(req);  };
    Port.flush();
    Port.waitForBytesWritten(2000);
    QThread::msleep(1000);
    writelog("Синхронизация ... ОК!","", false);
    /* Скорость передачи */
    req.clear();
    req.append('B');
    req.append(static_cast<char>( brate&0xff));
    req.append(static_cast<char>((brate>>8)&0xff));
    req.append(static_cast<char>((brate>>16)&0xff));
    req.append(static_cast<char>(0));
    Port.write(req);
    Port.flush();
    Port.waitForBytesWritten(2000);
    QThread::msleep(100);
    Port.setBaudRate(brate);
    writelog("Скорость обмена ... "+QString::number(brate)+".","", false);
    /*Приглашение*/
    req.clear();
    req.append(static_cast<char>(0xd) );
    Port.write(req);
    Port.flush();
    QThread::msleep(100);
    /* Загрузка программы в ОЗУ*/
    writelog("Загрузка в ОЗУ. Завершено 0%", "", false);
    for(int i=0; i<(loaderImage.count()/256); i++)
    {
        req.clear();
        req.append('L');
        req.append(static_cast <char> ((baseadr>>0 )&0xff));
        req.append(static_cast <char> ((baseadr>>8 )&0xff));
        req.append(static_cast <char> ((baseadr>>16)&0xff));
        req.append(static_cast <char> ((baseadr>>24)&0xff));
        req.append(static_cast <char> ((256>>0)&0xff));
        req.append(static_cast <char> ((256>>8)&0xff));
        req.append(static_cast <char> (0x0));
        req.append(static_cast <char> (0x0));

        Port.write(req);
        Port.flush();
        Port.waitForBytesWritten(2000);
        QThread::msleep(5);
        Port.write(loaderImage.mid(256*i,256));
        Port.flush();
        Port.waitForBytesWritten(2000);
        QThread::msleep(100);
        baseadr+=0x100;

        int progres= 100*(i+1)/static_cast<int>(loaderImage.count()/256);
        QString str_log = "Загрузка в ОЗУ. Завершено " + QString::number( progres )+"%.";
        writelog(str_log, "", true);

    };
    /*Команда на запуск с адресом таблицы прерываний*/
        req.clear();
        req.append('R');
        req.append(static_cast <char> (0x0));
        req.append(static_cast <char> (0x0));
        req.append(static_cast <char> (0x0));
        req.append(static_cast <char> (0x20));
        Port.write(req);
        Port.flush();
        while(Port.waitForReadyRead(500))Port.readAll();
        QString boot_ID;
        this->getBootID(&boot_ID);
        if(boot_ID != loaderImage.mid(256,14))
        {

            writelog("Сбой!","", false); return LoaderNotFound;
        };

    writelog("Успешно!","", false);
    return 0;
}

int bootloader::writeImage(QByteArray image, int baseadr)
{
    enum  resState{
        OK          =  0,
        PortIsClose = -1,
        TimeOut  = -2,
        ImageEmpty = -3,
    };
    QByteArray req;
    QString res;

    if(!Port.isOpen()) { _mBDEBUG("Порт не открыт."); return PortIsClose;};

     writelog("Запись образа в устройство.","", false);

    if(image.count()<=0)
    {
        writelog("Ошибка образ пустой","", false);
        return ImageEmpty;
    };


    req.clear();
    req[0]='A';
    Port.write(req);
    req.clear();
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (baseadr >> 16));
    req.append(static_cast <char> (baseadr >> 24));
    Port.write(req);
    req.clear();
    while(Port.bytesAvailable()<1)if(!Port.waitForReadyRead(2500)) return TimeOut; ;
//    if(!Port.waitForReadyRead(1000)){writelog("Ошибка. Неверный базовый адрес.","", false); return TimeOut;};
    Port.readAll();

    writelog("Стираю flash", "", false);
    req.clear();
    req[0]='E';
    Port.write(req);
    req.clear();
   // if(!Port.waitForReadyRead(1000)){return TimeOut;};
    while(Port.bytesAvailable()<3)if(!Port.waitForReadyRead(2500)) return TimeOut; ;
    res=Port.readAll();
    if(res!="EOK"){writelog("Сбой.","",false);return TimeOut;}
    writelog("Стираю flash ... OK.", "", true);

    req.clear();
    req[0]='A';
    Port.write(req);
    req.clear();
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (0x0));
    req.append(static_cast <char> (baseadr >> 16));
    req.append(static_cast <char> (baseadr >> 24));
    Port.write(req);
    req.clear();
    //if(!Port.waitForReadyRead(100)){writelog("Ошибка. Неверный базовый адрес.","", false); return TimeOut;};
    while(Port.bytesAvailable()<1)if(!Port.waitForReadyRead(2500)) return TimeOut; ;
    Port.readAll();

    writelog("Прошивка. Завершено 0%", "", false);
    for(int i=0 ; i< image.count()/256; i++)
    {
        req.clear();
        req[0]='P';
        Port.write(req);
        Port.write(image.mid(i*256,256));
        Port.flush();
        Port.waitForBytesWritten(2000);
        QThread::msleep(10);
        if(!Port.waitForReadyRead(1000))return TimeOut;
        res=Port.readAll();
        int progres= 100*(i+1)/static_cast<int>(image.count()/256);
        QString str_log = "Прошивка. Завершено " + QString::number( progres )+"%.";
        writelog(str_log, "", true);
    };

    return 0;
}

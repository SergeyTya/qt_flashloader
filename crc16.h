#ifndef CRC16_H
#define CRC16_H
#include <QListView>

namespace Ui {
class crc16;
}

class crc16
{
public:
   static char16_t usMBCRC16( char * pucFrame, int usLen );
};

#endif // CRC16_H

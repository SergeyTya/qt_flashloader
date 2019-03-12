#include <QTest>
#include <QApplication>
#include <QDir>
#include "bootloader.h"
#include "mainwindow.h"
#include "test.h"

test::test(QObject *parent) : QObject(parent)
{

}

void test::readHex()
{
    bootloader a;
    QByteArray strg;
    int adr;
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/EndFileErr.hex"  ,strg, &adr),-6);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/ShiftAdrErr.hex" ,strg, &adr),-5);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/BaseAdrErr.hex"  ,strg, &adr),-4);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/CRCErr.hex"      ,strg, &adr),-3);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/LineStartErr.hex",strg, &adr),-2);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + ""                          ,strg, &adr),-1);
    QCOMPARE(a.readHex( QDir::current().absolutePath() + "/test_hex/Norm.hex"        ,strg, &adr), 256);

}


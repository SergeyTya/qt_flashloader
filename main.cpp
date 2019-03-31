#include "mainwindow.h"
#include <QApplication>
//#include "test.h"
//#include <QTest>
#include <iostream>


int main(int argc, char *argv[])
{

    QApplication a(argc, argv);


//    QTest::qExec(new test, argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}

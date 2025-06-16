#include <QApplication>
#include <QScreen>

#include "NXApplication.h"
#include "FKLoginWidget.h"
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	nxApp->init();
	FKLoginWidget w;
	w.show();
	return a.exec();
}

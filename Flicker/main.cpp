#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include "NXApplication.h"
#include "FKLauncherShell.h"
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	nxApp->init();
	QFontDatabase::addApplicationFont(":/Resource/Font/iconfont.ttf");
	FKLauncherShell w;
	w.show();

	return a.exec();
}
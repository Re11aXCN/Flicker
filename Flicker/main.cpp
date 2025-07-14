#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include "NXApplication.h"
#include "FKLauncherShell.h"
#include "FKLogger.h"
int main(int argc, char* argv[])
{
    bool ok = FKLogger::getInstance().initialize("Flicker-Client", FKLogger::SingleFile, true);
    if (!ok)  return 1;

    QApplication a(argc, argv);
    nxApp->init();
    QFontDatabase::addApplicationFont(":/Resource/Font/iconfont.ttf");
    FKLauncherShell w;
    w.show();
    FK_CLIENT_INFO("Flicker Client started!!!");

    int ret = a.exec();  // 事件循环

    // 程序退出前显式关闭日志系统
    FKLogger::getInstance().shutdown();
    return ret;
}
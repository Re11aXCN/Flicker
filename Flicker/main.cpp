#include <QApplication>
#include <QScreen>
#include <QFontDatabase>
#include <NXApplication.h>
#include "Library/Logger/logger.h"
#include "FKLauncherShell.h"
int main(int argc, char* argv[])
{
    bool ok = Logger::getInstance().initialize("Flicker-Client", Logger::SingleFile, true);
    if (!ok)  return 1;

    QApplication a(argc, argv);
    nxApp->init();
    QFontDatabase::addApplicationFont(":/Resource/Font/iconfont.ttf");
    FKLauncherShell w;
    w.show();
    LOGGER_INFO("Flicker Client started!!!");

    int ret = a.exec();  // 事件循环

    // 程序退出前显式关闭日志系统
    Logger::getInstance().shutdown();
    return ret;
}
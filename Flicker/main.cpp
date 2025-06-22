#include <QApplication>
#include <QScreen>

#include "NXApplication.h"
#include "FKLoginWidget.h"
#include "Login_interface/responsive_form.h"
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);
	nxApp->init();
	FKLoginWidget w;
	w.show();
	/*Responsive_form login_form;
	login_form.show();*/
	return a.exec();
}

/*************************************************************************************
 *
 * @ Filename	 : FKLoginWidget.h
 * @ Description : 
 * 
 * @ Version	 : V1.0
 * @ Author		 : Re11a
 * @ Date Created: 2025/6/15
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_LOGINWIDGET_H_
#define FK_LOGINWIDGET_H_
#include <NXWidget.h>
#include <NXLineEdit.h>
#include <NXPushButton.h>
#include <NXToolButton.h>
#include <NXText.h>
#include <NXCheckBox.h>
#include <NXMessageButton.h>

#include "FKDef.h"
class QStackedWidget;
class FKLoginWidget : public NXWidget
{
	Q_OBJECT

public:
	explicit FKLoginWidget(QWidget* parent = nullptr);
	virtual ~FKLoginWidget();

private:
	// init
	void _initUi();
	void _initLoginPage();
	void _initRegisterPage();
	void _initForgetPasswordPage();
	void _initRegistryCallback();
	// private slots
	void _handleResponseRegisterSevice(const QString& response, Http::RequestId requestId, Http::RequestErrorCode errorCode);
	
	// utils
	void _showMessage(const QString& title, const QString& text, NXMessageBarType::MessageMode mode, NXMessageBarType::PositionPolicy position, int displayMsec = 2000);
	
	QHash<Http::RequestId, std::function<void(const QJsonObject&)>> _registerRequestHashMap;
	
	NXMessageButton* _pMessageButton{ nullptr };

	QStackedWidget* _pStackedWidget{ nullptr };
	NXLineEdit* _pRegisterUsernameLineEdit{ nullptr };
	NXLineEdit* _pRegisterEmailLineEdit{ nullptr };
	NXLineEdit* _pRegisterPasswordLineEdit{ nullptr };
	NXLineEdit* _pRegisterConfirmPasswordLineEdit{ nullptr };
	NXLineEdit* _pRegisterVerifyCodeLineEdit{ nullptr };
	NXPushButton* _pRegisterGetVerifyCodeButton{ nullptr };
	NXPushButton* _pRegisterConfirmButton{ nullptr };
	NXPushButton* _pRegisterCancelButton{ nullptr };

	QLabel* _pLoginAvatarLabel{ nullptr };
	NXLineEdit* _pLoginUsernameLineEdit{ nullptr };
	NXLineEdit* _pLoginPasswordLineEdit{ nullptr };
	NXCheckBox* _pLoginAutoLoginCheckBox{ nullptr };
	NXCheckBox* _pLoginRememberPasswordCheckBox{ nullptr };
	NXText* _pLoginSwitchToForgetPasswordPageButton{ nullptr };
	NXPushButton* _pLoginSubmitButton{ nullptr };
	NXText* _pLoginSwitchToRegisterPageButton{ nullptr };
	NXToolButton* _pLoginSwitchToQrCodeLoginPageButton{ nullptr };
};


#endif // !FK_LOGINWIDGET_H_

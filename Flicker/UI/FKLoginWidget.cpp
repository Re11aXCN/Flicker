#include "FKLoginWidget.h"
#include <NXIcon.h>
#include <QStackedWidget>
#include <QJsonDocument>
#include "Source/FKHttpManager.h"
constexpr int LOGIN_WIDGET_WIDTH = 430;
constexpr int LOGIN_WIDGET_HEIGHT = 330;
FKLoginWidget::FKLoginWidget(QWidget *parent)
    : NXWidget(parent)
{
	_initUi();

	QObject::connect(FKHttpManager::getInstance(), &FKHttpManager::registerServiceFinished, this, &FKLoginWidget::_handleResponseRegisterSevice);
}

FKLoginWidget::~FKLoginWidget()
{
}

/**
 * @brief : 初始化控件
 * @params: 无
 * @return: void

 * @PRECAUTIONS
 */
void FKLoginWidget::_initUi()
{
	// 窗口基础设置
	setWindowTitle("Flicker");
	setWindowIcon(QIcon(":/Resource/ico/Weixin_IDI_ICON1.ico"));
	setIsFixedSize(true);
	setFixedSize(LOGIN_WIDGET_WIDTH, LOGIN_WIDGET_HEIGHT);
	setAppBarHeight(30);
	setWindowButtonFlags(NXAppBarType::MinimizeButtonHint | NXAppBarType::CloseButtonHint);

	_pMessageButton = new NXMessageButton(this);
	_pMessageButton->setFixedSize(1, 1);
	_pMessageButton->setVisible(false);

	_pStackedWidget = new QStackedWidget(this);
	this->_initLoginPage();
	this->_initRegisterPage();
	_pStackedWidget->setCurrentIndex(0);

	QHBoxLayout* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(_pStackedWidget);

	QObject::connect(_pLoginSwitchToRegisterPageButton, &NXText::clicked, this, [this]() { _pStackedWidget->setCurrentIndex(LoginSide::Register); });
	QObject::connect(_pRegisterCancelButton, &NXPushButton::clicked, this, [this]() { _pStackedWidget->setCurrentIndex(LoginSide::Login); });
}

/**
 * @brief : 初始化登录页面
 * @params: 无
 * @return: void
 
 * @PRECAUTIONS
 * TODO: 
 1. 信号槽完善
 2. 数据信息数据库存储，包括头像显示，用户名密码等信息
 3. 自动登录功能实现
 4. 持久化存储，包括记住密码功能，多用户登录功能
 5. 错误处理，密码不对，不存在账号等情况
 */
void FKLoginWidget::_initLoginPage()
{
	constexpr int CENTRAL_WIDGET_WIDTH = 240;
	constexpr int CENTRAL_COMPONENT_HEIGHT = 32;
	constexpr int AVATAR_SIZE = 100;

	QWidget* loginPage = new QWidget(this);
	loginPage->setStyleSheet("background-color: #FFFFFF;");
	_pLoginAvatarLabel = new QLabel(loginPage);
	_pLoginUsernameLineEdit = new NXLineEdit(loginPage);
	_pLoginPasswordLineEdit = new NXLineEdit(loginPage);
	_pLoginAutoLoginCheckBox = new NXCheckBox("自动登录", loginPage);
	_pLoginRememberPasswordCheckBox = new NXCheckBox("记住密码", loginPage);
	_pLoginSwitchToForgetPasswordPageButton = new NXText("忘记密码", loginPage);
	_pLoginSubmitButton = new NXPushButton("登录", loginPage);
	_pLoginSwitchToRegisterPageButton = new NXText("注册账号", loginPage);
	_pLoginSwitchToQrCodeLoginPageButton = new NXToolButton(loginPage);
	_pStackedWidget->insertWidget(LoginSide::Login, loginPage);

	// =============================
	// 布局计算（Y坐标递进式）
	// =============================

	// 头像位置（顶部居中）
	constexpr int avatarY = 20;
	_pLoginAvatarLabel->setPixmap(QPixmap(":/Resource/Image/Login/avatar.jpg")
		.scaled(AVATAR_SIZE, AVATAR_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	_pLoginAvatarLabel->move((LOGIN_WIDGET_WIDTH - AVATAR_SIZE) / 2, avatarY);

	// 用户名框
	constexpr int usernameY = 135;
	_pLoginUsernameLineEdit->setPlaceholderText("微信号/手机号/邮箱");
	_pLoginUsernameLineEdit->setBorderRadius(0);
	_pLoginUsernameLineEdit->setIsClearButtonEnabled(false);
	_pLoginUsernameLineEdit->setContentsPaddings(5, 0, 5, 0);
	_pLoginUsernameLineEdit->setFixedSize(CENTRAL_WIDGET_WIDTH, CENTRAL_COMPONENT_HEIGHT);
	_pLoginUsernameLineEdit->move((LOGIN_WIDGET_WIDTH - CENTRAL_WIDGET_WIDTH) / 2, usernameY);

	// 密码框（用户名下方3px）
	constexpr int passwordY = usernameY + CENTRAL_COMPONENT_HEIGHT + 3;
	_pLoginPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pLoginPasswordLineEdit->setPlaceholderText("密码");
	_pLoginPasswordLineEdit->setBorderRadius(0);
	_pLoginPasswordLineEdit->setIsClearButtonEnabled(false);
	_pLoginPasswordLineEdit->setContentsPaddings(5, 0, 5, 0);
	_pLoginPasswordLineEdit->setFixedSize(CENTRAL_WIDGET_WIDTH, CENTRAL_COMPONENT_HEIGHT);
	_pLoginPasswordLineEdit->move((LOGIN_WIDGET_WIDTH - CENTRAL_WIDGET_WIDTH) / 2, passwordY);

	// 选项行（密码框下方6px，水平分布）
	constexpr int optionsY = passwordY + CENTRAL_COMPONENT_HEIGHT + 6;
	constexpr int optionsStartX = (LOGIN_WIDGET_WIDTH - CENTRAL_WIDGET_WIDTH) / 2;

	_pLoginAutoLoginCheckBox->setCheckIndicatorWidth(16);
	_pLoginAutoLoginCheckBox->setFixedWidth(74);
	_pLoginAutoLoginCheckBox->setTextStyle(NXTextType::CustomStyle, 12, QFont::Medium);
	_pLoginAutoLoginCheckBox->move(optionsStartX, optionsY);

	_pLoginRememberPasswordCheckBox->setCheckIndicatorWidth(16);
	_pLoginRememberPasswordCheckBox->setFixedWidth(74);
	_pLoginRememberPasswordCheckBox->setTextStyle(NXTextType::CustomStyle, 12, QFont::Medium);
	_pLoginRememberPasswordCheckBox->move(optionsStartX + 92, optionsY);  // 74+18间距

	_pLoginSwitchToForgetPasswordPageButton->setContentsMargins(0, 0, 0, 0);
	_pLoginSwitchToForgetPasswordPageButton->setIsAllowClick(true);
	_pLoginSwitchToForgetPasswordPageButton->setFixedSize(74, 21);
	_pLoginSwitchToForgetPasswordPageButton->setTextStyle(NXTextType::CustomStyle, 12, QFont::Medium);
	_pLoginSwitchToForgetPasswordPageButton->move(optionsStartX + 184, optionsY);  // 74*2+18*2

	// 登录按钮（选项行下方6px）
	constexpr int loginY = optionsY + CENTRAL_COMPONENT_HEIGHT;
	_pLoginSubmitButton->setFixedSize(CENTRAL_WIDGET_WIDTH, CENTRAL_COMPONENT_HEIGHT + 8);
	_pLoginSubmitButton->move((LOGIN_WIDGET_WIDTH - CENTRAL_WIDGET_WIDTH) / 2, loginY);

	// 底部辅助按钮（登录按钮下方）
	_pLoginSwitchToRegisterPageButton->setContentsMargins(0, 0, 0, 0);
	_pLoginSwitchToRegisterPageButton->setIsAllowClick(true);
	_pLoginSwitchToRegisterPageButton->setFixedSize(74, 21);
	_pLoginSwitchToRegisterPageButton->setTextStyle(NXTextType::CustomStyle, 12, QFont::Medium);
	_pLoginSwitchToRegisterPageButton->move(15, loginY + CENTRAL_COMPONENT_HEIGHT - 6);  // 视觉对齐调整

	QFont font;
	font.setPixelSize(24);
	_pLoginSwitchToQrCodeLoginPageButton->setNXIcon(NXIconType::Qrcode);
	_pLoginSwitchToQrCodeLoginPageButton->setFont(font);
	_pLoginSwitchToQrCodeLoginPageButton->setFixedSize(30, 30);
	_pLoginSwitchToQrCodeLoginPageButton->move(395, _pLoginSwitchToRegisterPageButton->y() - 4);  // 与注册按钮垂直对齐
}

/**
 * @brief : 初始化注册页面
 * @params: 无
 * @return: void
 
 * @PRECAUTIONS
 * TODO:
 1. 信号槽完善
 2. 密码位数8~20（除空格外允许键盘上的所有字符），邮箱格式验证，用户名限制仅仅字母、数字、下划线，错误时需要提示
 3. 两次密码验证匹配
 4. 邮箱验证码逻辑完善
 */
void FKLoginWidget::_initRegisterPage()
{
	auto initLineEditFunc = [this](NXLineEdit* lineEdit) -> void {
		lineEdit->setBorderRadius(0);
		lineEdit->setIsClearButtonEnabled(false);
		lineEdit->setContentsPaddings(5, 0, 5, 0);
		lineEdit->setFixedSize(220, 30);
		};
	auto initMultipleLineEditsFunc = [this, &initLineEditFunc]<typename... LineEdits>(LineEdits*... lineEdits) -> void {
		// 使用折叠表达式展开参数包
		(initLineEditFunc(lineEdits), ...);
	};

	QWidget* registerPage = new QWidget(this);
	QHBoxLayout* registerPageLayout = new QHBoxLayout(registerPage);
	QVBoxLayout* componentLayout = new QVBoxLayout();
	QHBoxLayout* verifyLayout = new QHBoxLayout();
	QHBoxLayout* buttonLayout = new QHBoxLayout();
	_pRegisterUsernameLineEdit = new NXLineEdit(registerPage);
	_pRegisterEmailLineEdit = new NXLineEdit(registerPage);
	_pRegisterPasswordLineEdit = new NXLineEdit(registerPage);
	_pRegisterConfirmPasswordLineEdit = new NXLineEdit(registerPage);
	_pRegisterVerifyCodeLineEdit = new NXLineEdit(registerPage);
	_pRegisterGetVerifyCodeButton = new NXPushButton("获取验证码", registerPage);
	_pRegisterConfirmButton = new NXPushButton("确认", registerPage);
	_pRegisterCancelButton = new NXPushButton("取消", registerPage);
	buttonLayout->addStretch();
	buttonLayout->addWidget(_pRegisterConfirmButton);
	buttonLayout->addSpacing(40);
	buttonLayout->addWidget(_pRegisterCancelButton);
	buttonLayout->addStretch();
	verifyLayout->addStretch();
	verifyLayout->addWidget(_pRegisterVerifyCodeLineEdit);
	verifyLayout->addSpacing(20);
	verifyLayout->addWidget(_pRegisterGetVerifyCodeButton);
	verifyLayout->addStretch();
	componentLayout->addStretch();
	componentLayout->addWidget(_pRegisterUsernameLineEdit);
	componentLayout->addSpacing(5);
	componentLayout->addWidget(_pRegisterEmailLineEdit);
	componentLayout->addSpacing(5);
	componentLayout->addWidget(_pRegisterPasswordLineEdit);
	componentLayout->addSpacing(5);
	componentLayout->addWidget(_pRegisterConfirmPasswordLineEdit);
	componentLayout->addSpacing(5);
	componentLayout->addLayout(verifyLayout);
	componentLayout->addSpacing(5);
	componentLayout->addLayout(buttonLayout);
	componentLayout->addStretch();
	registerPageLayout->addStretch();
	registerPageLayout->addLayout(componentLayout);
	registerPageLayout->addStretch();
	_pStackedWidget->insertWidget(LoginSide::Register, registerPage);

	initMultipleLineEditsFunc(_pRegisterUsernameLineEdit, _pRegisterEmailLineEdit, _pRegisterPasswordLineEdit, _pRegisterConfirmPasswordLineEdit, _pRegisterVerifyCodeLineEdit);
	_pRegisterVerifyCodeLineEdit->setFixedWidth(120);
	_pRegisterPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pRegisterConfirmPasswordLineEdit->setEchoMode(QLineEdit::Password);
}

void FKLoginWidget::_initForgetPasswordPage()
{

}

void FKLoginWidget::_initRegistryCallback()
{
	_registerRequestMap.insert(Http::RequestId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj) {
		if (jsonObj["error"].toInt() != Http::RequestErrorCode::SUCCESS) {
			this->_showMessage("ERROR", "注册表单错误，请填写！", NXMessageBarType::Error, NXMessageBarType::Bottom);
			return;
		}
		QString email = jsonObj["email"].toString();
		this->_showMessage("SUCCESS", "验证码已发送到邮箱，注意查收！", NXMessageBarType::Success, NXMessageBarType::Top);
		qDebug() << "email is " << email;
		});
}

void FKLoginWidget::_handleResponseRegisterSevice(QString&& response, Http::RequestId requestId, Http::RequestErrorCode errorCode)
{
	if (errorCode != Http::RequestErrorCode::SUCCESS) {
		this->_showMessage("ERROR", "网络请求错误！", NXMessageBarType::Error, NXMessageBarType::Bottom);
		return;
	}
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());

	if (jsonDoc.isNull() || !jsonDoc.isObject()) {
		this->_showMessage("ERROR", "JSON解析错误！", NXMessageBarType::Error, NXMessageBarType::Bottom);
		return;
	}

	QJsonObject jsonObject = jsonDoc.object();
}

void FKLoginWidget::_showMessage(const QString& title, const QString& text, NXMessageBarType::MessageMode mode, NXMessageBarType::PositionPolicy position, int displayMsec)
{
	_pMessageButton->setBarTitle(title);
	_pMessageButton->setBarText(text);
	_pMessageButton->setMessageMode(mode);
	_pMessageButton->setPositionPolicy(position);
	_pMessageButton->setDisplayMsec(displayMsec);
	Q_EMIT _pMessageButton->showMessage();
}

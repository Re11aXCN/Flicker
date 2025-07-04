#include "FKFormPannel.h"

#include <QTimer> 
#include <QPainter>
#include <QPainterPath>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRandomGenerator>

#include <NXTheme.h>
#include <NXIcon.h>
#include "FKUtils.h"
#include "FKConstant.h"
#include "FKLauncherShell.h"
#include "Components/FKPushButton.h"
#include "Components/FKLineEdit.h"
#include "Components/FKIconLabel.h"
#include "Source/FKHttpManager.h"


static QRegularExpression s_email_pattern_regex(R"(^[a-zA-Z0-9](?:[a-zA-Z0-9._%+-]*[a-zA-Z0-9])?@(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}$)");
static QRegularExpression s_username_pattern_regex("^[a-zA-Z0-9_]+$");

FKFormPannel::FKFormPannel(QWidget* parent /*= nullptr*/)
	: QWidget(parent)
	, _pVerifyCodeTimer(new QTimer(this))
{
	_initUI();

	_pErrorStyleSheet = _pVerifyCodeLineEdit->styleSheet().removeLast() + "border: 1px solid red;}";
	_pNormalStyleSheet = _pVerifyCodeLineEdit->styleSheet();
	_pVerifyCodeTimer->setInterval(1000); // 1秒间隔
	QObject::connect(_pVerifyCodeTimer, &QTimer::timeout, this, &FKFormPannel::_updateVerifyCodeTimer);
	QObject::connect(FKHttpManager::getInstance().get(), &FKHttpManager::httpRequestFinished, this, &FKFormPannel::_handleServerResponse);
	QObject::connect(_pConfirmButton, &NXPushButton::clicked, this, &FKFormPannel::_onComfirmButtonClicked);
	QObject::connect(_pVerifyCodeLineEdit, &FKLineEdit::buttonClicked, this, &FKFormPannel::_onGetVerifyCodeButtonClicked);
	
	QObject::connect(_pUsernameLineEdit, &QLineEdit::textChanged, this, &FKFormPannel::_onUsernameTextChanged);
	QObject::connect(_pEmailLineEdit, &QLineEdit::textChanged, this, &FKFormPannel::_onEmailTextChanged);
	QObject::connect(_pPasswordLineEdit, &QLineEdit::textChanged, this, &FKFormPannel::_onPasswordTextChanged);
	QObject::connect(_pConfirmPasswordLineEdit, &QLineEdit::textChanged, this, &FKFormPannel::_onConfirmPasswordTextChanged);
	QObject::connect(_pVerifyCodeLineEdit, &QLineEdit::textChanged, this, &FKFormPannel::_onVerifyCodeTextChanged);
	
	QObject::connect(_pUsernameLineEdit, &QLineEdit::editingFinished, this, &FKFormPannel::_onUsernameEditingFinished);
	QObject::connect(_pEmailLineEdit, &QLineEdit::editingFinished, this, &FKFormPannel::_onEmailEditingFinished);
	QObject::connect(_pPasswordLineEdit, &QLineEdit::editingFinished, this, &FKFormPannel::_onPasswordEditingFinished);
	QObject::connect(_pConfirmPasswordLineEdit, &QLineEdit::editingFinished, this, &FKFormPannel::_onConfirmPasswordEditingFinished);
	QObject::connect(_pVerifyCodeLineEdit, &QLineEdit::editingFinished, this, &FKFormPannel::_onVerifyCodeEditingFinished);
	
	QObject::connect(_pShowPasswordAction, &QAction::triggered, this, &FKFormPannel::_onShowPasswordActionTriggered);
	QObject::connect(_pShowConfirmPasswordAction, &QAction::triggered, this, &FKFormPannel::_onShowConfirmPasswordActionTriggered);
	
	QObject::connect(_pSwitchSigninOrResetText, &NXText::clicked, this, &FKFormPannel::_onSwitchSigninOrResetTextClicked);
}

FKFormPannel::~FKFormPannel()
{
}

void FKFormPannel::toggleFormType()
{
	_pIsSwitchStackedWidget = false;
	// 重置验证状态
	_pValidationFlags = Launcher::InputValidationFlag::None;
	
	// 清空所有输入框（可能当前输入LineEdit有信息，切换后要清空）
	_pUsernameLineEdit->clear();
	_pPasswordLineEdit->clear();
	_pEmailLineEdit->clear();
	_pConfirmPasswordLineEdit->clear();
	_pVerifyCodeLineEdit->clear();
	
	// 重置样式（可能当前输入LineEdit内容错误，切换后要重置样式）
	_pUsernameLineEdit->setStyleSheet(_pNormalStyleSheet);
	_pPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
	_pEmailLineEdit->setStyleSheet(_pNormalStyleSheet);
	_pConfirmPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
	_pVerifyCodeLineEdit->setStyleSheet(_pNormalStyleSheet);

	// 重置密码显示状态
	_pPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pShowPasswordAction->setIcon(_pShowPasswordIcon);
	_pShowPasswordAction->setText("显示密码");
	_pConfirmPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pShowConfirmPasswordAction->setIcon(_pShowPasswordIcon);
	_pShowConfirmPasswordAction->setText("显示密码");

	//< TODO:
	//< 最好能够有一个cache，记录邮箱和时间，一分钟计时
	//< 如果用户再次输入缓存的邮箱，就更新按钮为剩余时间，而不是enabled(true)
	//< 防止用户切换Switch进行恶意刷新时间
	// 重置验证码倒计时
	if (_pVerifyCodeTimer->isActive()) {
		_pVerifyCodeTimer->stop();
	}
	_pVerifyCodeLineEdit->setButtonEnabled(false);
	_pVerifyCodeLineEdit->setButtonText("获取验证码");

	QTimer::singleShot(200, this, [this]() { 
		_updateSwitchedUI();
		_updateConfirmButtonState(); 
	});
}

void FKFormPannel::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
	_drawEdgeShadow(painter, rect(), Constant::SWITCH_BOX_SHADOW_COLOR, Constant::SWITCH_BOX_SHADOW_WIDTH);

	QWidget::paintEvent(event);
}

void FKFormPannel::_initUI()
{
	_pShowPasswordIcon = NXIcon::getInstance()->getNXIcon(NXIconType::Eye, 32, 32, 32);
	_pHidePasswordIcon = NXIcon::getInstance()->getNXIcon(NXIconType::EyeSlash, 32, 32, 32);

	_pLoginRegisterWidget = new QWidget(this);
	_pResetPasswordWidget = new QWidget(this);
	_pCentralStackedWidget = new NXCentralStackedWidget(this);
	_pCentralStackedWidget->setIsTransparent(true);
	_pCentralStackedWidget->setIsHasRadius(false);
	_pCentralStackedWidget->insertWidget(0, _pLoginRegisterWidget);
	_pCentralStackedWidget->insertWidget(1, _pResetPasswordWidget);
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->addWidget(_pCentralStackedWidget/*, 0, Qt::AlignCenter*/);

	// Login
	_pTitleText = new NXText(_pLoginRegisterWidget);
	_pQQIconLabel = new FKIconLabel(QString::fromUcs4(U"\ue882", 1), _pLoginRegisterWidget);
	_pWechatIconLabel = new FKIconLabel(QString::fromUcs4(U"\U000F0106", 1), _pLoginRegisterWidget);
	_pBiliBiliIconLabel = new FKIconLabel(QString::fromUcs4(U"\ue66d", 1), _pLoginRegisterWidget);
	_pDescriptionText = new NXText(_pLoginRegisterWidget);
	_pUsernameLineEdit = new NXLineEdit(_pLoginRegisterWidget);
	_pPasswordLineEdit = new NXLineEdit(_pLoginRegisterWidget);
	_pConfirmButton = new FKPushButton("SIGN IN", _pLoginRegisterWidget);
	_pSwitchSigninOrResetText = new NXText(_pLoginRegisterWidget);
	_pShowPasswordAction = new QAction(_pShowPasswordIcon, "显示密码", _pPasswordLineEdit);

	_pTitleText->setText("登 录 账 号");
	_pTitleText->setFont(QFont{"华文宋体"});
	_pTitleText->setStyleSheet(FKUtils::concat("color: ", FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR), ";"));
	_pTitleText->setTextStyle(NXTextType::CustomStyle, 32, QFont::Weight::Black);
	_pQQIconLabel->setFixedSize(32, 32);
	_pWechatIconLabel->setFixedSize(32, 32);
	_pBiliBiliIconLabel->setFixedSize(32, 32);

	_pDescriptionText->setText("选择登录方式或输入用户名/邮箱登录");
	_pDescriptionText->setFixedSize(300, 14);
	_pDescriptionText->setAlignment(Qt::AlignCenter);
	_pDescriptionText->setStyleSheet(FKUtils::concat("color: ", FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR), ";"));
	_pDescriptionText->setTextStyle(NXTextType::CustomStyle, 13, QFont::Weight::Light);

	_pUsernameLineEdit->setPlaceholderText("用户名/邮箱");
	_pUsernameLineEdit->setIsClearButtonEnabled(false);
	_pUsernameLineEdit->setContentsPaddings(20, 0, 5, 0);
	_pUsernameLineEdit->setFixedSize(300, 36);
	
	_pPasswordLineEdit->setPlaceholderText("密码");
	_pPasswordLineEdit->setIsClearButtonEnabled(false);
	_pPasswordLineEdit->setContentsPaddings(20, 0, 5, 0);
	_pPasswordLineEdit->setFixedSize(300, 36);
	_pPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pPasswordLineEdit->addAction(_pShowPasswordAction, QLineEdit::ActionPosition::TrailingPosition);

	_pSwitchSigninOrResetText->setText("忘记密码?");
	_pSwitchSigninOrResetText->setFixedSize(70, 17);
	_pSwitchSigninOrResetText->setIsAllowClick(true);
	_pSwitchSigninOrResetText->setAlignment(Qt::AlignCenter);
	_pSwitchSigninOrResetText->setStyleSheet(FKUtils::concat("color: ", FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR), ";"));
	_pSwitchSigninOrResetText->setBorderStyle(1, NXWidgetType::BottomBorder, Constant::DESCRIPTION_TEXT_COLOR);
	_pSwitchSigninOrResetText->setTextStyle(NXTextType::CustomStyle, 14, QFont::Weight::Light);
	_pConfirmButton->setEnabled(false);

	// Register
	_pEmailLineEdit = new NXLineEdit(_pLoginRegisterWidget);
	_pConfirmPasswordLineEdit = new NXLineEdit(_pLoginRegisterWidget);
	_pVerifyCodeLineEdit = new FKLineEdit({ .ButtonText{ "获取验证码" }, .ButtonWidth{ 74 }, .ButtonEnabled{ false } }, _pLoginRegisterWidget);
	_pShowConfirmPasswordAction = new QAction(_pShowPasswordIcon, "显示密码", _pConfirmPasswordLineEdit);

	_pEmailLineEdit->setPlaceholderText("邮箱");
	_pEmailLineEdit->setIsClearButtonEnabled(true);
	_pEmailLineEdit->setContentsPaddings(20, 0, 5, 0);
	_pEmailLineEdit->setFixedSize(300, 36);
	_pEmailLineEdit->hide();

	_pConfirmPasswordLineEdit->setPlaceholderText("确认密码");
	_pConfirmPasswordLineEdit->setIsClearButtonEnabled(true);
	_pConfirmPasswordLineEdit->setContentsPaddings(20, 0, 5, 0);
	_pConfirmPasswordLineEdit->setFixedSize(300, 36);
	_pConfirmPasswordLineEdit->setEchoMode(QLineEdit::Password);
	_pConfirmPasswordLineEdit->addAction(_pShowConfirmPasswordAction, QLineEdit::ActionPosition::TrailingPosition);
	_pConfirmPasswordLineEdit->hide();

	_pVerifyCodeLineEdit->setPlaceholderText("验证码");
	_pVerifyCodeLineEdit->setIsClearButtonEnabled(true);
	_pVerifyCodeLineEdit->setContentsPaddings(20, 0, 5, 0);
	_pVerifyCodeLineEdit->setFixedSize(300, 36);
	_pVerifyCodeLineEdit->setLineEditIconMargin(122); // 74 + 20 * 2 + 8
	_pVerifyCodeLineEdit->setMaxLength(6);
	_pVerifyCodeLineEdit->hide();

	_pIconLabelLayout = new QHBoxLayout();
	_pIconLabelLayout->setContentsMargins(0, 0, 0, 0);
	_pIconLabelLayout->setAlignment(Qt::AlignCenter);
	_pIconLabelLayout->addStretch();
	_pIconLabelLayout->addWidget(_pQQIconLabel, 0, Qt::AlignCenter);
	_pIconLabelLayout->addSpacing(10);
	_pIconLabelLayout->addWidget(_pWechatIconLabel, 0, Qt::AlignCenter);
	_pIconLabelLayout->addSpacing(10);
	_pIconLabelLayout->addWidget(_pBiliBiliIconLabel, 0, Qt::AlignCenter);
	_pIconLabelLayout->addStretch();

	// 提示：addSpacing和addStretch都会让 layout->count() 增加1，下标从0开始
	_pUniqueLayout = new QVBoxLayout(_pLoginRegisterWidget);
	_pUniqueLayout->setContentsMargins(0, 0, 0, 0);
	_pUniqueLayout->setAlignment(Qt::AlignCenter);
	_pUniqueLayout->setSpacing(0);
	_pUniqueLayout->addSpacing(100);
	_pUniqueLayout->addWidget(_pTitleText, 0, Qt::AlignCenter);
	_pUniqueLayout->addSpacing(24);
	_pUniqueLayout->addLayout(_pIconLabelLayout, 0);
	_pUniqueLayout->addSpacing(24);
	_pUniqueLayout->addWidget(_pDescriptionText, 0, Qt::AlignCenter);
	_pUniqueLayout->addSpacing(15);
	_pUniqueLayout->addWidget(_pUsernameLineEdit, 0, Qt::AlignCenter); // 7
	_pUniqueLayout->addSpacing(5);
	_pUniqueLayout->addWidget(_pPasswordLineEdit, 0, Qt::AlignCenter); // 9
	_pUniqueLayout->addSpacing(30);
	_pUniqueLayout->addWidget(_pSwitchSigninOrResetText, 0, Qt::AlignCenter);
	_pUniqueLayout->addSpacing(50);
	_pUniqueLayout->addWidget(_pConfirmButton, 0, Qt::AlignCenter);
	_pUniqueLayout->addStretch();
}

void FKFormPannel::_updateSwitchedUI()
{
	if (_pCentralStackedWidget->currentIndex() == 0) {
		if (_pIsSwitchStackedWidget) {
			_pFormType = Launcher::FormType::Authentication;

			_pUniqueLayout->takeAt(9); // _pPasswordLineEdit
			_pUniqueLayout->takeAt(7); // _pUsernameLineEdit
		}
		else {
			_pFormType = (_pFormType == Launcher::FormType::Login)
				? Launcher::FormType::Register
				: Launcher::FormType::Login;

			while (_pUniqueLayout->count())
			{
				QLayoutItem* item = _pUniqueLayout->takeAt(0);
			}
		}
	}
	else {
		if (_pIsSwitchStackedWidget) {
			_pFormType = Launcher::FormType::Login;
		}
		else {
			_pFormType = (_pFormType == Launcher::FormType::Authentication)
				? Launcher::FormType::ResetPassword
				: Launcher::FormType::Authentication;
		}

		_pUniqueLayout->takeAt(9); // _pVerifyCodeLineEdit/_pConfirmPasswordLineEdit
		_pUniqueLayout->takeAt(7); // _pEmailLineEdit/_pPasswordLineEdit
	}

	switch (_pFormType) {
	case Launcher::FormType::Login: {// 其他状态都可以切换到Login
		_pTitleText->setText("登 录 账 号");
		_pDescriptionText->setText("选择登录方式或输入用户名/邮箱登录");
		_pUsernameLineEdit->setPlaceholderText("用户名/邮箱");
		_pUsernameLineEdit->setIsClearButtonEnabled(false);
		_pPasswordLineEdit->setPlaceholderText("密码");
		_pPasswordLineEdit->setIsClearButtonEnabled(false);
		_pSwitchSigninOrResetText->setText("忘记密码?");
		_pConfirmButton->setText("SIGN IN");
		_pConfirmButton->setEnabled(false); // 默认禁用，等待验证通过

		if (_pIsSwitchStackedWidget) {
			_pUniqueLayout->insertWidget(7, _pUsernameLineEdit, 0, Qt::AlignCenter);
			_pUniqueLayout->insertWidget(9, _pPasswordLineEdit, 0, Qt::AlignCenter);
			_pLoginRegisterWidget->setLayout(_pUniqueLayout);
		}
		else {
			_pUniqueLayout->addSpacing(100);
			_pUniqueLayout->addWidget(_pTitleText, 0, Qt::AlignCenter);
			_pUniqueLayout->addSpacing(24);
			_pUniqueLayout->addLayout(_pIconLabelLayout, 0);
			_pUniqueLayout->addSpacing(24);
			_pUniqueLayout->addWidget(_pDescriptionText, 0, Qt::AlignCenter);
			_pUniqueLayout->addSpacing(15);
			_pUniqueLayout->addWidget(_pUsernameLineEdit, 0, Qt::AlignCenter);
			_pUniqueLayout->addSpacing(5);
			_pUniqueLayout->addWidget(_pPasswordLineEdit, 0, Qt::AlignCenter);
			_pUniqueLayout->addSpacing(30);
			_pUniqueLayout->addWidget(_pSwitchSigninOrResetText, 0, Qt::AlignCenter);
			_pUniqueLayout->addSpacing(50);
			_pUniqueLayout->addWidget(_pConfirmButton, 0, Qt::AlignCenter);
			_pUniqueLayout->addStretch();
		}

		// 显示登录控件，隐藏注册控件
		_pUsernameLineEdit->show();
		_pPasswordLineEdit->show();
		_pSwitchSigninOrResetText->show();
		//<_pDescriptionText->show();
		//<_pQQIconLabel->show();
		//<_pWechatIconLabel->show();
		//<_pBiliBiliIconLabel->show();

		_pEmailLineEdit->hide();
		_pConfirmPasswordLineEdit->hide();
		_pVerifyCodeLineEdit->hide();
		break;
	}
	case Launcher::FormType::Register: {// 只能由Login切换到Register
		_pTitleText->setText("创 建 账 号");
		_pDescriptionText->setText("选择注册方式或电子邮箱注册");
		_pUsernameLineEdit->setPlaceholderText("用户名");
		_pUsernameLineEdit->setIsClearButtonEnabled(true);
		_pPasswordLineEdit->setPlaceholderText("输入密码");
		_pPasswordLineEdit->setIsClearButtonEnabled(true);
		_pConfirmButton->setText("SIGN UP");
		_pConfirmButton->setEnabled(false);

		//<_pUniqueLayout->addSpacing(100);
		//<_pUniqueLayout->addWidget(_pTitleText, 0, Qt::AlignCenter);
		//<_pUniqueLayout->addSpacing(35);
		_pUniqueLayout->addSpacing(80);
		_pUniqueLayout->addWidget(_pTitleText, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(24);
		_pUniqueLayout->addLayout(_pIconLabelLayout, 0);
		_pUniqueLayout->addSpacing(24);
		_pUniqueLayout->addWidget(_pDescriptionText, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(15);

		_pUniqueLayout->addWidget(_pUsernameLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(5);
		_pUniqueLayout->addWidget(_pEmailLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(5);
		_pUniqueLayout->addWidget(_pPasswordLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(5);
		_pUniqueLayout->addWidget(_pConfirmPasswordLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(5);
		_pUniqueLayout->addWidget(_pVerifyCodeLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->addSpacing(40);
		_pUniqueLayout->addWidget(_pConfirmButton, 0, Qt::AlignCenter);
		_pUniqueLayout->addStretch();

		_pEmailLineEdit->show();
		_pConfirmPasswordLineEdit->show();
		_pVerifyCodeLineEdit->show();

		_pSwitchSigninOrResetText->hide();
		//<_pDescriptionText->hide();
		//<_pQQIconLabel->hide();
		//<_pWechatIconLabel->hide();
		//<_pBiliBiliIconLabel->hide();
		break;
	}
	case Launcher::FormType::Authentication: {// 由Login/Reset可以切换到Auth
		_pTitleText->setText("验 证 身 份");
		_pDescriptionText->setText("选择验证方式或输入邮箱验证");
		_pSwitchSigninOrResetText->setText("返回登录");
		_pConfirmButton->setText("VERIFY");
		_pConfirmButton->setEnabled(false);

		_pUniqueLayout->insertWidget(7, _pEmailLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->insertWidget(9, _pVerifyCodeLineEdit, 0, Qt::AlignCenter);

		_pResetPasswordWidget->setLayout(_pUniqueLayout);

		_pEmailLineEdit->show();
		_pVerifyCodeLineEdit->show();

		_pUsernameLineEdit->hide();
		_pPasswordLineEdit->hide();
		_pConfirmPasswordLineEdit->hide();
		break;
	}
	case Launcher::FormType::ResetPassword: {// 只能由Auth切换到Reset
		_pTitleText->setText("重 置 密 码");
		_pDescriptionText->setText("请输入新的密码并确认");
		_pPasswordLineEdit->setPlaceholderText("输入密码");
		_pPasswordLineEdit->setIsClearButtonEnabled(true);
		_pConfirmButton->setText("RESET");
		_pConfirmButton->setEnabled(false); 

		_pUniqueLayout->insertWidget(7, _pPasswordLineEdit, 0, Qt::AlignCenter);
		_pUniqueLayout->insertWidget(9, _pConfirmPasswordLineEdit, 0, Qt::AlignCenter);

		_pPasswordLineEdit->show();
		_pConfirmPasswordLineEdit->show();

		_pEmailLineEdit->hide();
		_pVerifyCodeLineEdit->hide();
		break;
	}
	default: throw std::invalid_argument("Invalid form type");
	}
}

void FKFormPannel::_drawEdgeShadow(QPainter& painter, const QRect& rect, const QColor& color, int shadowWidth) const noexcept
{
#define DrawEdgeShadow(Left, Right)\
	QLinearGradient gradient(rect.top##Left(), contentRect.top##Left());\
	gradient.setColorAt(0, color);\
	gradient.setColorAt(1, Qt::transparent);\
	painter.fillRect(QRect{ rect.top##Left(), contentRect.bottom##Right() }, gradient)

	if (_pFormType == Launcher::FormType::Login || _pFormType == Launcher::FormType::Authentication) {
		QRect contentRect = rect.adjusted(shadowWidth, 0, 0, 0);
		DrawEdgeShadow(Left, Right);
	}
	else {
		QRect contentRect = rect.adjusted(0, 0, -shadowWidth, 0);
		DrawEdgeShadow(Right, Left);
	}
#undef DrawEdgeShadow
}


void FKFormPannel::_initRegistryCallback()
{
	_pResponseCallbacks.insert(Http::RequestId::ID_GET_VARIFY_CODE, [this](const QJsonObject& jsonObj) {
		qDebug() << "get varify code response is " << jsonObj;
		if (jsonObj["status_code"].toInt() != static_cast<int>(Http::RequestStatusCode::SUCCESS)) {
			FKLauncherShell::ShowMessage("ERROR", jsonObj["message"].toString(), NXMessageBarType::Error, NXMessageBarType::TopRight);
			return;
		}
		QJsonObject data = jsonObj["data"].toObject();
		FKLauncherShell::ShowMessage("SUCCESS", jsonObj["message"].toString(), NXMessageBarType::Success, NXMessageBarType::BottomRight);
		qDebug() << "email is " << data["email"].toString();
		});
	_pResponseCallbacks.insert(Http::RequestId::ID_REGISTER_USER, [this](const QJsonObject& jsonObj) {
		qDebug() << "register user response is " << jsonObj;
		if (jsonObj["status_code"].toInt() != static_cast<int>(Http::RequestStatusCode::SUCCESS)) {
			FKLauncherShell::ShowMessage("ERROR", jsonObj["message"].toString(), NXMessageBarType::Error, NXMessageBarType::TopRight);
			return;
		}
		QJsonObject data = jsonObj["data"].toObject();
		FKLauncherShell::ShowMessage("SUCCESS", jsonObj["message"].toString(), NXMessageBarType::Success, NXMessageBarType::BottomRight);
		qDebug() << "email is " << data["email"].toString();
		});
	_pResponseCallbacks.insert(Http::RequestId::ID_LOGIN_USER, [this](const QJsonObject& jsonObj) {
		qDebug() << "login user response is " << jsonObj;
		if (jsonObj["status_code"].toInt() != static_cast<int>(Http::RequestStatusCode::SUCCESS)) {
			FKLauncherShell::ShowMessage("ERROR", jsonObj["message"].toString(), NXMessageBarType::Error, NXMessageBarType::TopLeft);
			return;
		}
		QJsonObject data = jsonObj["data"].toObject();
		FKLauncherShell::ShowMessage("SUCCESS", "登录成功！", NXMessageBarType::Success, NXMessageBarType::BottomLeft);

		// TODO: 登录成功后的处理，如保存用户信息、跳转到主界面等
		qDebug() << "User logged in: " << data["username"].toString();
		});
	_pResponseCallbacks.insert(Http::RequestId::ID_RESET_PASSWORD, [this](const QJsonObject& jsonObj) {
		qDebug() << "reset password response is " << jsonObj;
		if (jsonObj["status_code"].toInt() != static_cast<int>(Http::RequestStatusCode::SUCCESS)) {
			FKLauncherShell::ShowMessage("ERROR", jsonObj["message"].toString(), NXMessageBarType::Error, NXMessageBarType::TopLeft);
			return;
		}
		QJsonObject data = jsonObj["data"].toObject();
		FKLauncherShell::ShowMessage("SUCCESS", "修改密码成功！", NXMessageBarType::Success, NXMessageBarType::BottomLeft);

		});
}

void FKFormPannel::_handleServerResponse(const QString& response, Http::RequestId requestId, Http::RequestSeviceType serviceType, Http::RequestStatusCode statusCode)
{
	if (statusCode != Http::RequestStatusCode::SUCCESS) {
		FKLauncherShell::ShowMessage("ERROR", "NETWORK REQUEST ERROR!", NXMessageBarType::Error, NXMessageBarType::TopRight);
		return;
	}
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response.toUtf8());

	if (jsonDoc.isNull() || !jsonDoc.isObject()) {
		FKLauncherShell::ShowMessage("ERROR", "JSON PARSE ERROR!", NXMessageBarType::Error, NXMessageBarType::TopRight);
		return;
	}

	switch (serviceType) {
	case Http::RequestSeviceType::GET_VARIFY_CODE:
		_pResponseCallbacks[requestId](jsonDoc.object());
	case Http::RequestSeviceType::REGISTER_USER:
		_pResponseCallbacks[requestId](jsonDoc.object());
	default:
		break;
	}
}

bool FKFormPannel::_validateUsername(const QString& username)
{
	if (username.isEmpty()) {
		return false;
	}
	return s_username_pattern_regex.match(username.trimmed()).hasMatch();
}

bool FKFormPannel::_validateEmail(const QString& email)
{
	if (email.isEmpty()) {
		return false;
	}
	return s_email_pattern_regex.match(email.trimmed()).hasMatch();
}

bool FKFormPannel::_validateUsernameOrEmail(const QString& usernameOrEmail)
{
	return _validateUsername(usernameOrEmail) || _validateEmail(usernameOrEmail);
}

bool FKFormPannel::_validatePassword(const QString& password)
{
	if (password.isEmpty()) {
		return false;
	}
	
	if (password.length() < 8 || password.length() > 20 || password.contains(' ')) {
		return false;
	}
	
	return true;
}

bool FKFormPannel::_validateConfirmPassword(const QString& password, const QString& confirmPassword)
{
	if (confirmPassword.isEmpty()) {
		return false;
	}
	
	return password == confirmPassword;
}

bool FKFormPannel::_validateVerifyCode(const QString& verifyCode)
{
	if (verifyCode.isEmpty() || verifyCode.length() != 6) {
		return false;
	}
	
	return true;
}

void FKFormPannel::_updateConfirmButtonState()
{
	bool isValid = false;
	// 所有字段都有效时启用按钮
	switch (_pFormType)
	{
	case Launcher::FormType::Login: {
		isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllLoginValid) == Launcher::InputValidationFlag::AllLoginValid;
		break;
	}
	case Launcher::FormType::Register: {
		isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllRegisterValid) == Launcher::InputValidationFlag::AllRegisterValid;
		break;
	}
	case Launcher::FormType::Authentication: {
		isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllAuthenticationValid) == Launcher::InputValidationFlag::AllAuthenticationValid;
		break;
	}
	case Launcher::FormType::ResetPassword: {
		isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllResetPasswordValid) == Launcher::InputValidationFlag::AllResetPasswordValid;
		break;
	}
	default: throw std::invalid_argument("Invalid form type");
	}
	_pConfirmButton->setEnabled(isValid);
}

void FKFormPannel::_onUsernameTextChanged(const QString& text)
{
	bool isValid = (_pFormType == Launcher::FormType::Login) 
		? _validateUsernameOrEmail(text) 
		: _validateUsername(text);
	
	if (isValid) {
		_pValidationFlags |= Launcher::InputValidationFlag::UsernameValid;
		_pUsernameLineEdit->setStyleSheet(_pNormalStyleSheet);
	} else {
		_pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::UsernameValid };
	}
	
	_updateConfirmButtonState();
}

void FKFormPannel::_onEmailTextChanged(const QString& text)
{
	bool isValid = _validateEmail(text);
	
	if (isValid) {
		_pValidationFlags |= Launcher::InputValidationFlag::EmailValid;
		_pEmailLineEdit->setStyleSheet(_pNormalStyleSheet);
		_pVerifyCodeLineEdit->setButtonEnabled(true);
	} else {
		_pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::EmailValid };
	}
	
	_updateConfirmButtonState();
}

void FKFormPannel::_onPasswordTextChanged(const QString& text)
{
	bool isValid = _validatePassword(text);
	
	if (isValid) {
		_pValidationFlags |= Launcher::InputValidationFlag::PasswordValid;
		_pPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
	} else {
		_pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::PasswordValid };
	}
	
	// 如果确认密码已经输入，则同时验证确认密码
	if (!_pConfirmPasswordLineEdit->text().isEmpty()) {
		_onConfirmPasswordTextChanged(_pConfirmPasswordLineEdit->text());
	}
	
	_updateConfirmButtonState();
}

void FKFormPannel::_onConfirmPasswordTextChanged(const QString& text)
{
	bool isValid = _validateConfirmPassword(_pPasswordLineEdit->text(), text);
	
	if (isValid) {
		_pValidationFlags |= Launcher::InputValidationFlag::ConfirmPasswordValid;
		_pConfirmPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
	} else {
		_pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::ConfirmPasswordValid };
	}
	
	_updateConfirmButtonState();
}

void FKFormPannel::_onVerifyCodeTextChanged(const QString& text)
{
	bool isValid = _validateVerifyCode(text);
	
	if (isValid) {
		_pValidationFlags |= Launcher::InputValidationFlag::VerifyCodeValid;
		_pVerifyCodeLineEdit->setStyleSheet(_pNormalStyleSheet);
	} else {
		_pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::VerifyCodeValid };
	}
	
	_updateConfirmButtonState();
}

void FKFormPannel::_onUsernameEditingFinished()
{
	if (!(_pValidationFlags & Launcher::InputValidationFlag::UsernameValid)) {
		_pUsernameLineEdit->setStyleSheet(_pErrorStyleSheet);
		if (!_pUsernameLineEdit->text().isEmpty()) {
			// 登录/注册
			FKLauncherShell::ShowMessage("ERROR", "用户名只能包含字母、数字和下划线！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::TopLeft : NXMessageBarType::TopRight);
		}
	}
}

void FKFormPannel::_onEmailEditingFinished()
{
	if (!(_pValidationFlags & Launcher::InputValidationFlag::EmailValid)) {
		_pEmailLineEdit->setStyleSheet(_pErrorStyleSheet);
		if (!_pEmailLineEdit->text().isEmpty()) {
			// 验证/注册
			FKLauncherShell::ShowMessage("ERROR", "邮箱格式错误！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Authentication ? NXMessageBarType::TopLeft : NXMessageBarType::TopRight);
		}
	}
}

void FKFormPannel::_onPasswordEditingFinished()
{
	if (!(_pValidationFlags & Launcher::InputValidationFlag::PasswordValid)) {
		_pPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
		if (!_pPasswordLineEdit->text().isEmpty()) {
			// 登录/（注册/重置密码）
			FKLauncherShell::ShowMessage("ERROR", "密码长度需在8-20位之间且不能包含空格！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::TopLeft : NXMessageBarType::TopRight);
		}
	}
}

void FKFormPannel::_onConfirmPasswordEditingFinished()
{
	if (!(_pValidationFlags & Launcher::InputValidationFlag::ConfirmPasswordValid)) {
		_pConfirmPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
		if (!_pConfirmPasswordLineEdit->text().isEmpty()) {
			// 注册/重置密码
			FKLauncherShell::ShowMessage("ERROR", "两次输入的密码不一致！", NXMessageBarType::Error, NXMessageBarType::TopRight);
		}
	}
}

void FKFormPannel::_onVerifyCodeEditingFinished()
{
	if (!(_pValidationFlags & Launcher::InputValidationFlag::VerifyCodeValid)) {
		_pVerifyCodeLineEdit->setStyleSheet(_pErrorStyleSheet);
		if (!_pVerifyCodeLineEdit->text().isEmpty()) {
			// 验证/注册
			FKLauncherShell::ShowMessage("ERROR", "验证码格式错误！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Authentication ? NXMessageBarType::TopLeft : NXMessageBarType::TopRight);
		}
	}
}

void FKFormPannel::_onShowPasswordActionTriggered()
{
	bool passwordVisible = _pPasswordLineEdit->echoMode() == QLineEdit::Password;
	_pPasswordLineEdit->setEchoMode(passwordVisible ? QLineEdit::Normal : QLineEdit::Password);
	_pShowPasswordAction->setIcon(passwordVisible ? _pHidePasswordIcon : _pShowPasswordIcon);
	_pShowPasswordAction->setText(passwordVisible ? "隐藏密码" : "显示密码");
}

void FKFormPannel::_onShowConfirmPasswordActionTriggered()
{
	bool confirmPasswordVisible = _pConfirmPasswordLineEdit->echoMode() == QLineEdit::Password;
	_pConfirmPasswordLineEdit->setEchoMode(confirmPasswordVisible ? QLineEdit::Normal : QLineEdit::Password);
	_pShowConfirmPasswordAction->setIcon(confirmPasswordVisible ? _pHidePasswordIcon : _pShowPasswordIcon);
	_pShowConfirmPasswordAction->setText(confirmPasswordVisible ? "隐藏密码" : "显示密码");
}

void FKFormPannel::_updateVerifyCodeTimer()
{
	_pRemainingSeconds--;

	if (_pRemainingSeconds > 0) {
		_pVerifyCodeLineEdit->setButtonText(QString("%1s").arg(_pRemainingSeconds));
	}
	else {
		// 倒计时结束
		_pVerifyCodeTimer->stop();
		_pVerifyCodeLineEdit->setButtonEnabled(true);
		_pVerifyCodeLineEdit->setButtonText("获取验证码");
	}
}

void FKFormPannel::_onGetVerifyCodeButtonClicked()
{
	QString email = _pEmailLineEdit->text().trimmed();
	// 验证码按钮只有在邮箱有效时才能点击，这里再次验证
	if (!_validateEmail(email)) {
		FKLauncherShell::ShowMessage("ERROR", "邮箱格式错误！", NXMessageBarType::Error, NXMessageBarType::TopRight);
		_pVerifyCodeLineEdit->setButtonEnabled(false);
		return;
	}

	// 定时器设置，防止用户多次点击
	_pRemainingSeconds = 60;
	_pVerifyCodeLineEdit->setButtonEnabled(false);
	_pVerifyCodeLineEdit->setButtonText(QString("%1s").arg(_pRemainingSeconds));
	_pVerifyCodeTimer->start();

	// 创建 JSON 请求对象
	QJsonObject requestObj;
	requestObj["request_type"] = static_cast<int>(Http::RequestSeviceType::GET_VARIFY_CODE);
	requestObj["message"] = "Client request for varify code";

	// 创建数据对象
	QJsonObject dataObj;
	dataObj["email"] = email;
	requestObj["data"] = dataObj;
	qDebug() << "request is " << requestObj;

	FKHttpManager::getInstance()->postHttpRequest("http://localhost:8080/get_varify_code",
		requestObj,
		Http::RequestId::ID_GET_VARIFY_CODE,
		Http::RequestSeviceType::GET_VARIFY_CODE);
}

void FKFormPannel::_onComfirmButtonClicked()
{
	// 由于我们已经实现了实时验证，按钮只有在所有字段都有效时才能点击
	// 这里只需要处理提交逻辑，不需要重复验证
	switch (_pFormType) {
	case Launcher::FormType::Login: {
		QString usernameOrEmail = _pUsernameLineEdit->text().trimmed();
		QString password = _pPasswordLineEdit->text();

		QJsonObject requestObj;
		requestObj["request_type"] = static_cast<int>(Http::RequestSeviceType::LOGIN_USER);
		requestObj["message"] = "Client request for login";

		QJsonObject dataObj;

		// 判断输入是邮箱还是用户名
		if (s_email_pattern_regex.match(usernameOrEmail).hasMatch()) {
			dataObj["email"] = usernameOrEmail;
			dataObj["username"] = "";
		}
		else {
			dataObj["username"] = usernameOrEmail;
			dataObj["email"] = "";
		}

		// 对密码进行MD5哈希处理
		QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex();
		dataObj["password"] = QString(passwordHash);

		// 记住密码和自动登录选项
		//dataObj["remember_password"] = _pLoginRememberPasswordCheckBox->isChecked();
		//dataObj["auto_login"] = _pLoginAutoLoginCheckBox->isChecked();

		requestObj["data"] = dataObj;

		FKHttpManager::getInstance()->postHttpRequest("http://localhost:8080/login_user",
			requestObj,
			Http::RequestId::ID_LOGIN_USER,
			Http::RequestSeviceType::LOGIN_USER);
		break;
	}
	case Launcher::FormType::Register: {
		QString username = _pUsernameLineEdit->text().trimmed();
		QString email = _pEmailLineEdit->text().trimmed();
		QString password = _pPasswordLineEdit->text();
		QString varifyCode = _pVerifyCodeLineEdit->text().trimmed();

		QJsonObject requestObj;
		requestObj["request_type"] = static_cast<int>(Http::RequestSeviceType::REGISTER_USER);
		requestObj["message"] = "Client request for register user";

		QJsonObject dataObj;
		dataObj["username"] = username;
		dataObj["email"] = email;
		dataObj["password"] = password;
		dataObj["verify_code"] = varifyCode;
		requestObj["data"] = dataObj;

		FKHttpManager::getInstance()->postHttpRequest("http://localhost:8080/register_user",
			requestObj,
			Http::RequestId::ID_REGISTER_USER,
			Http::RequestSeviceType::REGISTER_USER);
		break;
	}
	case Launcher::FormType::Authentication: {
		//TODO
		// Authentication succeeded身份验证成功才允许跳转到ResetPassword界面进行密码重置
		break;
	}
	case Launcher::FormType::ResetPassword: {
		//TODO
		
		break;
	}
	default: throw std::invalid_argument("Invalid form type");
	}
}

void FKFormPannel::_onSwitchSigninOrResetTextClicked()
{
	_pIsSwitchStackedWidget = true;
	if (_pCentralStackedWidget->currentIndex() == 0) {
		_updateSwitchedUI();
		_pCentralStackedWidget->doWindowStackSwitch(NXWindowType::Scale, 1, false);
	}
	else {
		_updateSwitchedUI();
		_pCentralStackedWidget->doWindowStackSwitch(NXWindowType::Scale, 0, true);
	}
}

#include "FKFormPannel.h"

#include <QTimer> 
#include <QPainter>
#include <QPainterPath>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QRandomGenerator>

#include <NXTheme.h>
#include <NXIcon.h>
#include "FKLogger.h"
#include "FKUtils.h"
#include "FKConstant.h"
#include "FKLauncherShell.h"
#include "Components/FKPushButton.h"
#include "Components/FKLineEdit.h"
#include "Components/FKIconLabel.h"
#include "Source/FKHttpManager.h"
#include "magic_enum/magic_enum.hpp"

static QRegularExpression s_email_pattern_regex(R"(^[a-zA-Z0-9](?:[a-zA-Z0-9._%+-]*[a-zA-Z0-9])?@(?:[a-zA-Z0-9-]+\.)+[a-zA-Z]{2,}$)");
static QRegularExpression s_username_pattern_regex("^[a-zA-Z0-9_]+$");

FKFormPannel::FKFormPannel(QWidget* parent /*= nullptr*/)
    : QWidget(parent)
    , _pVerifyCodeTimer(new QTimer(this))
{
    _initUI();
    _initRegistryCallback();

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
    // TODO: 
    // 登录和注册切换时，需要重置状态
    // 登录和重置密码——邮箱验证码 切换时，需要重置状态
    // 重置密码——邮箱验证码 到 重置密码——输入新的密码 切换不需要重置状态，因为需要发送验证码和邮箱给Gate
    // 重置密码——输入新的密码 到  重置密码——邮箱验证码，需要重置状态
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
    _pVerifyCodeLineEdit->setButtonText("获取验证码");
    _pVerifyCodeLineEdit->setButtonEnabled(false);

    QTimer::singleShot(200, this, [this]() { 
        _updateSwitchedUI();
        _updateConfirmButtonState(); 
    });
}

void FKFormPannel::clickSwitchSigninOrResetText()
{
    Q_EMIT _pSwitchSigninOrResetText->clicked();
}

void FKFormPannel::paintEvent(QPaintEvent* event)
{
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
    _pTitleText->setStyleSheet(FKUtils::qconcat("color: ", FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR), ";"));
    _pTitleText->setTextStyle(NXTextType::CustomStyle, 32, QFont::Weight::Black);
    _pQQIconLabel->setFixedSize(32, 32);
    _pWechatIconLabel->setFixedSize(32, 32);
    _pBiliBiliIconLabel->setFixedSize(32, 32);

    _pDescriptionText->setText("选择登录方式或输入用户名/邮箱登录");
    _pDescriptionText->setFixedSize(300, 14);
    _pDescriptionText->setAlignment(Qt::AlignCenter);
    _pDescriptionText->setStyleSheet(FKUtils::qconcat("color: ", FKUtils::colorToCssString(Constant::DESCRIPTION_TEXT_COLOR), ";"));
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
    _pSwitchSigninOrResetText->setStyleSheet(FKUtils::qconcat("color: ", FKUtils::colorToCssString(Constant::DARK_TEXT_COLOR), ";"));
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
    _pConfirmPasswordLineEdit->setToolTip("8~20位，可由英文、数字、特殊字符组成，不允许空格");
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

void FKFormPannel::_initRegistryCallback()
{
#define SHOW_HTTP_RESPONSE_MESSAGE(SuccessPos, ErrorPos) \
if (responseJsonObj["response_status_code"].toInt() != static_cast<int>(flicker::http::status::ok)) {\
    FKLauncherShell::ShowMessage(FKUtils::qconcat("ERROR, HTTP CODE: " + responseJsonObj["response_status_code"].toString()), responseJsonObj["message"].toString(), NXMessageBarType::Error, NXMessageBarType::##ErrorPos);\
    return;\
}\
FKLauncherShell::ShowMessage(FKUtils::qconcat("SUCCESS, HTTP CODE: " + responseJsonObj["response_status_code"].toString()), responseJsonObj["message"].toString(), NXMessageBarType::Success, NXMessageBarType::##SuccessPos)


    _pResponseCallbacks.insert(flicker::http::service::VerifyCode, [this](const QJsonObject& responseJsonObj) {
        if (_pFormType == Launcher::FormType::Register) {
            auto status = static_cast<flicker::http::status>(responseJsonObj["response_status_code"].toInt());
            auto message = responseJsonObj["message"].toString();
            if (status != flicker::http::status::ok) {
                FKLauncherShell::ShowMessage(FKUtils::qconcat("ERROR, HTTP CODE: " + responseJsonObj["response_status_code"].toString()), message, NXMessageBarType::Error, NXMessageBarType::BottomRight);
                if (status == flicker::http::status::conflict && message.contains("exist")) {
                    if (_pVerifyCodeTimer->isActive()) {
                        _pVerifyCodeTimer->stop();
                    }
                    _pVerifyCodeLineEdit->setButtonText("获取验证码");
                    _pVerifyCodeLineEdit->setButtonEnabled(true);
                }
                return;
            }
            FKLauncherShell::ShowMessage(FKUtils::qconcat("SUCCESS, HTTP CODE: " + responseJsonObj["response_status_code"].toString()), responseJsonObj["message"].toString(), NXMessageBarType::Success, NXMessageBarType::TopRight);
        }
        else {
            SHOW_HTTP_RESPONSE_MESSAGE(TopLeft, BottomLeft);
        }
        QJsonObject data = responseJsonObj["data"].toObject();
        if (data["verify_type"].toInt() == static_cast<int>(flicker::http::service::ResetPassword)) {
            _pConfirmButton->setEnabled(true);
        }
        });

    _pResponseCallbacks.insert(flicker::http::service::Login, [this](const QJsonObject& responseJsonObj) {
        SHOW_HTTP_RESPONSE_MESSAGE(TopLeft, BottomLeft);
        });

    _pResponseCallbacks.insert(flicker::http::service::Register, [this](const QJsonObject& responseJsonObj) {
        SHOW_HTTP_RESPONSE_MESSAGE(TopRight, BottomRight);
        });

    _pResponseCallbacks.insert(flicker::http::service::ResetPassword, [this](const QJsonObject& responseJsonObj) {
        SHOW_HTTP_RESPONSE_MESSAGE(TopRight, BottomRight);

        });
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
        _pPasswordLineEdit->setToolTip("");
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
        _pPasswordLineEdit->setToolTip("8~20位，可由英文、数字、特殊字符组成，不允许空格");
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


void FKFormPannel::_handleServerResponse(flicker::http::service serviceType, const QJsonObject& responseJsonObj)
{
    auto callback = _pResponseCallbacks.value(serviceType);

    if (!callback) {
        qWarning() << "回调函数无效:" << magic_enum::enum_name(serviceType);
        return;
    }

    try {
        callback(responseJsonObj);
    }
    catch (const std::exception& e) {
        qCritical() << "回调执行异常:" << e.what();
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
    // 检查是否只包含英文、数字、特殊字符，不含空格
    for (const QChar& ch : password) {
        char c = ch.toLatin1();
        if (c < 33 || c > 126) {
            return false;
        }
    }
    
    return true;
}

bool FKFormPannel::_validatePasswordLength(const QString& password)
{
    return !password.isEmpty() && password.length() >= 8 && password.length() <= 20;
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
    for (const QChar& ch : verifyCode) {
        if (!std::isalnum(ch.toLatin1())) {
            return false;
        }
    }
    return true;
}

void FKFormPannel::_updateConfirmButtonState()
{
    // 所有字段都有效时启用按钮
    switch (_pFormType)
    {
    case Launcher::FormType::Login: {
        _pConfirmButton->setEnabled((_pValidationFlags & Launcher::InputValidationFlag::AllLoginValid) == Launcher::InputValidationFlag::AllLoginValid);
        break;
    }
    case Launcher::FormType::Register: {
        _pConfirmButton->setEnabled((_pValidationFlags & Launcher::InputValidationFlag::AllRegisterValid) == Launcher::InputValidationFlag::AllRegisterValid);
        break;
    }
    case Launcher::FormType::Authentication: {
        //isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllAuthenticationValid) == Launcher::InputValidationFlag::AllAuthenticationValid;
        break;
    }
    case Launcher::FormType::ResetPassword: {
        //isValid = (_pValidationFlags & Launcher::InputValidationFlag::AllResetPasswordValid) == Launcher::InputValidationFlag::AllResetPasswordValid;
        break;
    }
    default: throw std::invalid_argument("Invalid form type");
    }
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
    // 在文本变化时只检查密码格式（仅英文、数字、特殊字符，不含空格）
    bool isValid = _validatePassword(text);

    if (isValid) {
        _pPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
    }
    else {
        _pPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
        FKLauncherShell::ShowMessage("ERROR", "密码只能包含英文、数字、特殊字符，不允许空格！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
        _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::PasswordValid };
    }
    
    _updateConfirmButtonState();
}

void FKFormPannel::_onConfirmPasswordTextChanged(const QString& text)
{
    bool isValid = _validatePassword(text);

    if (isValid) {
        _pConfirmPasswordLineEdit->setStyleSheet(_pNormalStyleSheet);
    }
    else {
        _pConfirmPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
        FKLauncherShell::ShowMessage("ERROR", "密码只能包含英文、数字、特殊字符，不允许空格！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
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
            FKLauncherShell::ShowMessage("ERROR", "用户名只能包含字母、数字和下划线！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
        }
    }
}

void FKFormPannel::_onEmailEditingFinished()
{
    if (!(_pValidationFlags & Launcher::InputValidationFlag::EmailValid)) {
        _pEmailLineEdit->setStyleSheet(_pErrorStyleSheet);
        if (!_pEmailLineEdit->text().isEmpty()) {
            // 验证/注册
            FKLauncherShell::ShowMessage("ERROR", "邮箱格式错误！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Authentication ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
            _pVerifyCodeLineEdit->setButtonEnabled(false);
        }
    }
}

void FKFormPannel::_onPasswordEditingFinished()
{
    // 检查确认密码长度是否符合8-20位要求
    QString passwordText = _pPasswordLineEdit->text();
    bool isLengthValid = _validatePasswordLength(passwordText);
    if (_pFormType == Launcher::FormType::Login) {
        if (isLengthValid) {
            _pValidationFlags |= Launcher::InputValidationFlag::PasswordValid;
        }
        else {
            _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::PasswordValid };
            _pPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
            FKLauncherShell::ShowMessage("ERROR", "密码长度必须为8~20位！", NXMessageBarType::Error, NXMessageBarType::BottomLeft);
        }
    }
    else {
        if (isLengthValid) {
            QString comfirmPasswordText = _pConfirmPasswordLineEdit->text();
            // 如果确认密码也已输入且格式正确，检查两个密码是否匹配
            if (!comfirmPasswordText.isEmpty() &&
                _validatePassword(comfirmPasswordText) &&
                _validatePasswordLength(comfirmPasswordText))
            {
                bool isMatch = _validateConfirmPassword(passwordText, comfirmPasswordText);
                if (isMatch) {
                    _pValidationFlags |= Launcher::InputValidationFlag::PasswordValid;
                    _pValidationFlags |= Launcher::InputValidationFlag::ConfirmPasswordValid;
                }
                else {
                    _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::PasswordValid };
                    _pPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
                    FKLauncherShell::ShowMessage("ERROR", "两次输入的密码不一致！", NXMessageBarType::Error, NXMessageBarType::BottomRight);
                }
            }
        }
        else {
            _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::PasswordValid };
            _pPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
            if (!passwordText.isEmpty()) {
                FKLauncherShell::ShowMessage("ERROR", "密码长度必须为8~20位！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Login ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
            }
        }
    }
    
    _updateConfirmButtonState();
}

void FKFormPannel::_onConfirmPasswordEditingFinished()
{
    QString comfirmPasswordText = _pConfirmPasswordLineEdit->text();
    bool isLengthValid = _validatePasswordLength(comfirmPasswordText);
    if (isLengthValid) {
        QString passwordText = _pPasswordLineEdit->text();
        if (!passwordText.isEmpty() &&
            _validatePassword(passwordText) &&
            _validatePasswordLength(passwordText)) 
        {
            bool isMatch = _validateConfirmPassword(passwordText, comfirmPasswordText);
            if (isMatch) {
                _pValidationFlags |= Launcher::InputValidationFlag::PasswordValid;
                _pValidationFlags |= Launcher::InputValidationFlag::ConfirmPasswordValid;
            } 
            else {
                _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::ConfirmPasswordValid };
                _pConfirmPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
                FKLauncherShell::ShowMessage("ERROR", "两次输入的密码不一致！", NXMessageBarType::Error, NXMessageBarType::BottomRight);
            }
        }
    } 
    else {
        _pValidationFlags &= ~Launcher::InputValidationFlags{ Launcher::InputValidationFlag::ConfirmPasswordValid };
        _pConfirmPasswordLineEdit->setStyleSheet(_pErrorStyleSheet);
        if (!_pConfirmPasswordLineEdit->text().isEmpty()) {
            FKLauncherShell::ShowMessage("ERROR", "密码长度必须为8~20位！", NXMessageBarType::Error, NXMessageBarType::BottomRight);
        }
    }
    
    _updateConfirmButtonState();
}

void FKFormPannel::_onVerifyCodeEditingFinished()
{
    if (!(_pValidationFlags & Launcher::InputValidationFlag::VerifyCodeValid)) {
        _pVerifyCodeLineEdit->setStyleSheet(_pErrorStyleSheet);
        if (!_pVerifyCodeLineEdit->text().isEmpty()) {
            // 验证/注册
            FKLauncherShell::ShowMessage("ERROR", "验证码格式错误！", NXMessageBarType::Error, _pFormType == Launcher::FormType::Authentication ? NXMessageBarType::BottomLeft : NXMessageBarType::BottomRight);
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
        _pVerifyCodeLineEdit->setButtonText("获取验证码");
        _pVerifyCodeLineEdit->setButtonEnabled(true);
    }
}

void FKFormPannel::_onGetVerifyCodeButtonClicked()
{
    // 定时器设置，防止用户多次点击
    _pRemainingSeconds = 60;
    _pVerifyCodeLineEdit->setButtonText(QString("%1s").arg(_pRemainingSeconds));
    _pVerifyCodeLineEdit->setButtonEnabled(false);
    _pVerifyCodeTimer->start();

    QJsonObject requestObj, dataObj;
    dataObj["email"] = _pEmailLineEdit->text().trimmed();
    dataObj["verify_type"] = _pFormType == Launcher::FormType::Register
        ? static_cast<int>(flicker::http::service::Register)
        : static_cast<int>(flicker::http::service::ResetPassword);
    requestObj["request_service_type"] = static_cast<int>(flicker::http::service::VerifyCode);
    requestObj["data"] = dataObj;

    FKHttpManager::getInstance()->sendHttpRequest(flicker::http::service::VerifyCode,
        "http://localhost:8080/get_verify_code",
        requestObj);
}

void FKFormPannel::_onComfirmButtonClicked()
{
    // 由于我们已经实现了实时验证，按钮只有在所有字段都有效时才能点击
    // 这里只需要处理提交逻辑，不需要重复验证
    QJsonObject requestObj, dataObj;
    switch (_pFormType) {
    case Launcher::FormType::Login: {
        dataObj["username"] = _pUsernameLineEdit->text().trimmed();
        dataObj["hashed_password"] = QString(QCryptographicHash::hash(_pPasswordLineEdit->text().toUtf8(), QCryptographicHash::Sha256).toHex());
        requestObj["request_service_type"] = static_cast<int>(flicker::http::service::Login);
        requestObj["data"] = dataObj;

        FKHttpManager::getInstance()->sendHttpRequest(flicker::http::service::Login,
            "http://localhost:8080/login_user",
            requestObj);
        break;
    }
    case Launcher::FormType::Register: {
        dataObj["username"] = _pUsernameLineEdit->text().trimmed();
        dataObj["email"] = _pEmailLineEdit->text().trimmed();
        dataObj["hashed_password"] = QString(QCryptographicHash::hash(_pPasswordLineEdit->text().toUtf8(), QCryptographicHash::Sha256).toHex());
        dataObj["verify_code"] = _pVerifyCodeLineEdit->text().trimmed();
        requestObj["request_service_type"] = static_cast<int>(flicker::http::service::Register);
        requestObj["data"] = dataObj;

        FKHttpManager::getInstance()->sendHttpRequest(flicker::http::service::Register, 
            "http://localhost:8080/register_user",
            requestObj);
        break;
    }
    case Launcher::FormType::Authentication: {
        dataObj["email"] = _pEmailLineEdit->text().trimmed();
        dataObj["verify_code"] = _pVerifyCodeLineEdit->text().trimmed();
        requestObj["request_service_type"] = static_cast<int>(flicker::http::service::AuthenticateUser);
        requestObj["data"] = dataObj;

        FKHttpManager::getInstance()->sendHttpRequest(flicker::http::service::AuthenticateUser,
            "http://localhost:8080/authenticate_user",
            requestObj);
        Q_EMIT this->switchClicked();
        break;
    }
    case Launcher::FormType::ResetPassword: {
        dataObj["email"] = _pEmailLineEdit->text().trimmed();
        dataObj["hashed_password"] = QString(QCryptographicHash::hash(_pPasswordLineEdit->text().toUtf8(), QCryptographicHash::Sha256).toHex());
        requestObj["request_service_type"] = static_cast<int>(flicker::http::service::ResetPassword);
        requestObj["data"] = dataObj;

        FKHttpManager::getInstance()->sendHttpRequest(flicker::http::service::ResetPassword,
            "http://localhost:8080/reset_password",
            requestObj);

        Q_EMIT this->switchClicked();
        _onSwitchSigninOrResetTextClicked();
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

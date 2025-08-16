/*************************************************************************************
 *
 * @ Filename     : FKFormPannel.h
 * @ Description : 
 * 
 * @ Version     : V1.0
 * @ Author         : Re11a
 * @ Date Created: 2025/6/28
 * ======================================
 * HISTORICAL UPDATE HISTORY
 * Version: V          Modify Time:         Modified By: 
 * Modifications: 
 * ======================================
*************************************************************************************/
#ifndef FK_FORM_PANNEL_H_
#define FK_FORM_PANNEL_H_

#include <NXLineEdit.h>
#include <NXText.h>
#include <NXCentralStackedWidget.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include "FKDef.h"
class FKPushButton;
class FKLineEdit;
class FKIconLabel;
class FKFormPannel : public QWidget
{
    Q_OBJECT
public:
    explicit FKFormPannel(QWidget* parent = nullptr);
    ~FKFormPannel() override;
    void toggleFormType();
    Q_SLOT void clickSwitchSigninOrResetText();
Q_SIGNALS:
    void switchClicked();
protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void _initUI();
    void _initRegistryCallback();
    
    void _updateSwitchedUI(); // 更新UI显示
    void _updateConfirmButtonState(); // 更新按钮状态

    // 验证方法
    bool _validateUsername(const QString& username);
    bool _validateEmail(const QString& email);
    bool _validateUsernameOrEmail(const QString& usernameOrEmail);
    bool _validatePassword(const QString& password);
    bool _validatePasswordLength(const QString& password);
    bool _validateConfirmPassword(const QString& password, const QString& confirmPassword);
    bool _validateVerifyCode(const QString& verifyCode);

private Q_SLOTS:
    void _handleServerResponse(Flicker::Client::Enums::ServiceType serviceType, const QJsonObject& responseJsonObj);
    void _onGetVerifyCodeButtonClicked();
    void _onComfirmButtonClicked();
    void _onSwitchSigninOrResetTextClicked();

    // 输入验证槽
    void _onUsernameTextChanged(const QString& text);
    void _onEmailTextChanged(const QString& text);
    void _onPasswordTextChanged(const QString& text);
    void _onConfirmPasswordTextChanged(const QString& text);
    void _onVerifyCodeTextChanged(const QString& text);
    
    // 输入框焦点变化槽
    void _onUsernameEditingFinished();
    void _onEmailEditingFinished();
    void _onPasswordEditingFinished();
    void _onConfirmPasswordEditingFinished();
    void _onVerifyCodeEditingFinished();

    void _onShowPasswordActionTriggered();
    void _onShowConfirmPasswordActionTriggered();

    void _updateVerifyCodeTimer();

    // TCP连接相关方法;
    void _connectToChatServer(const QString& host, int port, const QString& loginToken, const QString& clientDeviceId);
    void _handleTcpConnectionError(const QString& error);
    void _handleAuthenticationResult(bool success, const QString& message);
private:
    bool _pIsSwitchStackedWidget{ false };
    int _pVerifyRemainingSeconds{ 0 };

    Flicker::Client::Enums::FormType _pFormType
    { Flicker::Client::Enums::FormType::Login };
    // 验证状态标志
    Flicker::Client::Enums::InputValidationFlags _pValidationFlags
    { Flicker::Client::Enums::InputValidationFlag::None };

    QString _pErrorStyleSheet{};
    QString _pNormalStyleSheet{};
    QIcon _pShowPasswordIcon;
    QIcon _pHidePasswordIcon;
    QHash<Flicker::Client::Enums::ServiceType, std::function<void(const QJsonObject&)>> _pResponseCallbacks;

    QWidget* _pLoginRegisterWidget{ nullptr };
    QWidget* _pResetPasswordWidget{ nullptr };
    QVBoxLayout* _pUniqueLayout{ nullptr };
    // 登录/注册界面控件
    NXLineEdit* _pEmailLineEdit{ nullptr };
    NXLineEdit* _pConfirmPasswordLineEdit{ nullptr };
    FKLineEdit* _pVerifyCodeLineEdit{ nullptr };
    NXText* _pDescriptionText{ nullptr };
    NXText* _pSwitchSigninOrResetText{ nullptr };
    QAction* _pShowConfirmPasswordAction{ nullptr };
    FKIconLabel* _pQQIconLabel{ nullptr };
    FKIconLabel* _pWechatIconLabel{ nullptr };
    FKIconLabel* _pBiliBiliIconLabel{ nullptr };
    NXCentralStackedWidget* _pCentralStackedWidget{ nullptr };

    // 公共控件
    NXText* _pTitleText{ nullptr };
    NXLineEdit* _pUsernameLineEdit{ nullptr };
    NXLineEdit* _pPasswordLineEdit{ nullptr };
    FKPushButton* _pConfirmButton{ nullptr };
    QAction* _pShowPasswordAction{ nullptr };

    QHBoxLayout* _pIconLabelLayout{ nullptr };
    QTimer* _pVerifyCodeTimer;  // 验证码倒计时定时器
};

#endif //!FK_FORM_PANNEL_H_


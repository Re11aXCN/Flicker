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

    // private slots
    Q_SLOT void _handleServerResponse(flicker::http::service serviceType, const QJsonObject& responseJsonObj);
    Q_SLOT void _onGetVerifyCodeButtonClicked();
    Q_SLOT void _onComfirmButtonClicked();
    Q_SLOT void _onSwitchSigninOrResetTextClicked();

    // 输入验证槽
    Q_SLOT void _onUsernameTextChanged(const QString& text);
    Q_SLOT void _onEmailTextChanged(const QString& text);
    Q_SLOT void _onPasswordTextChanged(const QString& text);
    Q_SLOT void _onConfirmPasswordTextChanged(const QString& text);
    Q_SLOT void _onVerifyCodeTextChanged(const QString& text);
    
    // 输入框焦点变化槽
    Q_SLOT void _onUsernameEditingFinished();
    Q_SLOT void _onEmailEditingFinished();
    Q_SLOT void _onPasswordEditingFinished();
    Q_SLOT void _onConfirmPasswordEditingFinished();
    Q_SLOT void _onVerifyCodeEditingFinished();

    Q_SLOT void _onShowPasswordActionTriggered();
    Q_SLOT void _onShowConfirmPasswordActionTriggered();

    Q_SLOT void _updateVerifyCodeTimer();

    bool _pIsSwitchStackedWidget{ false };
    int _pRemainingSeconds{ 0 };

    Launcher::FormType _pFormType{ Launcher::FormType::Login };
    // 验证状态标志
    Launcher::InputValidationFlags _pValidationFlags{ Launcher::InputValidationFlag::None };
    QString _pErrorStyleSheet{};
    QString _pNormalStyleSheet{};
    QIcon _pShowPasswordIcon;
    QIcon _pHidePasswordIcon;
    QHash<flicker::http::service, std::function<void(const QJsonObject&)>> _pResponseCallbacks;

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


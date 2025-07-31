#ifndef FK_LAUNCHERSHELL_H_
#define FK_LAUNCHERSHELL_H_

#include <NXWidget.h>
#include <NXMessageButton.h>
#include <NXShadowWidget.h>
class QPropertyAnimation;
class QParallelAnimationGroup;
class FKFormPannel;
class FKSwitchPannel;
class FKLauncherShell : public NXWidget
{
    Q_OBJECT
        Q_PROPERTY_CREATE_BASIC_H(qreal, CircleX)
public:
    explicit FKLauncherShell(QWidget* parent = nullptr);
    virtual ~FKLauncherShell();
    static void ShowMessage(const QString& title, const QString& text, NXMessageBarType::MessageMode mode, NXMessageBarType::PositionPolicy position, int displayMsec = 2000);
protected:
    virtual void paintEvent(QPaintEvent* event) override;
private:
    void _initUi();
    void _initAnimation();

    void _drawEdgeShadow(QPainter& painter, const QRect& rect,
        const QColor& color, int shadowWidth) const noexcept;

    // private slots
    Q_SLOT void _onSwitchPannelButtonClicked();

    // 面板
    FKFormPannel* _pFormPannel{ nullptr };
    FKSwitchPannel* _pSwitchPannel{ nullptr };
    NXShadowWidget* _pShadowWidget{ nullptr };
    // 动画
    QParallelAnimationGroup* _pAnimationGroup{ nullptr };
    QPropertyAnimation* _pSwitchPannelAnimation{ nullptr };
    QPropertyAnimation* _pFormPannelAnimation{ nullptr };
    QPropertyAnimation* _pShadowWidgetAnimation{ nullptr };
    QPropertyAnimation* _pShadowWidgetWidthAnimation{ nullptr };
    QPropertyAnimation* _pSwitchPannelWidthAnimation{ nullptr };
    QPropertyAnimation* _pSwitchCircleXAnimation{ nullptr };

    inline static NXMessageButton* _pMessageButton;
};

#endif // !FK_LAUNCHERSHELL_H_

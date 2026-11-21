#pragma once

#include <QHideEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QShowEvent>
#include <QVariantAnimation>

// Keep the link text steady while the border and background slowly brighten.
// Paint the pulse directly: no stylesheet/layout rebuild on every frame.
class HyperFusionBanner final : public QLabel {
public:
    explicit HyperFusionBanner(QWidget *parent = nullptr) : QLabel(parent), pulse(this) {
        setTextFormat(Qt::RichText);
        setAlignment(Qt::AlignCenter);
        setWordWrap(true);
        setOpenExternalLinks(true);
        setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
        setFocusPolicy(Qt::StrongFocus);
        setContentsMargins(22, 14, 22, 14);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        setMinimumHeight(94);
        setStyleSheet("QLabel { background:transparent; border:0; padding:0; color:#eefaff; }");
        const QString url = QStringLiteral("https://github.com/XetalEngine/x_hyperland");
        setToolTip(url);
        setAccessibleName(tr("Discover Hyper Fusion on GitHub"));
        setAccessibleDescription(tr("A complete Hyprland desktop by Xetal. Opens the x_hyperland repository in your browser."));
        setText(QStringLiteral(
            "<a href='%1' style='color:#eefaff; text-decoration:none;'>"
            "<span style='font-size:22px; font-weight:bold; color:#69e5ff;'>%2</span>"
            "&nbsp;&nbsp;<span style='font-size:12px; color:#b5cced;'>%3</span><br>"
            "<span style='font-size:14px;'>%4</span><br>"
            "<span style='font-size:14px; font-weight:bold; color:#8bffcb; text-decoration:underline;'>%5</span>"
            "</a>")
            .arg(url, tr("MEET HYPER FUSION").toHtmlEscaped(), tr("BY XETAL").toHtmlEscaped(),
                 tr("Your complete Hyprland desktop: Fusion Files, themes, a 3D desktop cube & more.").toHtmlEscaped(),
                 tr("Explore x_hyperland on GitHub →").toHtmlEscaped()));

        pulse.setDuration(2800);
        pulse.setStartValue(0.0);
        pulse.setKeyValueAt(0.5, 1.0);
        pulse.setEndValue(0.0);
        pulse.setEasingCurve(QEasingCurve::InOutSine);
        pulse.setLoopCount(-1);
        connect(&pulse, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
            glow = value.toReal();
            update();
        });
    }

protected:
    void showEvent(QShowEvent *event) override {
        QLabel::showEvent(event);
        pulse.start();
    }
    void hideEvent(QHideEvent *event) override {
        pulse.stop(); // No animation work while a different tab is open.
        QLabel::hideEvent(event);
    }
    void paintEvent(QPaintEvent *event) override {
        {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            const QRectF frame = QRectF(rect()).adjusted(2, 2, -2, -2);
            QLinearGradient background(frame.topLeft(), frame.bottomRight());
            background.setColorAt(0, QColor("#132b50"));
            background.setColorAt(0.5, QColor(17, 45 + qRound(14 * glow), 63 + qRound(22 * glow)));
            background.setColorAt(1, QColor("#102e32"));
            QLinearGradient edge(frame.topLeft(), frame.topRight());
            edge.setColorAt(0, QColor(79, 153, 255, 155 + qRound(100 * glow)));
            edge.setColorAt(0.5, QColor(76, 220, 255, 155 + qRound(100 * glow)));
            edge.setColorAt(1, QColor(90, 255, 187, 155 + qRound(100 * glow)));
            painter.setBrush(background);
            painter.setPen(QPen(QBrush(edge), 2));
            painter.drawRoundedRect(frame, 12, 12);
            if (hasFocus()) {
                painter.setBrush(Qt::NoBrush);
                painter.setPen(QPen(QColor("#eefaff"), 1, Qt::DashLine));
                painter.drawRoundedRect(frame.adjusted(4, 4, -4, -4), 8, 8);
            }
        }
        QLabel::paintEvent(event);
    }

private:
    QVariantAnimation pulse;
    qreal glow = 0;
};

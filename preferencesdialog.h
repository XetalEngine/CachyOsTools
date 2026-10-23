#pragma once

#include <QDialog>
#include <QMap>
#include <QVariantMap>
#include <QVector>
#include <functional>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSettings;
class QStackedWidget;
class QVBoxLayout;

namespace AppPreferences {
struct Tab { QString id, title, description; };
QList<Tab> tabs();
QVariantMap defaults();
QVariantMap read(QSettings &settings);
bool write(QSettings &settings, const QVariantMap &values);
}

// Editing stays inside this dialog until apply() succeeds. The owner performs
// persistence and updates the application; the dialog never starts a tool.
class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    using Save = std::function<bool(const QVariantMap &)>;
    PreferencesDialog(const QVariantMap &values, Save save, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Page { QWidget *widget; QVBoxLayout *layout; QString title, description; QStringList keys; };
    struct Card { QGroupBox *widget; int page; };
    QVariantMap m_saved, m_draft;
    Save m_save;
    bool m_loading = false;
    bool m_saveFailed = false;
    QVector<Page> m_sections;
    QVector<Card> m_cards;
    QMap<QString, QWidget *> m_controls;
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages;
    QLineEdit *m_search;
    QLabel *m_status;
    QLabel *m_tabCount;
    QWidget *m_themePreview;
    QPushButton *m_apply;
    QPushButton *m_done;
    QPushButton *m_cancel;
    QPushButton *m_reset;

    int addPage(const QString &title, const QString &description, const QString &icon);
    QGroupBox *addCard(int page, const QString &title, const QString &description = {});
    QCheckBox *addCheck(QGroupBox *card, const QString &key, const QString &title, const QString &description = {});
    QComboBox *addCombo(QGroupBox *card, const QString &key, const QString &title,
                       const QList<QPair<QString, QVariant>> &choices);
    void addSpin(QGroupBox *card, const QString &key, const QString &title, int minimum, int maximum);
    void registerControl(QGroupBox *card, const QString &key, QWidget *control);
    void buildGeneral();
    void buildAppearance();
    void buildWorkspace();
    void buildTools();
    void buildIso();
    void loadWidgets();
    void updateStartupChoices();
    void updateThemePreview();
    void styleDialog();
    void edited(bool stageOnly = false);
    void updateFooter();
    QString validationError() const;
    bool apply();
    void resetPage();
    void filter(const QString &query);
};

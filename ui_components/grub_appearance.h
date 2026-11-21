#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>

class QBoxLayout;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QProcess;
class QTabWidget;
class QSlider;
class QTimer;
class GrubDesignPreview;

class GrubAppearance final : public QWidget {
  Q_OBJECT
public:
  explicit GrubAppearance(const QString &helper, QWidget *parent = nullptr);
  ~GrubAppearance() override;
  QSize minimumSizeHint() const override { return QSize(760, 400); }
  bool hasHeightForWidth() const override { return false; }
  void setMenu(const QJsonArray &entries);
  void setAvailability(bool busy, bool supported, bool readable);
  void acceptResult(const QJsonObject &data);
  void reviewChanges(const QJsonObject &data);
signals:
  void requested(const QJsonObject &data);
protected:
  void showEvent(QShowEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
private:
  QString helper, revision, backgroundHash;
  QJsonObject settings;
  QJsonArray menu;
  QListWidget *themes;
  QComboBox *pictures, *position, *textSize;
  QLineEdit *heading, *subtitle;
  QSlider *dimming, *contrast;
  QCheckBox *hints;
  QPushButton *accent, *textColor, *base, *apply, *undo, *restore, *reload, *browse;
  QLabel *caption, *selection;
  QBoxLayout *body;
  QTabWidget *controls;
  QComboBox *themePicker;
  QWidget *themeShelf;
  QWidget *content;
  GrubDesignPreview *preview;
  QProcess *catalog, *renderer;
  QTimer *debounce;
  QJsonObject rendering;
  bool loaded = false, syncing = false, previewReady = false, busy = false;
  bool supported = false, readable = false, active = false, canUndo = false;
  int generation = 0, renderingGeneration = 0;
  void loadCatalog();
  void loadOptions(const QJsonObject &value);
  void changed();
  void renderPreview();
  void updateActions();
  void chooseColor(const QString &key, QPushButton *button);
  void colorButtons();
};

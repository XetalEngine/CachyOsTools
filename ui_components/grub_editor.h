#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QWidget>

class QTreeWidget;
class QBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QProcess;
class QProgressBar;
class QPushButton;
class QTabWidget;
class GrubAppearance;

class GrubEditor final : public QWidget {
  Q_OBJECT
public:
  explicit GrubEditor(const QString &helper, QWidget *parent = nullptr);
  ~GrubEditor() override;
  int heightForWidth(int width) const override;
protected:
  void showEvent(QShowEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
private:
  QString helper, revision, menuRevision, operation;
  QProcess *reader, *worker;
  QByteArray output, errors;
  QJsonObject pending;
  QJsonArray managedEntries;
  QBoxLayout *choicesLayout;
  QTabWidget *tabs;
  GrubAppearance *appearance;
  QTreeWidget *drives, *entries;
  QListWidget *systems;
  QLineEdit *name;
  QPushButton *refresh, *detect, *properties, *add, *load, *rename, *remove, *undo;
  QLabel *status, *mode, *selection, *saved, *notes, *driveCount, *entryDetails;
  QProgressBar *progress;
  bool busy = false, supported = false, loaded = false, canUndo = false;
  void refreshDrives();
  void request(const QJsonObject &data);
  void readWorker();
  void result(const QJsonObject &data);
  void setBusy(bool value);
  void updateActions();
  void chooseDrive();
  void scanDrive();
  void showDriveProperties(const QJsonObject &drive);
  void chooseSystem();
  void editEntry();
  void chooseEntry();
  QJsonObject drive() const;
  QJsonObject system() const;
  QJsonObject entry() const;
};

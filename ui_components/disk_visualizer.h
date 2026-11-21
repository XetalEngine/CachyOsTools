#pragma once
#include <QJsonObject>
#include <QWidget>
#include <functional>
#include <memory>

class DiskVisualizer final : public QWidget {
  Q_OBJECT
public:
  explicit DiskVisualizer(std::function<bool()> authenticate, QWidget *parent = nullptr);
  ~DiskVisualizer() override;
  // Uses exactly the normal scan and rendering path, without sudo, for fixtures.
  void scanFixture(const QString &path);
signals:
  void viewReady();
private:
  struct Impl;
  std::unique_ptr<Impl> d;
};

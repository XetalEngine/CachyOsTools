#pragma once
#include <QRectF>
#include <QVector>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace DiskLayout {
// Squarified area layout. Sorting is internal and results retain input order.
// Zero-size nodes stay in the tree; they deliberately occupy no map area.
inline QVector<QRectF> rectangles(const QVector<double> &weights, QRectF bounds) {
  QVector<QRectF> result(weights.size());
  QVector<int> order;
  double total = 0;
  for (int i = 0; i < weights.size(); ++i)
    if (weights[i] > 0 && std::isfinite(weights[i])) {
      order.append(i);
      total += weights[i];
    }
  if (total <= 0 || bounds.width() <= 0 || bounds.height() <= 0) return result;
  std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return weights[a] > weights[b]; });
  const auto scale = bounds.width() * bounds.height() / total;
  int cursor = 0;
  while (cursor < order.size() && bounds.width() > .00001 && bounds.height() > .00001) {
    const double side = qMin(bounds.width(), bounds.height());
    auto worst = [&](double sum, double low, double high) {
      return qMax(side * side * high / (sum * sum), sum * sum / (side * side * low));
    };
    int end = cursor + 1;
    double sum = weights[order[cursor]] * scale, high = sum, low = sum;
    double score = worst(sum, low, high);
    while (end < order.size()) {
      const double next = weights[order[end]] * scale;
      const double trial = worst(sum + next, next, high);
      if (trial > score) break;
      sum += next;
      low = next;
      score = trial;
      ++end;
    }
    Q_UNUSED(low);
    const bool vertical = bounds.width() >= bounds.height();
    const double thickness = sum / side;
    double offset = 0;
    for (int i = cursor; i < end; ++i) {
      const double length = weights[order[i]] * scale / thickness;
      result[order[i]] = vertical ? QRectF(bounds.x(), bounds.y() + offset, thickness, length)
                                 : QRectF(bounds.x() + offset, bounds.y(), length, thickness);
      offset += length;
    }
    if (vertical) bounds.setLeft(bounds.left() + thickness);
    else bounds.setTop(bounds.top() + thickness);
    cursor = end;
  }
  return result;
}
} // namespace DiskLayout

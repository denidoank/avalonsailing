#ifndef ANGLECONTROLLER_H
#define ANGLECONTROLLER_H

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QColor>

class AngleController : public QObject, public QGraphicsEllipseItem
{
  Q_OBJECT
public:
  AngleController(QPointF pos, float radius, QGraphicsItem *parent)
      : QGraphicsEllipseItem(QRectF(-radius + pos.x(), -radius + pos.y(), 2*radius, 2*radius), parent),
      pos_(pos), minAngle_(-180), maxAngle_(180) {
    QBrush controller_brush(QColor(255, 0, 0), Qt::SolidPattern);
    setBrush(controller_brush);
  }

  void setBounds(double minAngle, double maxAngle) { minAngle_ = minAngle; maxAngle_ = maxAngle; }

protected:
  // Overloading mousePressEvent is necessary for mouseMoveEvent to work.
  void mousePressEvent(QGraphicsSceneMouseEvent * event);

  void mouseMoveEvent(QGraphicsSceneMouseEvent * event);

signals:
  void turned(double angle);

private:
  QPointF pos_;
  double minAngle_, maxAngle_;
};

#endif // ANGLECONTROLLER_H

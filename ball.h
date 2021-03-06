#ifndef BALL_H
#define BALL_H
#include "gamescene.h"
#include <QPair>
#include <QPainterPath>
#include <QGraphicsEllipseItem>

class ball : public QGraphicsEllipseItem
{
    QColor m_color;
    QPointF m_speed;    // Vector
    bool m_moving;
    int m_try;
public:
    ball(int aX, int aY, int tries);
    enum { Type = GameScene::BallItem };
    int type() const { return Type; }
    static const qreal Radius;
    static const qreal LinSpeed;
    QPointF speed() { return m_speed; } //Getter
    void stop() { m_moving = false; }
    void resume() { m_moving = true; }
    void start(qreal aAngle, int level);
    void updatePos(qreal aTime);
    int getTries() { return m_try; }
    bool bounceWalls( qreal& aTime, QPointF& aImpact );
    bool bounceItems( qreal& aTime, QPointF& aImpactPoint, Brick*& aHitBrick);
};

#endif // BALL_H

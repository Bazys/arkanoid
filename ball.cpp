#include "ball.h"
#include "brick.h"
#include "gamescene.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <qmath.h>
#include <QPainter>
#include <QtDebug>
#include <QLine>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsBlurEffect>

const qreal ball::LinSpeed = 0.3; // скорость перемещения мяча pix/ms
const qreal ball::Radius = 9.5;

static int sgn(qreal aVal) // signed / unsigned
{
    if( aVal < 0 )
        return -1;
    else
        return 1;
}
// статическая функция вычисления пересечния окружности
static QPair<QPointF, QPointF> circleIntersection( QPointF aCenter, qreal aRadius, QLineF aLine )
{
    aLine.translate( -1 * aCenter );
    qreal dx = aLine.dx();
    qreal dy = aLine.dy();
    qreal len = aLine.length();
    qreal Det = aLine.x1() * aLine.y2() - aLine.x2() * aLine.y1();
    qreal Discr = qPow(aRadius, 2) * qPow(len, 2) - qPow(Det, 2);
    if( Discr < 0)
        return QPair<QPointF, QPointF>();
    QPointF p1;
    QPointF p2;
    if( Discr == 0)
        {
        p1.setX( Det * dy / qPow(len, 2));
        p1.setY( -1 * Det * dx / qPow(len, 2));
        p1 += aCenter;
        return qMakePair( p1, QPointF() );
        }
    else
        {
        p1.setX( (Det * dy + sgn(dy) * dx * qSqrt(Discr)) / qPow(len, 2) );
        p1.setY( (-1 * Det * dx + qAbs(dy) * qSqrt(Discr)) / qPow(len, 2));
        p1 += aCenter;
        p2.setX( (Det * dy - sgn(dy) * dx * qSqrt(Discr)) / qPow(len, 2) );
        p2.setY( (-1 * Det * dx - qAbs(dy) * qSqrt(Discr)) / qPow(len, 2));
        p2 += aCenter;
        return qMakePair( p1, p2 );
        }
}

ball::ball(int aX, int aY,int tries = 3)
    :QGraphicsEllipseItem( -Radius, -Radius, 2*Radius, 2*Radius ), m_color(Qt::gray), m_moving(false)
{
    m_try = tries;
    setPos( aX, aY);
    setFlag( ItemSendsGeometryChanges );
    QRadialGradient gradient( QPointF(0, 0), Radius, QPointF( Radius/2, -Radius/2) );
    gradient.setColorAt( 0, Qt::white );
    gradient.setColorAt( 1, m_color );
    setBrush( gradient );
}
void ball::start(qreal aAngle, int level = 1 )
{
    m_speed.setX( LinSpeed * level * cos(aAngle) );
    m_speed.setY( LinSpeed * level * sin(aAngle) );
    m_moving = true;
}

void ball::updatePos( qreal aTime )     // Passed time in ms
{
    if( m_moving )
        setPos( pos() + aTime * m_speed );
}

bool ball::bounceWalls(qreal &aTime, QPointF &aImpact)//преверка на удар об стены
{
    QPolygonF wallCorners( scene()->sceneRect() );
    QPolygonF ballCorners( mapToScene(rect()) );
    QLineF reverseSpeedLine( QPointF(0, 0), -1 * m_speed );  // меняем направление после удара

     for( int i=0; i<3; i++)
        {
        QLineF wall( wallCorners.at((i+3)%4), wallCorners.at((i+4)%4) );
        QLineF wallNormalLine = wall;
        wallNormalLine.setAngle( wall.angle() - 90 );
        qreal incidenceAngle = reverseSpeedLine.angleTo( wallNormalLine );
        if( incidenceAngle > 180 )
            incidenceAngle -= 360;
        if( qAbs( incidenceAngle ) >= 90 )   // неверное направление
            continue;
        QPointF ballTouchPoint = ballCorners[i];
        QLineF ballTrack = QLineF( ballTouchPoint - aTime*m_speed, ballTouchPoint );
        QPointF impactPoint;
        if( ballTrack.intersect(wall, &impactPoint) == QLineF::BoundedIntersection )
            {
            QLineF trackToImpact(ballTrack.p1(), impactPoint);
            qreal distToImpact = trackToImpact.length();
            qreal timeToImpact = distToImpact / ballTrack.length() * aTime;
            qreal timeAfterImpact = aTime - timeToImpact;
            reverseSpeedLine.setAngle( reverseSpeedLine.angle() + 2*incidenceAngle );
            m_speed = reverseSpeedLine.p2();
            QPointF impactBallPos = impactPoint - mapFromScene( ballTouchPoint );
            setPos( impactBallPos );
            aImpact = impactPoint;
            aTime = timeAfterImpact;
            return true;
            }
        }
    if( y() > scene()->sceneRect().bottom() ) { //если достигли нижней стены - остановка игры
        if (m_moving){
            m_try--;
            if (m_try>0) {
                setPos(0, GameScene::Height/2-65);
                m_moving = false;
//                start(6 * M_PI / 2 - qrand() * M_PI/2 / RAND_MAX);
            }
            else{
             stop();
            }
        }
    }
    return false;
}

bool ball::bounceItems(qreal &aTime, QPointF &aImpactPoint, Brick*& aHitBrick) // удар об предметы кирпичи или доску
{
    QList<QGraphicsItem*> items = scene()->items( mapToScene( shape() ) );
    QList<QGraphicsRectItem*> hitItems;
    foreach( QGraphicsItem* item, items )
    {
    QGraphicsRectItem* rectItem = dynamic_cast<QGraphicsRectItem*>( item );
    if( rectItem )
        hitItems << rectItem;
    }
    if( hitItems.isEmpty() )
        return false;
    QPolygonF ballCorners( mapToScene( rect() ) );
    QLineF reverseSpeedLine( QPointF(0, 0), -m_speed );  // направление удара - противоположенное
    const qreal inf = 10000;

    qreal nearestDistToImpact = inf;
    QPointF nearestImpactPoint;
    qreal nearestIncidenceAngle = inf;
    QPointF nearestBallPoint;
    QGraphicsRectItem* nearestItem = NULL;
    int nearestEdge = -1;

    foreach( QGraphicsRectItem* hitItem, hitItems )
        {
        QPolygonF itemCorners( hitItem->sceneBoundingRect() );
        // Проверка границ предмета
        for( int i=0; i<4; i++ )    // для каждой границы предмета и точки касания мяча
            {
            QLineF itemEdge( itemCorners.at(i), itemCorners.at((i+1)%4) ); // 0=T, 1=R, 2=B, 3=L
            QLineF itemNormalLine = itemEdge.normalVector();
            qreal incidenceAngle = reverseSpeedLine.angleTo( itemNormalLine );
            if( incidenceAngle > 180 )
                incidenceAngle -= 360;
            if( qAbs( incidenceAngle ) >= 90 )   // Упс... направление не верное
                continue;
            QPointF ballTouchPoint = QLineF( ballCorners[(i+2)%4], ballCorners[(i+3)%4] ).pointAt(0.5);   // центр границ прямоугольника
            QLineF ballTrack( ballTouchPoint - aTime * m_speed, ballTouchPoint );
            QPointF impactPoint;
            if( itemEdge.intersect(ballTrack, &impactPoint) == QLineF::BoundedIntersection )
                {
                QLineF trackToImpact(ballTrack.p1(), impactPoint);
                qreal distToImpact = trackToImpact.length();
                if( distToImpact < nearestDistToImpact )
                    {
                    nearestDistToImpact = distToImpact;
                    nearestImpactPoint = impactPoint;
                    nearestIncidenceAngle = incidenceAngle;
                    nearestBallPoint = ballTouchPoint;
                    nearestItem = hitItem;
                    nearestEdge = i;
                    }
                }
            }
        // Проверка уголов предметов (кирпичи/мяч/доска)
        for( int i=0; i<4; i++ )
            {
            // Для ракетки достаточно проверить только верхний левый и верхний правый углы
            if( hitItem->type() == GameScene::PaddleItem && i > 1 )
                break;
            QPointF itemCorner = itemCorners.at(i);
            QLineF centerToCorner( pos(), itemCorner );
            if( centerToCorner.length() > Radius ) // у мяча нет углов
                continue;
            QLineF itemEdge( itemCorners.at(i), itemCorners.at((i+1)%4) ); // 0=Top, 1=Right, 2=Bottom, 3=Left
            QLineF itemNormalLine = itemEdge;
            itemNormalLine.setAngle( itemNormalLine.angle() + 135 );
            qreal incidenceAngle = reverseSpeedLine.angleTo( itemNormalLine );
            if( incidenceAngle > 180 )
                incidenceAngle -= 360;
            if( qAbs( incidenceAngle ) >= 90 )   // Направление неверное
                continue;
            QLineF ballTrack( itemCorner - aTime * m_speed, itemCorner ); // С предыдущей позиции мяча  в угол
            QPair<QPointF, QPointF> intPoints = circleIntersection( pos() - aTime * m_speed, Radius, ballTrack );
            if( intPoints.first.isNull() || intPoints.second.isNull() )  // только точка пересечения
                continue;
            qreal distToImpact = inf;
            QPointF ballImpactPoint;
            QLineF trackToImpact;
            trackToImpact = QLineF( intPoints.first, itemCorner );
            if( trackToImpact.length() < distToImpact )
                {
                distToImpact = trackToImpact.length();
                ballImpactPoint = intPoints.first;
                }
            trackToImpact = QLineF( intPoints.second, itemCorner );
            if( trackToImpact.length() < distToImpact )
                {
                distToImpact = trackToImpact.length();
                ballImpactPoint = intPoints.second;
                }
            if( distToImpact < nearestDistToImpact )
                {
                nearestDistToImpact = distToImpact;
                nearestImpactPoint = itemCorner;
                nearestIncidenceAngle = incidenceAngle;
                nearestBallPoint = ballImpactPoint;
                nearestItem = hitItem;
                nearestEdge = -1;   // Нет границ
                }
            }
        }
    if( nearestDistToImpact == inf )   // Упс... удар в никуда
        return false;

    // изменить вектор скорости после удара
    if( nearestItem->type() == GameScene::PaddleItem && nearestEdge == 0 )
        {
        // Угол отражения зависит от точки удара об ракетку
        qreal paddleWidth = nearestItem->rect().width();
        qreal relImpactPos = ( paddleWidth/2 + nearestItem->mapFromScene(nearestImpactPoint).x() ) / paddleWidth;
        qreal angle = relImpactPos * M_PI/2 + M_PI/4;
        m_speed.setX( -LinSpeed * cos(angle) );
        m_speed.setY( -LinSpeed * sin(angle) );
        }
    else
        {
        // Угол падения равен углу отражения
        reverseSpeedLine.setAngle( reverseSpeedLine.angle() + 2*nearestIncidenceAngle );
        m_speed = reverseSpeedLine.p2();
        }

    // Установить мяч в позицию удара
    QPointF ballImpactPos = nearestImpactPoint - mapFromScene( nearestBallPoint );
    setPos( ballImpactPos );
    aTime -= LinSpeed / nearestDistToImpact;
    aImpactPoint = nearestImpactPoint;
    aHitBrick = qgraphicsitem_cast<Brick*>( nearestItem );
    return true;

}

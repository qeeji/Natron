/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "BezierCPPrivate.h"
#include "BezierCP.h"

#include <algorithm> // min, max
#include <limits> // INT_MAX
#include <set>
#include <list>
#include <utility>
#include <cassert>
#include <stdexcept>

#include <QtCore/QThread>
#include <QtCore/QCoreApplication>

#include "Engine/Bezier.h"
#include "Engine/KnobTypes.h"
#include "Engine/Transform.h" // Point3D
#include "Engine/ViewIdx.h"

#include "Serialization/BezierCPSerialization.h"

NATRON_NAMESPACE_ENTER;


////////////////////////////////////ControlPoint////////////////////////////////////

BezierCP::BezierCP()
    : _imp( new BezierCPPrivate( BezierPtr() ) )
{
}

BezierCP::BezierCP(const BezierCP & other)
    : _imp( new BezierCPPrivate( other._imp->holder.lock() ) )
{
    clone(other);
}

BezierCP::BezierCP(const BezierPtr& curve)
    : _imp( new BezierCPPrivate(curve) )
{
}

BezierCP::~BezierCP()
{
}

bool
BezierCP::getPositionAtTime(double time,
                            ViewIdx /*view*/,
                            double* x,
                            double* y) const
{
    bool ret = false;
    KeyFrame k;
    Curve* xCurve = _imp->curveX.get();
    Curve* yCurve = _imp->curveY.get();

    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            *x = _imp->x;
            *y = _imp->y;
        }

        ret = false;
    }

    return ret;
}

void
BezierCP::setPositionAtTime(double time,
                            double x,
                            double y)
{
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveX->addKeyFrame(k);
    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear); 
        _imp->curveY->addKeyFrame(k);
    }
}

void
BezierCP::setStaticPosition(double x,
                            double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);

    _imp->x = x;
    _imp->y = y;
}

void
BezierCP::setLeftBezierStaticPosition(double x,
                                      double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);

    _imp->leftX = x;
    _imp->leftY = y;
}

void
BezierCP::setRightBezierStaticPosition(double x,
                                       double y)
{
    QMutexLocker l(&_imp->staticPositionMutex);

    _imp->rightX = x;
    _imp->rightY = y;
}

bool
BezierCP::getLeftBezierPointAtTime(double time,
                                   ViewIdx /*view*/,
                                   double* x,
                                   double* y) const
{
    KeyFrame k;
    bool ret = false;
    Curve* xCurve = _imp->curveLeftBezierX.get();
    Curve* yCurve = _imp->curveLeftBezierY.get();

    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            *x = _imp->leftX;
            *y = _imp->leftY;
        }

        ret = false;
    }


    return ret;
} // BezierCP::getLeftBezierPointAtTime

bool
BezierCP::getRightBezierPointAtTime(double time,
                                    ViewIdx /*view*/,
                                    double *x,
                                    double *y) const
{
    KeyFrame k;
    bool ret = false;
    Curve* xCurve = _imp->curveRightBezierX.get();
    Curve* yCurve = _imp->curveRightBezierY.get();

    if ( xCurve->getKeyFrameWithTime(time, &k) ) {
        bool ok;
        *x = k.getValue();
        ok = yCurve->getKeyFrameWithTime(time, &k);
        assert(ok);
        if (ok) {
            *y = k.getValue();
            ret = true;
        }
    } else {
        try {
            *x = xCurve->getValueAt(time);
            *y = yCurve->getValueAt(time);
        } catch (const std::exception & e) {
            QMutexLocker l(&_imp->staticPositionMutex);
            *x = _imp->rightX;
            *y = _imp->rightY;
        }

        ret =  false;
    }


    return ret;
} // BezierCP::getRightBezierPointAtTime

void
BezierCP::setLeftBezierPointAtTime(double time,
                                   double x,
                                   double y)
{
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveLeftBezierX->addKeyFrame(k);

    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveLeftBezierY->addKeyFrame(k);

    }
}

void
BezierCP::setRightBezierPointAtTime(double time,
                                    double x,
                                    double y)
{
    {
        KeyFrame k(time, x);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveRightBezierX->addKeyFrame(k);
    }
    {
        KeyFrame k(time, y);
        k.setInterpolation(eKeyframeTypeLinear);
        _imp->curveRightBezierY->addKeyFrame(k);
    }
}

void
BezierCP::removeAnimation(double currentTime)
{
    {
        QMutexLocker k(&_imp->staticPositionMutex);
        try {
            _imp->x = _imp->curveX->getValueAt(currentTime);
            _imp->y = _imp->curveY->getValueAt(currentTime);
            _imp->leftX = _imp->curveLeftBezierX->getValueAt(currentTime);
            _imp->leftY = _imp->curveLeftBezierY->getValueAt(currentTime);
            _imp->rightX = _imp->curveRightBezierX->getValueAt(currentTime);
            _imp->rightY = _imp->curveRightBezierY->getValueAt(currentTime);
        } catch (const std::exception & e) {
            //
        }
    }
    _imp->curveX->clearKeyFrames();
    _imp->curveY->clearKeyFrames();
    _imp->curveLeftBezierX->clearKeyFrames();
    _imp->curveLeftBezierY->clearKeyFrames();
    _imp->curveRightBezierX->clearKeyFrames();
    _imp->curveRightBezierY->clearKeyFrames();

}

void
BezierCP::removeKeyframe(double time)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    ///if the keyframe count reaches 0 update the "static" values which may be fetched
    if (_imp->curveX->getKeyFramesCount() == 1) {
        QMutexLocker l(&_imp->staticPositionMutex);
        _imp->x = _imp->curveX->getValueAt(time);
        _imp->y = _imp->curveY->getValueAt(time);
        _imp->leftX = _imp->curveLeftBezierX->getValueAt(time);
        _imp->leftY = _imp->curveLeftBezierY->getValueAt(time);
        _imp->rightX = _imp->curveRightBezierX->getValueAt(time);
        _imp->rightY = _imp->curveRightBezierY->getValueAt(time);
    }

    try {
        _imp->curveX->removeKeyFrameWithTime(time);
        _imp->curveY->removeKeyFrameWithTime(time);
        _imp->curveLeftBezierX->removeKeyFrameWithTime(time);
        _imp->curveLeftBezierY->removeKeyFrameWithTime(time);
        _imp->curveRightBezierX->removeKeyFrameWithTime(time);
        _imp->curveRightBezierY->removeKeyFrameWithTime(time);
    } catch (...) {
    }
}

bool
BezierCP::hasKeyFrameAtTime(double time) const
{
    KeyFrame k;
    return _imp->curveX->getKeyFrameWithTime(time, &k);

}

void
BezierCP::getKeyframeTimes(std::set<double>* times) const
{
    KeyFrameSet set;
    set = _imp->curveX->getKeyFrames_mt_safe();

    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        times->insert( it->getTime() );
    }
}

void
BezierCP::getKeyFrames(std::list<std::pair<double, KeyframeTypeEnum> >* keys) const
{
    KeyFrameSet set;

    set = _imp->curveX->getKeyFrames_mt_safe();

    for (KeyFrameSet::iterator it = set.begin(); it != set.end(); ++it) {
        keys->push_back( std::make_pair( it->getTime(), it->getInterpolation() ) );
    }
}

int
BezierCP::getKeyFrameIndex(double time) const
{
    return _imp->curveX->keyFrameIndex(time);
}

void
BezierCP::setKeyFrameInterpolation(KeyframeTypeEnum interp,
                                   int index)
{
    _imp->curveX->setKeyFrameInterpolation(interp, index);
    _imp->curveY->setKeyFrameInterpolation(interp, index);
    _imp->curveLeftBezierX->setKeyFrameInterpolation(interp, index);
    _imp->curveLeftBezierY->setKeyFrameInterpolation(interp, index);
    _imp->curveRightBezierX->setKeyFrameInterpolation(interp, index);
    _imp->curveRightBezierY->setKeyFrameInterpolation(interp, index);
}

double
BezierCP::getKeyframeTime(int index) const
{
    KeyFrame k;
    bool ok;

    ok = _imp->curveX->getKeyFrameWithIndex(index, &k);
    if (ok) {
        return k.getTime();
    } else {
        return INT_MAX;
    }
}

int
BezierCP::getKeyframesCount() const
{
    return _imp->curveX->getKeyFramesCount();
}

int
BezierCP::getControlPointsCount() const
{
    BezierPtr b = _imp->holder.lock();

    assert(b);

    return b->getControlPointsCount();
}

BezierPtr
BezierCP::getBezier() const
{
    BezierPtr b = _imp->holder.lock();

    assert(b);

    return b;
}

int
BezierCP::isNearbyTangent(double time,
                          ViewIdx view,
                          double x,
                          double y,
                          double acceptance) const
{
    Transform::Point3D p, left, right;

    p.z = left.z = right.z = 1;

    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);

    getPositionAtTime(time, view, &p.x, &p.y);
    getLeftBezierPointAtTime(time, view, &left.x, &left.y);
    getRightBezierPointAtTime(time, view, &right.x, &right.y);

    p = Transform::matApply(transform, p);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);

    if ( (p.x != left.x) || (p.y != left.y) ) {
        if ( ( left.x >= (x - acceptance) ) && ( left.x <= (x + acceptance) ) && ( left.y >= (y - acceptance) ) && ( left.y <= (y + acceptance) ) ) {
            return 0;
        }
    }
    if ( (p.x != right.x) || (p.y != right.y) ) {
        if ( ( right.x >= (x - acceptance) ) && ( right.x <= (x + acceptance) ) && ( right.y >= (y - acceptance) ) && ( right.y <= (y + acceptance) ) ) {
            return 1;
        }
    }

    return -1;
}

#define TANGENTS_CUSP_LIMIT 25


NATRON_NAMESPACE_ANONYMOUS_ENTER

static void
cuspTangent(double x,
            double y,
            double *tx,
            double *ty,
            const std::pair<double, double>& pixelScale)
{
    ///decrease the tangents distance by 1 fourth
    ///if the tangents are equal to the control point, make them 10 pixels long
    double dx = *tx - x;
    double dy = *ty - y;
    double distSquare = dx * dx + dy * dy;

    if (distSquare <= pixelScale.first * pixelScale.second * TANGENTS_CUSP_LIMIT * TANGENTS_CUSP_LIMIT) {
        *tx = x;
        *ty = y;
    } else {
        double newDx = 0.9 * dx;
        double newDy = 0.9 * dy;
        *tx = x + newDx;
        *ty = y + newDy;
    }
}

static void
smoothTangent(double time,
              bool left,
              const BezierCP* p,
              const Transform::Matrix3x3& transform,
              double x,
              double y,
              double *tx,
              double *ty,
              const std::pair<double, double>& pixelScale)
{
    if ( (x == *tx) && (y == *ty) ) {
        const std::list < BezierCPPtr > & cps = ( p->isFeatherPoint() ?
                                                                  p->getBezier()->getFeatherPoints() :
                                                                  p->getBezier()->getControlPoints() );

        if (cps.size() == 1) {
            return;
        }

        std::list < BezierCPPtr >::const_iterator prev = cps.end();
        if ( prev != cps.begin() ) {
            --prev;
        }
        std::list < BezierCPPtr >::const_iterator next = cps.begin();
        if ( next != cps.end() ) {
            ++next;
        }

        int index = 0;
        int cpCount = (int)cps.size();
        for (std::list < BezierCPPtr >::const_iterator it = cps.begin();
             it != cps.end();
             ++it) {
            if ( prev == cps.end() ) {
                prev = cps.begin();
            }
            if ( next == cps.end() ) {
                next = cps.begin();
            }
            if (it->get() == p) {
                break;
            }

            // increment for next iteration
            if ( prev != cps.end() ) {
                ++prev;
            }
            if ( next != cps.end() ) {
                ++next;
            }
            ++index;
        } // for(it)
        if ( prev == cps.end() ) {
            prev = cps.begin();
        }
        if ( next == cps.end() ) {
            next = cps.begin();
        }

        assert(index < cpCount);
        Q_UNUSED(cpCount);

        double leftDx, leftDy, rightDx, rightDy;
        Bezier::leftDerivativeAtPoint(time, *p, **prev, transform, &leftDx, &leftDy);
        Bezier::rightDerivativeAtPoint(time, *p, **next, transform, &rightDx, &rightDy);
        double norm = sqrt( (rightDx - leftDx) * (rightDx - leftDx) + (rightDy - leftDy) * (rightDy - leftDy) );
        Point delta;
        ///normalize derivatives by their norm
        if (norm != 0) {
            delta.x = ( (rightDx - leftDx) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
            delta.y = ( (rightDy - leftDy) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
        } else {
            ///both derivatives are the same, use the direction of the left one
            norm = sqrt( (leftDx - x) * (leftDx - x) + (leftDy - y) * (leftDy - y) );
            if (norm != 0) {
                delta.x = ( (rightDx - x) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.first;
                delta.y = ( (leftDy - y) / norm ) * TANGENTS_CUSP_LIMIT * pixelScale.second;
            } else {
                ///both derivatives and control point are equal, just use 0
                delta.x = delta.y = 0;
            }
        }

        if (!left) {
            *tx = x + delta.x;
            *ty = y + delta.y;
        } else {
            *tx = x - delta.x;
            *ty = y - delta.y;
        }
    } else {
        ///increase the tangents distance by 1 fourth
        ///if the tangents are equal to the control point, make them 10 pixels long
        double dx = *tx - x;
        double dy = *ty - y;
        double newDx, newDy;
        if ( (dx == 0) && (dy == 0) ) {
            dx = (dx < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.first;
            dy = (dy < 0 ? -TANGENTS_CUSP_LIMIT : TANGENTS_CUSP_LIMIT) * pixelScale.second;
        }
        newDx = dx * 1.1;
        newDy = dy * 1.1;

        *tx = x + newDx;
        *ty = y + newDy;
    }
} // smoothTangent

NATRON_NAMESPACE_ANONYMOUS_EXIT


bool
BezierCP::cuspPoint(double time,
                    ViewIdx view,
                    bool autoKeying,
                    bool rippleEdit,
                    const std::pair<double, double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    double x, y, leftX, leftY, rightX, rightY;
    getPositionAtTime(time, view, &x, &y);
    getLeftBezierPointAtTime(time, view, &leftX, &leftY);
    bool isOnKeyframe = getRightBezierPointAtTime(time, view, &rightX, &rightY);
    double newLeftX = leftX, newLeftY = leftY, newRightX = rightX, newRightY = rightY;
    cuspTangent(x, y, &newLeftX, &newLeftY, pixelScale);
    cuspTangent(x, y, &newRightX, &newRightY, pixelScale);

    bool keyframeSet = false;

    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, newLeftX, newLeftY);
        setRightBezierPointAtTime(time, newRightX, newRightY);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        getKeyframeTimes(&times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(*it, newLeftX, newLeftY);
            setRightBezierPointAtTime(*it, newRightX, newRightY);
        }
    }

    return keyframeSet;
}

bool
BezierCP::smoothPoint(double time,
                      ViewIdx view,
                      bool autoKeying,
                      bool rippleEdit,
                      const std::pair<double, double>& pixelScale)
{
    ///only called on the main-thread
    assert( QThread::currentThread() == qApp->thread() );

    Transform::Matrix3x3 transform;
    getBezier()->getTransformAtTime(time, &transform);

    Transform::Point3D pos, left, right;
    pos.z = left.z = right.z = 1.;

    getPositionAtTime(time, view, &pos.x, &pos.y);

    getLeftBezierPointAtTime(time, view, &left.x, &left.y);
    bool isOnKeyframe = getRightBezierPointAtTime(time, view, &right.x, &right.y);

    pos = Transform::matApply(transform, pos);
    left = Transform::matApply(transform, left);
    right = Transform::matApply(transform, right);

    smoothTangent(time, true, this, transform, pos.x, pos.y, &left.x, &left.y, pixelScale);
    smoothTangent(time, false, this, transform, pos.x, pos.y, &right.x, &right.y, pixelScale);

    bool keyframeSet = false;

    transform = Transform::matInverse(transform);

    if (autoKeying || isOnKeyframe) {
        setLeftBezierPointAtTime(time, left.x, left.y);
        setRightBezierPointAtTime(time, right.x, right.y);
        if (!isOnKeyframe) {
            keyframeSet = true;
        }
    }

    if (rippleEdit) {
        std::set<double> times;
        getKeyframeTimes(&times);
        for (std::set<double>::iterator it = times.begin(); it != times.end(); ++it) {
            setLeftBezierPointAtTime(*it, left.x, left.y);
            setRightBezierPointAtTime(*it, right.x, right.y);
        }
    }

    return keyframeSet;
} // BezierCP::smoothPoint

CurvePtr
BezierCP::getXCurve() const
{
    return _imp->curveX;
}

CurvePtr
BezierCP::getYCurve() const
{
    return _imp->curveY;
}

CurvePtr
BezierCP::getLeftXCurve() const
{
    return _imp->curveLeftBezierX;
}

CurvePtr
BezierCP::getLeftYCurve() const
{
    return _imp->curveLeftBezierY;
}

CurvePtr
BezierCP::getRightXCurve() const
{
    return _imp->curveRightBezierX;
}

CurvePtr
BezierCP::getRightYCurve() const
{
    return _imp->curveRightBezierY;
}

void
BezierCP::clone(const BezierCP & other)
{
    _imp->curveX->clone(*other._imp->curveX);
    _imp->curveY->clone(*other._imp->curveY);
    _imp->curveLeftBezierX->clone(*other._imp->curveLeftBezierX);
    _imp->curveLeftBezierY->clone(*other._imp->curveLeftBezierY);
    _imp->curveRightBezierX->clone(*other._imp->curveRightBezierX);
    _imp->curveRightBezierY->clone(*other._imp->curveRightBezierY);


    {
        QMutexLocker l(&_imp->staticPositionMutex);
        _imp->x = other._imp->x;
        _imp->y = other._imp->y;
        _imp->leftX = other._imp->leftX;
        _imp->leftY = other._imp->leftY;
        _imp->rightX = other._imp->rightX;
        _imp->rightY = other._imp->rightY;
    }
}

bool
BezierCP::equalsAtTime(double time,
                       ViewIdx view,
                       const BezierCP & other) const
{
    double x, y, leftX, leftY, rightX, rightY;

    getPositionAtTime(time, view, &x, &y);
    getLeftBezierPointAtTime(time, view, &leftX, &leftY);
    getRightBezierPointAtTime(time, view, &rightX, &rightY);

    double ox, oy, oLeftX, oLeftY, oRightX, oRightY;
    other.getPositionAtTime(time, view, &ox, &oy);
    other.getLeftBezierPointAtTime(time, view, &oLeftX, &oLeftY);
    other.getRightBezierPointAtTime(time, view, &oRightX, &oRightY);

    if ( (x == ox) && (y == oy) && (leftX == oLeftX) && (leftY == oLeftY) && (rightX == oRightX) && (rightY == oRightY) ) {
        return true;
    }

    return false;
}

bool
BezierCP::operator==(const BezierCP& other) const
{
    if (*_imp->curveX != *other._imp->curveX) {
        return false;
    }
    if (*_imp->curveY != *other._imp->curveY) {
        return false;
    }
    if (*_imp->curveLeftBezierX != *other._imp->curveLeftBezierX) {
        return false;
    }
    if (*_imp->curveLeftBezierY != *other._imp->curveLeftBezierY) {
        return false;
    }
    if (*_imp->curveRightBezierX != *other._imp->curveRightBezierX) {
        return false;
    }
    if (*_imp->curveRightBezierY != *other._imp->curveRightBezierY) {
        return false;
    }

    if (_imp->x != other._imp->x) {
        return false;
    }
    if (_imp->y != other._imp->y) {
        return false;
    }
    if (_imp->leftX != other._imp->leftX) {
        return false;
    }
    if (_imp->leftY != other._imp->leftY) {
        return false;
    }
    if (_imp->rightX != other._imp->rightX) {
        return false;
    }
    if (_imp->rightY != other._imp->rightY) {
        return false;
    }
    return true;
    
}

void
BezierCP::cloneInternalCurvesToGuiCurves()
{
    _imp->guiCurveX->clone(*_imp->curveX);
    _imp->guiCurveY->clone(*_imp->curveY);


    _imp->guiCurveLeftBezierX->clone(*_imp->curveLeftBezierX);
    _imp->guiCurveLeftBezierY->clone(*_imp->curveLeftBezierY);
    _imp->guiCurveRightBezierX->clone(*_imp->curveRightBezierX);
    _imp->guiCurveRightBezierY->clone(*_imp->curveRightBezierY);

    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->guiX = _imp->x;
    _imp->guiY = _imp->y;
    _imp->guiLeftX = _imp->leftX;
    _imp->guiLeftY = _imp->leftY;
    _imp->guiRightX = _imp->rightX;
    _imp->guiRightY = _imp->rightY;
}

void
BezierCP::cloneGuiCurvesToInternalCurves()
{
    _imp->curveX->clone(*_imp->guiCurveX);
    _imp->curveY->clone(*_imp->guiCurveY);


    _imp->curveLeftBezierX->clone(*_imp->guiCurveLeftBezierX);
    _imp->curveLeftBezierY->clone(*_imp->guiCurveLeftBezierY);
    _imp->curveRightBezierX->clone(*_imp->guiCurveRightBezierX);
    _imp->curveRightBezierY->clone(*_imp->guiCurveRightBezierY);

    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->x = _imp->guiX;
    _imp->y = _imp->guiY;
    _imp->leftX = _imp->guiLeftX;
    _imp->leftY = _imp->guiLeftY;
    _imp->rightX = _imp->guiRightX;
    _imp->rightY = _imp->guiRightY;
}

void
BezierCP::toSerialization(SERIALIZATION_NAMESPACE::SerializationObjectBase* obj)
{
    SERIALIZATION_NAMESPACE::BezierCPSerialization* s = dynamic_cast<SERIALIZATION_NAMESPACE::BezierCPSerialization*>(obj);
    if (!s) {
        return;
    }
    _imp->curveX->toSerialization(&s->xCurve);
    _imp->curveY->toSerialization(&s->yCurve);
    _imp->curveLeftBezierX->toSerialization(&s->leftCurveX);
    _imp->curveLeftBezierY->toSerialization(&s->leftCurveY);
    _imp->curveRightBezierX->toSerialization(&s->rightCurveX);
    _imp->curveRightBezierY->toSerialization(&s->rightCurveY);

    QMutexLocker k(&_imp->staticPositionMutex);
    s->x = _imp->x;
    s->y = _imp->y;
    s->leftX = _imp->leftX;
    s->leftY = _imp->leftY;
    s->rightX = _imp->rightX;
    s->rightY = _imp->rightY;
}

void
BezierCP::fromSerialization(const SERIALIZATION_NAMESPACE::SerializationObjectBase & obj)
{
    const SERIALIZATION_NAMESPACE::BezierCPSerialization* s = dynamic_cast<const SERIALIZATION_NAMESPACE::BezierCPSerialization*>(&obj);
    if (!s) {
        return;
    }
    _imp->curveX->fromSerialization(s->xCurve);
    _imp->curveY->fromSerialization(s->yCurve);
    _imp->curveLeftBezierX->fromSerialization(s->leftCurveX);
    _imp->curveLeftBezierY->fromSerialization(s->leftCurveY);
    _imp->curveRightBezierX->fromSerialization(s->rightCurveX);
    _imp->curveRightBezierY->fromSerialization(s->rightCurveY);

    QMutexLocker k(&_imp->staticPositionMutex);
    _imp->x = s->x;
    _imp->y = s->y;
    _imp->leftX = s->leftX;
    _imp->leftY = s->leftY;
    _imp->rightX = s->rightX;
    _imp->rightY = s->rightY;

}

NATRON_NAMESPACE_EXIT;


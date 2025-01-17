/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Geom_BSplineCurve.hxx>
# include <GeomAPI_PointsToBSpline.hxx>
# include <GeomAPI_Interpolate.hxx>
# include <GeomConvert_BSplineCurveToBezierCurve.hxx>
# include <gp_Pnt.hxx>
# include <TColStd_Array1OfReal.hxx>
# include <TColgp_Array1OfPnt.hxx>
# include <TColgp_HArray1OfPnt.hxx>
# include <TColStd_Array1OfInteger.hxx>
# include <Handle_TColgp_HArray1OfPnt.hxx>
# include <Precision.hxx>
#endif

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "Geometry.h"
#include "BSplineCurvePy.h"
#include "BSplineCurvePy.cpp"
#include "BezierCurvePy.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BSplineCurvePy::representation(void) const
{
    return "<BSplineCurve object>";
}

PyObject *BSplineCurvePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of BSplineCurvePy and the Twin object 
    return new BSplineCurvePy(new GeomBSplineCurve);
}

// constructor method
int BSplineCurvePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "B-Spline constructor accepts:\n"
        "-- empty parameter list\n");
    return -1;
}

PyObject* BSplineCurvePy::isRational(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsRational();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineCurvePy::isPeriodic(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsPeriodic();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineCurvePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    Standard_Boolean val = curve->IsClosed();
    if (val) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineCurvePy::increaseDegree(PyObject * args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i", &degree))
        return 0;
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    curve->IncreaseDegree(degree);
    Py_Return;
}

PyObject* BSplineCurvePy::increaseMultiplicity(PyObject * args)
{
    int mult=-1;
    int start, end;
    if (!PyArg_ParseTuple(args, "ii|i", &start, &end, &mult))
        return 0;

    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    if (mult == -1) {
        mult = end;
        curve->IncreaseMultiplicity(start, mult);
    }
    else {
        curve->IncreaseMultiplicity(start, end, mult);
    }

    Py_Return;
}

PyObject* BSplineCurvePy::incrementMultiplicity(PyObject * args)
{
    int start, end, mult;
    if (!PyArg_ParseTuple(args, "iii", &start, &end, &mult))
        return 0;

    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->IncrementMultiplicity(start, end, mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::insertKnot(PyObject * args)
{
    double U, tol = 0.0;
    int M=1;
    PyObject* add = Py_True;
    if (!PyArg_ParseTuple(args, "d|idO!", &U, &M, &tol, &PyBool_Type, &add))
        return 0;

    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->InsertKnot(U,M,tol,(add==Py_True));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::insertKnots(PyObject * args)
{
    double tol = 0.0;
    PyObject* add = Py_True;
    PyObject* obj1;
    PyObject* obj2;
    if (!PyArg_ParseTuple(args, "O!O!|dO!", &PyList_Type, &obj1,
                                            &PyList_Type, &obj2,
                                            &tol, &PyBool_Type, &add))
        return 0;

    try {
        Py::List knots(obj1);
        TColStd_Array1OfReal k(1,knots.size());
        int index=1;
        for (Py::List::iterator it = knots.begin(); it != knots.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }
        Py::List mults(obj2);
        TColStd_Array1OfInteger m(1,mults.size());
        index=1;
        for (Py::List::iterator it = mults.begin(); it != mults.end(); ++it) {
            Py::Int val(*it);
            m(index++) = (int)val;
        }

        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->InsertKnots(k,m,tol,(add==Py_True));
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }

    Py_Return;
}

PyObject* BSplineCurvePy::removeKnot(PyObject * args)
{
    double tol;
    int Index,M;
    if (!PyArg_ParseTuple(args, "iid", &Index, &M, &tol))
        return 0;

    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        Standard_Boolean ok = curve->RemoveKnot(Index,M,tol);
        if (ok) {
            Py_INCREF(Py_True);
            return Py_True;
        }
        else {
            Py_INCREF(Py_False);
            return Py_False;
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::segment(PyObject * args)
{
    double u1,u2;
    if (!PyArg_ParseTuple(args, "dd", &u1,&u2))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->Segment(u1,u2);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setKnot(PyObject * args)
{
    int Index, M=-1;
    double K;
    if (!PyArg_ParseTuple(args, "id|i", &Index, &K, &M))
        return 0;

    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    if (M == -1) {
        curve->SetKnot(Index, K);
    }
    else {
        curve->SetKnot(Index, K, M);
    }

    Py_Return;
}

PyObject* BSplineCurvePy::getKnot(PyObject * args)
{
    int Index;
    if (!PyArg_ParseTuple(args, "i", &Index))
        return 0;

    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    double M = curve->Knot(Index);

    return Py_BuildValue("d",M);
}

PyObject* BSplineCurvePy::setKnots(PyObject * args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &obj))
        return 0;
    try {
        Py::List list(obj);
        TColStd_Array1OfReal k(1,list.size());
        int index=1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Float val(*it);
            k(index++) = (double)val;
        }

        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->SetKnots(k);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getKnots(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,curve->NbKnots());
        curve->Knots(w);
        Py::List knots;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            knots.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(knots);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setPole(PyObject * args)
{
    int index;
    double weight=-1.0;
    PyObject* p;
    if (!PyArg_ParseTuple(args, "iO!|d", &index, &(Base::VectorPy::Type), &p, &weight))
        return 0;
    Base::Vector3d vec = static_cast<Base::VectorPy*>(p)->value();
    gp_Pnt pnt(vec.x, vec.y, vec.z);
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        if (weight < 0.0)
            curve->SetPole(index,pnt);
        else
            curve->SetPole(index,pnt,weight);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getPole(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles(), "Pole index out of range");
        gp_Pnt pnt = curve->Pole(index);
        Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
            pnt.X(), pnt.Y(), pnt.Z()));
        return vec;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getPoles(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        TColgp_Array1OfPnt p(1,curve->NbPoles());
        curve->Poles(p);
        Py::List poles;
        for (Standard_Integer i=p.Lower(); i<=p.Upper(); i++) {
            gp_Pnt pnt = p(i);
            Base::VectorPy* vec = new Base::VectorPy(Base::Vector3d(
                pnt.X(), pnt.Y(), pnt.Z()));
            poles.append(Py::Object(vec));
        }
        return Py::new_reference_to(poles);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setWeight(PyObject * args)
{
    int index;
    double weight;
    if (!PyArg_ParseTuple(args, "id", &index,&weight))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->SetWeight(index,weight);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getWeight(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        Standard_OutOfRange_Raise_if
            (index < 1 || index > curve->NbPoles() , "Weight index out of range");
        double weight = curve->Weight(index);
        return Py_BuildValue("d", weight);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getWeights(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfReal w(1,curve->NbPoles());
        curve->Weights(w);
        Py::List weights;
        for (Standard_Integer i=w.Lower(); i<=w.Upper(); i++) {
            weights.append(Py::Float(w(i)));
        }
        return Py::new_reference_to(weights);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getResolution(PyObject * args)
{
    double tol;
    if (!PyArg_ParseTuple(args, "d", &tol))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        double utol;
        curve->Resolution(tol,utol);
        return Py_BuildValue("d",utol);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::movePoint(PyObject * args)
{
    double U;
    int index1, index2;
    PyObject* pnt;
    if (!PyArg_ParseTuple(args, "dO!ii", &U, &(Base::VectorPy::Type),&pnt, &index1, &index2))
        return 0;
    try {
        Base::Vector3d p = static_cast<Base::VectorPy*>(pnt)->value();
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        int first, last;
        curve->MovePoint(U, gp_Pnt(p.x,p.y,p.z), index1, index2, first, last);
        return Py_BuildValue("(ii)",first, last);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setNotPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->SetNotPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setPeriodic(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->SetPeriodic();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::setOrigin(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        curve->SetOrigin(index);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getMultiplicity(PyObject * args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        int mult = curve->Multiplicity(index);
        return Py_BuildValue("i", mult);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::getMultiplicities(PyObject * args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    try {
        Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
            (getGeometryPtr()->handle());
        TColStd_Array1OfInteger m(1,curve->NbKnots());
        curve->Multiplicities(m);
        Py::List mults;
        for (Standard_Integer i=m.Lower(); i<=m.Upper(); i++) {
            mults.append(Py::Int(m(i)));
        }
        return Py::new_reference_to(mults);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

Py::Int BSplineCurvePy::getDegree(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->Degree()); 
}

Py::Int BSplineCurvePy::getMaxDegree(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->MaxDegree()); 
}

Py::Int BSplineCurvePy::getNbPoles(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->NbPoles()); 
}

Py::Int BSplineCurvePy::getNbKnots(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->NbKnots()); 
}

Py::Object BSplineCurvePy::getStartPoint(void) const
{
    Handle_Geom_BSplineCurve c = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->StartPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

Py::Object BSplineCurvePy::getEndPoint(void) const
{
    Handle_Geom_BSplineCurve c = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    gp_Pnt pnt = c->EndPoint();
    return Py::Vector(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
}

Py::Object BSplineCurvePy::getFirstUKnotIndex(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->FirstUKnotIndex()); 
}

Py::Object BSplineCurvePy::getLastUKnotIndex(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    return Py::Int(curve->LastUKnotIndex()); 
}

Py::List BSplineCurvePy::getKnotSequence(void) const
{
    Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast
        (getGeometryPtr()->handle());
    Standard_Integer m = 0;
    for (int i=1; i<= curve->NbKnots(); i++)
        m += curve->Multiplicity(i);
    TColStd_Array1OfReal k(1,m);
    curve->KnotSequence(k);
    Py::List list;
    for (Standard_Integer i=k.Lower(); i<=k.Upper(); i++) {
        list.append(Py::Float(k(i)));
    }
    return list;
}

PyObject* BSplineCurvePy::approximate(PyObject *args)
{
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!",&(PyList_Type), &obj))
        return 0;
    try {
        Py::List list(obj);
        TColgp_Array1OfPnt pnts(1,list.size());
        Standard_Integer index = 1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Base::Vector3d vec = Py::Vector(*it).toVector();
            pnts(index++) = gp_Pnt(vec.x,vec.y,vec.z);
        }

        GeomAPI_PointsToBSpline fit(pnts);
        Handle_Geom_BSplineCurve spline = fit.Curve();
        if (!spline.IsNull()) {
            this->getGeomBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to approximate points");
            return 0; // goes to the catch block
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::interpolate(PyObject *args)
{
    PyObject* obj;
    double tol3d = Precision::Approximation();
    PyObject* closed = Py_False;
    PyObject* t1=0; PyObject* t2=0;
    if (!PyArg_ParseTuple(args, "O!|O!dO!O!",&(PyList_Type), &obj, &PyBool_Type, &closed, &tol3d,
                                             &Base::VectorPy::Type, &t1, &Base::VectorPy::Type, &t2))
        return 0;
    try {
        Py::List list(obj);
        Handle_TColgp_HArray1OfPnt interpolationPoints = new TColgp_HArray1OfPnt(1, list.size());
        Standard_Integer index = 1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            interpolationPoints->SetValue(index++, gp_Pnt(pnt.x,pnt.y,pnt.z));
        }

        if (interpolationPoints->Length() < 2) {
            Standard_Failure::Raise("not enough points given");
        }

        GeomAPI_Interpolate aBSplineInterpolation(interpolationPoints, (closed == Py_True), tol3d);
        if (t1 && t2) {
            Base::Vector3d v1 = Py::Vector(t1,false).toVector();
            Base::Vector3d v2 = Py::Vector(t1,false).toVector();
            gp_Vec initTangent(v1.x,v1.y,v1.z), finalTangent(v2.x,v2.y,v2.z);
            aBSplineInterpolation.Load(initTangent, finalTangent);
        }
        aBSplineInterpolation.Perform();
        if (aBSplineInterpolation.IsDone()) {
            Handle_Geom_BSplineCurve aBSplineCurve(aBSplineInterpolation.Curve());
            this->getGeomBSplineCurvePtr()->setHandle(aBSplineCurve);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to interpolate points");
            return 0; // goes to the catch block
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        std::string err = e->GetMessageString();
        if (err.empty()) err = e->DynamicType()->Name();
        PyErr_SetString(PyExc_Exception, err.c_str());
        return 0;
    }
}

PyObject* BSplineCurvePy::buildFromPoles(PyObject *args)
{
    PyObject* obj;
    int degree = 3;
    PyObject* closed = Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!i",&(PyList_Type), &obj, &PyBool_Type, &closed, &degree))
        return 0;
    try {
        Py::List list(obj);
        TColgp_Array1OfPnt poles(1, list.size());
        Standard_Integer index = 1;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Vector v(*it);
            Base::Vector3d pnt = v.toVector();
            poles(index++) = gp_Pnt(pnt.x,pnt.y,pnt.z);
        }

        if (poles.Length() <= degree)
             degree = poles.Length()-1;

        TColStd_Array1OfReal knots(1, poles.Length()+degree+1-2*(degree));
        TColStd_Array1OfInteger mults(1, poles.Length()+degree+1-2*(degree));
        for (int i=1; i<=knots.Length(); i++){
            knots.SetValue(i,(double)(i-1)/(knots.Length()-1));
            mults.SetValue(i,1);
        }
        mults.SetValue(1, degree+1);
        mults.SetValue(knots.Length(), degree+1);

        Handle_Geom_BSplineCurve spline = new Geom_BSplineCurve(poles, knots, mults, degree, (closed == Py_True));
        if (!spline.IsNull()) {
            this->getGeomBSplineCurvePtr()->setHandle(spline);
            Py_Return;
        }
        else {
            Standard_Failure::Raise("failed to create spline");
            return 0; // goes to the catch block
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* BSplineCurvePy::toBezier(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    Handle_Geom_BSplineCurve spline = Handle_Geom_BSplineCurve::DownCast
        (this->getGeomBSplineCurvePtr()->handle());
    GeomConvert_BSplineCurveToBezierCurve crt(spline);

    Py::List list;
    Standard_Integer arcs = crt.NbArcs();
    for (Standard_Integer i=1; i<=arcs; i++) {
        Handle_Geom_BezierCurve bezier = crt.Arc(i);
        list.append(Py::asObject(new BezierCurvePy(new GeomBezierCurve(bezier))));
    }

    return Py::new_reference_to(list);
}

PyObject* BSplineCurvePy::join(PyObject *args)
{
    PyObject* c;
    if (!PyArg_ParseTuple(args, "O!", &BSplineCurvePy::Type, &c))
        return 0;

    GeomBSplineCurve* curve1 = this->getGeomBSplineCurvePtr();
    BSplineCurvePy* curve2 = static_cast<BSplineCurvePy*>(c);
    Handle_Geom_BSplineCurve spline = Handle_Geom_BSplineCurve::DownCast
        (curve2->getGeomBSplineCurvePtr()->handle());

    bool ok = curve1->join(spline);

    if (ok) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

PyObject* BSplineCurvePy::makeC1Continuous(PyObject *args)
{
    double tol = Precision::Approximation();
    double ang_tol = 1.0e-7;
    if (!PyArg_ParseTuple(args, "|dd", &tol, &ang_tol))
        return 0;

    try {
        GeomBSplineCurve* spline = this->getGeomBSplineCurvePtr();
        spline->makeC1Continuous(tol, ang_tol);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        std::string err = e->GetMessageString();
        if (err.empty()) err = e->DynamicType()->Name();
        PyErr_SetString(PyExc_Exception, err.c_str());
        return 0;
    }
}

PyObject* BSplineCurvePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BSplineCurvePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

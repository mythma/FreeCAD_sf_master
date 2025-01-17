/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepAdaptor_Surface.hxx>
#endif

#include "TaskTransformedParameters.h"
#include "TaskMultiTransformParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureAdditive.h>
#include <Mod/PartDesign/App/FeatureSubtractive.h>

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskTransformedParameters */

TaskTransformedParameters::TaskTransformedParameters(ViewProviderTransformed *TransformedView, QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap((std::string("PartDesign_") + TransformedView->featureName).c_str()),
              QString::fromAscii((TransformedView->featureName + " parameters").c_str()),
              true,
              parent),
      TransformedView(TransformedView),
      parentTask(NULL),
      insideMultiTransform(false),
      blockUpdate(false)
{
    originalSelectionMode = false;
}

TaskTransformedParameters::TaskTransformedParameters(TaskMultiTransformParameters *parentTask)
    : TaskBox(QPixmap(), tr(""), true, parentTask),
      TransformedView(NULL),
      parentTask(parentTask),
      insideMultiTransform(true),
      blockUpdate(false)
{
    // Original feature selection makes no sense inside a MultiTransform
    originalSelectionMode = false;
}

const bool TaskTransformedParameters::originalSelected(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection && originalSelectionMode) {

        if (strcmp(msg.pDocName, getObject()->getDocument()->getName()) != 0)
            return false;

        PartDesign::Transformed* pcTransformed = getObject();
        App::DocumentObject* selectedObject = pcTransformed->getDocument()->getObject(msg.pObjectName);
        if (selectedObject->isDerivedFrom(PartDesign::Additive::getClassTypeId()) ||
            selectedObject->isDerivedFrom(PartDesign::Subtractive::getClassTypeId())) {

            // Do the same like in TaskDlgTransformedParameters::accept() but without doCommand
            std::vector<App::DocumentObject*> originals(1,selectedObject);
            pcTransformed->Originals.setValues(originals);
            recomputeFeature();

            originalSelectionMode = false;
            return true;
        }
    }

    return false;
}

PartDesign::Transformed *TaskTransformedParameters::getObject() const
{

    if (insideMultiTransform)
        return parentTask->getSubFeature();
    else
        return static_cast<PartDesign::Transformed*>(TransformedView->getObject());
}

void TaskTransformedParameters::recomputeFeature()
{
    if (insideMultiTransform) {
        // redirect recompute and let the parent decide if recompute has to be blocked
        parentTask->recomputeFeature();
    } else if (!blockUpdate) {
        TransformedView->recomputeFeature();
    }
}

const std::vector<App::DocumentObject*> TaskTransformedParameters::getOriginals(void) const
{
    if (insideMultiTransform) {
        return parentTask->getOriginals();
    } else {
        PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

        return originals;
    }
}

App::DocumentObject* TaskTransformedParameters::getSupportObject() const
{
    if (insideMultiTransform) {
        return parentTask->getSupportObject();
    } else {
        PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
        return pcTransformed->getSupportObject();
    }
}

void TaskTransformedParameters::hideObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc)
        if (insideMultiTransform) {
            doc->setHide(parentTask->TransformedView->getObject()->getNameInDocument());
        } else {
            doc->setHide(TransformedView->getObject()->getNameInDocument());
        }
}

void TaskTransformedParameters::showObject()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc)
        if (insideMultiTransform) {
            doc->setShow(parentTask->TransformedView->getObject()->getNameInDocument());
        } else {
            doc->setShow(TransformedView->getObject()->getNameInDocument());
        }
}

void TaskTransformedParameters::hideOriginals()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        std::vector<App::DocumentObject*> originals = getOriginals();
        for (std::vector<App::DocumentObject*>::iterator it = originals.begin(); it != originals.end(); ++it)
            doc->setHide((*it)->getNameInDocument());
    }
}

void TaskTransformedParameters::showOriginals()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (doc) {
        std::vector<App::DocumentObject*> originals = getOriginals();
        for (std::vector<App::DocumentObject*>::iterator it = originals.begin(); it != originals.end(); ++it)
            doc->setShow((*it)->getNameInDocument());
    }
}

void TaskTransformedParameters::exitSelectionMode()
{
    originalSelectionMode = false;
    referenceSelectionMode = false;
    Gui::Selection().rmvSelectionGate();
    showObject();
    hideOriginals();
}

class ReferenceSelection : public Gui::SelectionFilterGate
{
    const App::DocumentObject* support;
    bool edge, plane;
public:
    ReferenceSelection(const App::DocumentObject* support_, bool edge_, bool plane_)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)0),
          support(support_), edge(edge_), plane(plane_)
    {
    }
    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
    {
        if (!sSubName || sSubName[0] == '\0')
            return false;
        if (pObj != support)
            return false;
        std::string subName(sSubName);
        if (edge && subName.size() > 4 && subName.substr(0,4) == "Edge")
            return true;
        if (plane && subName.size() > 4 && subName.substr(0,4) == "Face") {
            const Part::TopoShape &shape = static_cast<const Part::Feature*>(support)->Shape.getValue();
            TopoDS_Shape sh = shape.getSubShape(subName.c_str());
            const TopoDS_Face& face = TopoDS::Face(sh);
            if (!face.IsNull()) {
                BRepAdaptor_Surface adapt(face);
                if (adapt.GetType() == GeomAbs_Plane)
                    return true;
            }
        }
        return false;
    }
};

void TaskTransformedParameters::addReferenceSelectionGate(bool edge, bool face)
{
    Gui::Selection().addSelectionGate(new ReferenceSelection(getSupportObject(), edge, face));
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgTransformedParameters::TaskDlgTransformedParameters(ViewProviderTransformed *TransformedView_)
    : TaskDialog(), TransformedView(TransformedView_)
{
    assert(TransformedView);
    message = new TaskTransformedMessages(TransformedView);

    Content.push_back(message);
}

//==== calls from the TaskView ===============================================================

bool TaskDlgTransformedParameters::accept()
{
    std::string name = TransformedView->getObject()->getNameInDocument();

    try {
        //Gui::Command::openCommand(featureName + " changed");
        std::vector<App::DocumentObject*> originals = parameter->getOriginals();
        std::stringstream str;
        str << "App.ActiveDocument." << name.c_str() << ".Originals = [";
        for (std::vector<App::DocumentObject*>::const_iterator it = originals.begin(); it != originals.end(); ++it)
        {
            if ((*it) != NULL)
                str << "App.ActiveDocument." << (*it)->getNameInDocument() << ",";
        }
        str << "]";
        Gui::Command::runCommand(Gui::Command::Doc,str.str().c_str());
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(parameter, tr("Input error"), QString::fromAscii(e.what()));
        return false;
    }

    // Continue (usually in virtual method accept())
    return true;
}

bool TaskDlgTransformedParameters::reject()
{
    // Get object before view is invalidated
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(TransformedView->getObject());
    std::vector<App::DocumentObject*> pcOriginals = pcTransformed->Originals.getValues();

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");

    // if abort command deleted the object the originals are visible again
    if (!Gui::Application::Instance->getViewProvider(pcTransformed)) {
        for (std::vector<App::DocumentObject*>::const_iterator it = pcOriginals.begin(); it != pcOriginals.end(); ++it)
        {
            if (((*it) != NULL) && (Gui::Application::Instance->getViewProvider(*it) != NULL)) {
                Gui::Application::Instance->getViewProvider(*it)->show();
            }
        }
    }

    return true;
}



#include "moc_TaskTransformedParameters.cpp"

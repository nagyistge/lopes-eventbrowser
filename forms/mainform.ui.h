/****************************************************************************
** Ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include "Draw.h"
#include "Canvas.h"
#include "GraphInfos.h"
#include "Helper.h"
#include "formviewdata.h"
#include "CanvasCollection.h"
#include "formOpenFile.h"
#include "formAbout.h"

#include <qstatusbar.h>
#include <q3filedialog.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qobject.h>
#include <qregexp.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3VBoxLayout>
#include "TQtWidget.h"

#include <string>
#include <vector>
#include <sstream>
#include <stdio.h>

#ifdef DEBUG
#include <iostream>
#endif

#ifdef QT_NO_CAST_FROM_ASCII
#undef QT_NO_CAST_FROM_ASCII
#endif

#define NOT_IMPLEMENTED QMessageBox::information(this, \
                        "Message","Sorry, not implemented yet." );

using namespace std;


//----------------------------------------------------------------------

Draw * draw = new Draw(); //the Draw object for drawing stuff

//pointers for creating new tab with TQtWidget in it
QWidget * newTab; // pointer to newly created tab
TQtWidget * newTqtw; // pointer to newly created TQtWidget
Q3VBoxLayout * newLayout; // pointer for laying out TQtWidget properly

// filters for saving to files
static QString fileTypeFilterSingle = "Postscript (*.ps);;"
                                     "Root File (*.root);;"
                                     "Encapsulated Postscript (*.eps);;"
                                     "PDF (*.pdf);;"
                                     "JPEG (*.jpg);;"
                                     "GIF (*.gif);;"
                                     "C++ Macro (*.cxx)";
static QString fileTypeFilterMultiple = "Postscript (*.ps);;"
                                       "Encapsulated Postscript (*.eps)";


// keeping track of the corresponding tabnames in the listview
// and the widgetstack
vector<int> tabIds;
vector<Q3ListViewItem*> tabNames;
CanvasCollection* canvases = new CanvasCollection();

bool isInitialized = false;
bool isDrawingInNewTab = false; // keep track of when is drawing in new tab so
                                // that the old tab's information is not
                                // overrided; I know, it's a ugly hack ;P
// statusbar
QLabel* lblStatus;
QLabel* lblEventCutStatus;

//---------------------------------------------------------------------


void MainForm::init()
{
    //don't sort the listview - it messes up the order of inserting tabs
    lvGraphs->setSorting(-1);

    //populate the graph type drop-down list
    for (int i = 0; i < Canvas::numberOfGraphTypes(); ++i)
      cmbGraphType->addItem(QString::fromStdString(Canvas::getDescription(
                                       (Canvas::graphTypes) i)));
    
    // set up statusbar; important: don't attempt to write to status bar
    // before this!!!
    lblStatus = new QLabel("Ready",this);
    lblEventCutStatus = new QLabel("Current number of events: 00000",this);
    lblStatus->setMinimumSize(lblStatus->sizeHint());
    statusBar()->addWidget(lblStatus,1);
    statusBar()->addWidget(lblEventCutStatus);

    getSavedEventCuts();
    setMultigraphStatus (false);
    
    addNewTab();    
    isInitialized = fileOpen();
}

bool MainForm::fileOpen()
{
    formOpenFile f(this);
    f.initialize(draw->rootTree);
    
    if (f.exec())
        draw->rootTree = f.getReadRootTree();
    else if (!(draw->rootTree)) // for the first time when program opens
    {
        QMessageBox::information(this,"Open files",
                                 "You must select some root files to"
                                 "start with! Use File -> "
                                 "Open to open root files.");
         return false;
    }

    txtEventCut->setText("");
    applyRootCut();
    fillBranchNames();

    this->setCaption(QString::fromStdString("Browser - " +
                     joinStrings(draw->rootTree->getListOfFiles(),"; ")));
    //changeStatus("Opened files: " + joinStrings(filenames, ", "));
    return true;
}

void MainForm::importCanvases()
{
    QString file = Q3FileDialog::getOpenFileName("",
                    "Config files (*.cfg)",
                    this,
                    tr("Open file dialog"),
                    tr("Choose file that contains canvas definitions"));

    if (file.isEmpty()) return;

    CanvasCollection * cc = CanvasCollection::readFromFile(file.toStdString());

    if (!cc)
    {
        QMessageBox::warning(this,"File parse failed","Unable to parse the "
                             "canvas definition file. It maybe of a wrong "
                             "type or contains corrupted information.");
        return;
    }
    else
    {
        loadCanvasCollection(cc);
        QMessageBox::information(this,"Success",QString("%1 canvas(es) is "
                                 "loaded").arg(cc->size()));
    }
}

void MainForm::exportCanvases()
{
    QString file = Q3FileDialog::getSaveFileName(
                    "canvases.cfg",
                    "Config file (*.cfg)",
                    this,
                    tr("save file dialog"),
                    tr("Choose a filename to save under"));

    if (!file.isEmpty()) canvases->saveToFile(file.toStdString());
}

void MainForm::loadCanvasCollection(CanvasCollection* cc)
{
    if (!cc) return;
    removeAllTabs();
    ckbMultiGraph->setChecked(false);
    ckbNewTab->setChecked(false);

    canvases = cc;

    for (int i = 0; i < (int) canvases->size(); ++i)
        addNewTab(canvases->at(i),true);
    
}

void MainForm::fileSave()
{
    QString file = Q3FileDialog::getSaveFileName(
                   s(validateFilename(
                         tabNames[getTabIndex()]->text(0).toStdString())),
                    fileTypeFilterSingle,
                    this,
                    tr("save file dialog"),
                    tr("Choose a filename to save under"));


    if (!file.isEmpty()) draw->saveAs(NULL,file.ascii());
}


void MainForm::fileSaveAllSingle()
{
    int tabcount = (int) tabIds.size();
    TQtWidget * tqtw;

    if (tabcount < 1) return;

    QString file = Q3FileDialog::getSaveFileName("",
                    fileTypeFilterMultiple,
                    this,
                    tr("save file dialog"),
                    tr("Choose a filename to save under"));

    if (file.isEmpty()) return;

    //open for saving multiple canvas in one file
    draw->saveAs(NULL,file.ascii(),Draw::OPEN_ONLY); 
    
    for (int i = 0; i<tabcount ; i++)
    {
        tqtw = (TQtWidget*) wgStack->widget(tabIds[i])
                ->child("tqtw","TQtWidget", FALSE);
        if (tqtw)
        {
            draw->saveAs(tqtw->GetCanvas(),file.ascii());
        }
    }

    draw->saveAs(NULL,file.ascii(),Draw::CLOSE_ONLY);
}

void MainForm::fileSaveAllMultiple()
{
    int tabcount = (int) tabIds.size();
    TQtWidget * tqtw;
    bool ok;

    if (tabcount < 1) return;

    // get filename extension
    QString ext = QInputDialog::getItem(
            "File Type", "Choose a file type to save as: ",
            QStringList::split(";;",fileTypeFilterSingle),
            0, false, &ok, this);
    if (!ok) return;
    // search for the extension part in the string
    QRegExp regex("\\(\\*\\.(.+)\\)");
    regex.search(ext);
    ext = regex.cap(1);
        
    // get folder to save under
    QString folder = Q3FileDialog::getExistingDirectory("", this,
                    "Choose directory",
                    "Choose a directory to save files under", TRUE);
    if (folder.isEmpty()) return;

    // saving!
    for (int i = 0; i<tabcount ; i++)
    {
        tqtw = (TQtWidget*) wgStack->widget(tabIds[i])
                ->child("tqtw","TQtWidget", FALSE);
        if (tqtw)
        {
            string filename = folder.toStdString() + "/" + 
              validateFilename(tabNames[i]->text(0).toStdString())
              + "." + ext.toStdString();
            draw->saveAs(tqtw->GetCanvas(),filename.c_str());
        }
    }
}

void MainForm::fileExit()
{
    close();
}
void MainForm::filePrint()
{
    NOT_IMPLEMENTED
}

void MainForm::helpAbout()
{
    formAbout f(this);
    f.exec();
}

void MainForm::Draw()
{
    if (ckbNewTab->isChecked())
    {
        isDrawingInNewTab = true;
        addNewTab();
        isDrawingInNewTab = false;
    }

    saveToCanvas(); // first save the information into the canvas object
    
    if (!canvases->at(getTabIndex())->readyToDraw())
    {
        QMessageBox::information(this,"Message","Not enough information is "
                                 "given for drawing. Please check your input");
        return;
    }
    
    // calling the draw method of the canvas
    canvases->at(getTabIndex())->draw(draw);

    // update the user interface if needed
    loadCanvas(canvases->at(getTabIndex()));
}

void MainForm::applyRootCut()
{
    if (!draw->rootTree) return;
    
    draw->rootTree->setEventCut(txtEventCut->text().ascii());
    btnApplyCut->setEnabled(false);

    QString status  = QString("Current number of events: %1")
        .arg(draw->rootTree->getNumberEntries());

//    cout << status << endl;

    lblEventCutStatus->setText(status);
}

void MainForm::txtEventCut_changed()
{
    btnApplyCut->setEnabled(true);
}

void MainForm::addNewTab(Canvas* c, bool autoDraw)
{
    if (!c) return;
    
    int id = -1;
    Q3ListViewItem * item;
    
    newTab = new QWidget(wgStack,"tab_"); //todo add number!
    newTqtw = new TQtWidget(newTab,"tqtw");
    newLayout = new Q3VBoxLayout(newTab,0,6,"tabLayout");
    newLayout->addWidget(newTqtw);
    
    id = wgStack->addWidget(newTab);

    if (id != -1)
    {
        tabIds.push_back(id);

        //create an item after the last one in the listview
        item = new Q3ListViewItem(lvGraphs,
              (tabNames.size() == 0)? 0 : tabNames[(int)tabNames.size() - 1],
                                  s(c->getName()));
        tabNames.push_back(item);
        
        //focus on the newly created tab
        lvGraphs->setSelected(item,true);

        // make sure that the correct TQtWidget is the current Canvas
        setTabAsCanvas(newTab);

        loadCanvas(c);

        if (autoDraw) Draw();
    }
}    

void MainForm::addNewTab(Canvas* c)
{
    addNewTab(c,false);
}

void MainForm::addNewTab()
{
    Canvas *c = new Canvas();
    int index = getTabIndex();

    if (index >= 0) // make copy of the current information in the new Canvas
        saveToCanvas(c);
    else
        c->setGraphType(Canvas::GRAPH_2D); // default to 2D graph

    c->setName("Untitled Canvas");
    canvases->push_back(c);
    
    addNewTab(c);    
}

void MainForm::enableTabActionButtons()
{
    bool y = (getTabIndex() != -1);
    
    btnRemoveTab->setEnabled(y);
    btnRenameTab->setEnabled(y);
}


void MainForm::setTabAsCanvas( QWidget * qw )
{
    // find the TQtWidget belong to the tab page
    TQtWidget * tqtw = (TQtWidget*) qw->child("tqtw","TQtWidget", FALSE);
    // if found, set it as current canvas for drawing
    if (tqtw) draw->setCanvas(tqtw->GetCanvas());
}


// get the index in the arrays tabIds and tabNames for the currently selected
int MainForm::getTabIndex()
{
    return getTabIndex(wgStack->id(wgStack->visibleWidget()));
}


int MainForm::getTabIndex( int widgetID )
{
    int id = (widgetID == 0)? wgStack->id(wgStack->visibleWidget()) : widgetID;

    if (id == 0) return -1;

    for (int i = 0; i < (int)tabIds.size(); i++)
        if (tabIds[i] == id) return i;

    return -1;
}

int MainForm::getTabIndex( Q3ListViewItem * item )
{
    if (!item) return -1;
    
    // find the index for item corresponding to this QListViewItem
    for (int i = 0; i < (int)tabIds.size(); i++)
        if (tabNames[i] == item) return i;

    return -1;
}

void MainForm::selectTab( Q3ListViewItem * sel)
{
    if (!isDrawingInNewTab)
        saveToCanvas();

    // make sure that a new graph with axis is started
    ckbMultiGraph->setChecked(false); 
    
    int index = (sel)? getTabIndex(sel) : getTabIndex();
    int id;
//    int subpad;

    if (index < 0) //not a canvas; select subpad if applicable
    {
        if (!sel || !sel->parent()) return;

        index = getTabIndex(sel->parent());

        if (index < 0) return;

        // todo cd to the subpad

        return;
    }

    id = tabIds[index];
    
    if (id == 0)
        wgStack->raiseWidget(wgStack->id(wgEmtpyPage));
    else
        wgStack->raiseWidget(id);

    if (!isDrawingInNewTab)
        loadCanvas(canvases->at(index));
}

// load the inforamtion from the object Canvas to the user interface
void MainForm::loadCanvas(Canvas* c)
{
    if (!c) return;

    txtEventCut->setText(s(c->getEventCut()));
    if (isInitialized)
        applyRootCut();

    renameTab(s(c->getName()));
    wgsAction->raiseWidget(findGraphWidget(c->getGraphType()));
    cmbGraphType->setCurrentItem(c->getGraphType());
    
    switch (c->getGraphType())
    {
        case Canvas::GRAPH_2D:
        {
            infoGraph2D* info = (infoGraph2D*) c->getGraphInfo();

            ckb2DErrors->setChecked(info->useErrors);
            txt2DXAxis->setText(s(info->xAxis));
            txt2DYAxis->setText(s(info->yAxis));
            txt2DXAxisError->setText(s(info->xAxis_err));
            txt2DYAxisError->setText(s(info->yAxis_err));
            break;
        }
        case Canvas::SHOWER_ANGLE:
        {
            infoShowerAngle* info2 = (infoShowerAngle*) c->getGraphInfo();

            txtPolarThetaAxis->setText(s(info2->thetaAxis));
            txtPolarRAxis->setText(s(info2->rAxis));
            txtColorCode->setText(s(info2->colorCodeBy));
            break;
        }
        case Canvas::GRAPH_POLAR:
        {
            infoGraphPolar* i3 = (infoGraphPolar*) c->getGraphInfo();

            ckbPolarErrors->setChecked(i3->useErrors);
            txtPolarRAxis_2->setText(s(i3->rAxis));
            txtPolarThetaAxis_2->setText(s(i3->thetaAxis));
            txtPolarRAxisError->setText(s(i3->rAxis_err));
            txtPolarThetaAxisError->setText(s(i3->thetaAxis_err));
            break;
        }
        case Canvas::ANTENNA_POSITION:
        {
            cmbPositionType->setCurrentItem(
                ((infoGraphPosition*)c->getGraphInfo())->positionType);
            break;
        }
        case Canvas::HIST_1D:
        {
            infoHist1D* i4 = (infoHist1D*) c->getGraphInfo();

            ckbHist1DUseDefault->setChecked(i4->useDefault);
            txtHist1D_data->setText(s(i4->data));
            txtHist1D_xmin->setText(s(ftos(i4->min)));
            txtHist1D_xmax->setText(s(ftos(i4->max)));
            txtHist1D_nbins->setText(s(itos(i4->nbins)));
            break;
        }
        // ## add the case for new graph type here
        // case Canvas::_your new enum graph type enum name_:
        // {
        //     take the values from the infoGraph struct and fill in the user
        //     interface like above
        //     break;
        // }
        default:
        {
            cout << "hmm, this should not happen..." << endl;
        }
    }
            
}

void MainForm::saveToCanvas()
{
    int index = getTabIndex();
    if (index < 0) return;

    Canvas* c = canvases->at(index);

    if (c) saveToCanvas(c);
}

// save the current text fields, such as event cut, etc.  to the object Canvas
void MainForm::saveToCanvas(Canvas* c)
{
    if (!c) return;

    c->setEventCut(txtEventCut->text().ascii());
    c->setName(tabNames[getTabIndex()]->text(0).toStdString());

    if (wgsAction->visibleWidget() == tabPosition)
    {
        c->setGraphType(Canvas::ANTENNA_POSITION);
        infoGraphPosition* info0 = (infoGraphPosition*) c->getGraphInfo();
        info0->positionType = (infoGraphPosition::positionTypes)
                                   cmbPositionType->currentItem();
    }
    else if (wgsAction->visibleWidget() == tabGraph2D)
    {
        c->setGraphType(Canvas::GRAPH_2D);

        infoGraph2D* info = (infoGraph2D*) c->getGraphInfo();
        info->useErrors = ckb2DErrors->isChecked();
        info->xAxis = txt2DXAxis->text().ascii();
        info->yAxis = txt2DYAxis->text().ascii();
        info->xAxis_err = txt2DXAxisError->text().ascii();
        info->yAxis_err = txt2DYAxisError->text().ascii();
    }
    else if (wgsAction->visibleWidget() == tabShowerAngles)
    {
        c->setGraphType(Canvas::SHOWER_ANGLE);
        
        infoShowerAngle* info2 = (infoShowerAngle*) c->getGraphInfo();
        info2->rAxis = txtPolarRAxis->text().ascii();
        info2->thetaAxis = txtPolarThetaAxis->text().ascii();
        info2->colorCodeBy = txtColorCode->text().ascii();
    }
    else if (wgsAction->visibleWidget() == tabGraphPolar)
    {
        c->setGraphType(Canvas::GRAPH_POLAR);

        infoGraphPolar* i3 = (infoGraphPolar*) c->getGraphInfo();
        i3->useErrors = ckbPolarErrors->isChecked();
        i3->rAxis = txtPolarRAxis_2->text().ascii();
        i3->thetaAxis = txtPolarThetaAxis_2->text().ascii();
        i3->rAxis_err = txtPolarRAxisError->text().ascii();
        i3->thetaAxis_err = txtPolarThetaAxisError->text().ascii();
    }
    else if (wgsAction->visibleWidget() == tabHist1D)
    {
        c->setGraphType(Canvas::HIST_1D);

        infoHist1D* i4 = (infoHist1D*) c->getGraphInfo();
        i4->useDefault = ckbHist1DUseDefault->isChecked();
        i4->data = txtHist1D_data->text().ascii();
        i4->min = atof(txtHist1D_xmin->text().ascii());
        i4->max = atof(txtHist1D_xmax->text().ascii());
        i4->nbins = atoi(txtHist1D_nbins->text().ascii());
    }
// ## uncomment below to add another graph type ##
//  else if (wgsAction->visibleWidget() == _the "tab page" widget you created_)
//  {
//      c->setGraphType(Canvas::_your enum member in Canvas.h_);
//      // take information from the user interface to the infoGraph struct
//      // like above
//  }
}

void MainForm::removeTab()
{
    int index = getTabIndex();
    if (index > -1)
        removeTab(index);
}

void MainForm::removeTab( int index )
{
    if (index < 0 || index > (int) tabIds.size()) return;

    // the new index to be selected
    int s_index = ((index == (int)tabIds.size() - 1)?
                   (index - 1) : (index + 1));

     // select the tab above
    if (s_index > 0)
        selectTab(tabNames[s_index]); 

    delete tabNames[index]; // free memory

    // delete entries in all three appropriate arrays
    tabIds.erase(tabIds.begin() +  index);
    tabNames.erase(tabNames.begin() + index);
    canvases->removeCanvas(index);
}

void MainForm::removeAllTabs()
{
    while (tabIds.size() > 0)
        removeTab(0);
}

// rename the current tab, asking the user for the new name
void MainForm::renameTab()
{
    int index = getTabIndex();
    if (index > -1)
        renameTab(index);
}

// rename the tab of index "index", asking user for the new name
void MainForm::renameTab(int index)
{
    if (index < 0 || index > (int) tabIds.size()) return;

    bool ok;
    
    QString name = QInputDialog::getText(tr("Rename Canvas"),
                                         tr("Enter a new name:"),
                                         QLineEdit::Normal,
                                         tabNames[index]->text(0),
                                         &ok,
                                         this);
    if (ok && !name.isEmpty())
        renameTab(index,name);
}

// rename the current tab to "name"
void MainForm::renameTab(QString name)
{
    int index = getTabIndex();
    if (index > -1)
        renameTab(index,name);
}

// rename the tab of index "index" to "name"
void MainForm::renameTab(int index, QString name)
{
    if (index < 0 || index > (int) tabIds.size()) return;

//    QListViewItem * item = tabNames[index];
        
    if (tabNames[index])
      tabNames[index]->setText(0,name);
}

void MainForm::divideCanvas()
{
    bool ok;
    
    QString s = QInputDialog::getText(tr("Divide Canvas"),
                                      tr("How to divide? e.g. 3x2"),
                                      QLineEdit::Normal,
                                      "3x2",
                                      &ok,
                                      this);
    if (ok && !s.isEmpty())
    {
        int n_columns, n_rows;
        sscanf(s.ascii(),"%ix%i",&n_columns,&n_rows);
        divideCanvas(n_columns,n_rows);
    }    
}

void MainForm::divideCanvas(int n_columns, int n_rows)
{
    draw->divideCanvas(n_columns,n_rows);
    addNewSubPad(n_rows * n_columns);
}

void MainForm::addNewSubPad( int n_subpad )
{
    Q3ListViewItem * parent_item = tabNames[getTabIndex()];

    if (!parent_item) return;
    
    Q3ListViewItem * child_item, *after_item;
    ostringstream ss;

    // find the last child under the parent_item
    after_item = parent_item->firstChild();
    if (after_item)
    {
        while (after_item->nextSibling() != 0)
            after_item = after_item->nextSibling();
    }
    
    // rename the new subpads to Pad 1, Pad 2, etc.
    for (int i = 1; i <= n_subpad; i++)
    {
        ss.str("");
        ss << "Pad " << i;
        
        if (after_item)
          child_item = new Q3ListViewItem(parent_item,after_item,s(ss.str()));
        else
          child_item = new Q3ListViewItem(parent_item,s(ss.str()));

        after_item = child_item;
    }

    parent_item->setOpen(true);
}

void MainForm::clearGraph()
{
    draw->clearCanvas();
}

void MainForm::setMultigraphStatus( bool state )
{
    draw->setMultiGraph(state);
}

void MainForm::saveEventCut()
{
    if (txtEventCut->text().isEmpty()) return;
    
    cmbEventCuts->insertItem(txtEventCut->text());
    appendLine(EVENTCUTS_FILE, txtEventCut->text().toStdString());
}

void MainForm::changeStatus(QString status)
{
    lblStatus->setText(status);
}

QWidget* MainForm::findGraphWidget(Canvas::graphTypes type)
{
    switch (type)
    {
        case Canvas::GRAPH_2D:           return tabGraph2D;
        case Canvas::SHOWER_ANGLE:       return tabShowerAngles;
        case Canvas::ANTENNA_POSITION:   return tabPosition;
        case Canvas::GRAPH_POLAR:        return tabGraphPolar;
        case Canvas::HIST_1D:            return tabHist1D;
       // ## add case for new graph type to return your new "tab" widget; see
       // example above
        default:                         return tabUnknown;
    }
}

void MainForm::selectGraphType( int index )
{
    Canvas::graphTypes type = (Canvas::graphTypes) index;
    
    QWidget* w = findGraphWidget(type);

    if (w)
        wgsAction->raiseWidget(w);
}

void MainForm::viewData()
{
    static formViewData* f;

    if (!f)
        f = new formViewData(this);
    
    f->initialize(draw->rootTree);
    f->show();
    f->raise();
    f->setActiveWindow();
}

void MainForm::fillBranchNames()
{
    if (!draw->rootTree) return;

    cmbBranchNames->clear();
    
    vector<string> names = draw->rootTree->getBranchNames();
    for (int i = 0; i < (int) names.size(); i++)
      cmbBranchNames->addItem(s(names[i]));
}
void MainForm::getSavedEventCuts()
{
    vector<string> files;
    cmbEventCuts->clear();
    
    getLines(EVENTCUTS_FILE,files);

    for (int i = 0; i < (int) files.size(); ++i)
      cmbEventCuts->addItem(s(files[i]));
}
void MainForm::insertBranchName( const QString& name )
{
    // find the QLineEdit or QTextEdit that has focus and append the branch
    // name to it; the find method relies on the fact that all the textboxes
    // are named with prefix txt
  /* TODO
  QObjectList *l = topLevelWidget()->queryList(NULL,"txt*");
    QObjectListIt it( *l );
    QWidget* w;
        
    while ((w = (QWidget*) it.current()) != 0 )
    {
        ++it;
        if (w->hasFocus())
        {
            if (dynamic_cast<QLineEdit*>(w) != 0)
            {
                ((QLineEdit*)w)->setText(((QLineEdit*)w)->text() + name);
                break;
            }

            if ((Q3TextEdit*)w != 0)
            {
                ((Q3TextEdit*)w)->setText(((Q3TextEdit*)w)->text() + name);
                break;
            }
        }
    }

    delete l; */
}

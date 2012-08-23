# used to parse files more easily
from __future__ import with_statement

#numerical computations
import numpy as np

# for command-line arguments
import sys
import os
os.environ['NUMPTHREADS'] = '1'
import sys
import subprocess
from datetime import date

#moose imports
import moose
import moose.utils as mooseUtils
from collections import defaultdict
from objectedit import ObjectFieldsModel, ObjectEditView
from moosehandler import MooseHandler
from mooseplot import MoosePlot,MoosePlotWindow,newPlotSubWindow

import kineticlayout
#import kl 
from neuralLayout import *

from filepaths import *
import defaults

# Qt4 bindings for Qt
from PyQt4 import QtCore,QtGui
from PyQt4.QtCore import QEvent, Qt

# import the MainWindow widget from the converted .ui (pyuic4) files
from newgui import Ui_MainWindow

import config


class DesignerMainWindow(QtGui.QMainWindow, Ui_MainWindow):
    """Customization for Qt Designer created window"""
    def __init__(self, interpreter=None,parent = None):
        # initialization of the superclass
        super(DesignerMainWindow, self).__init__(parent)
        # setup the GUI --> function generated by pyuic4
        self.objFieldEditorMap = {}
        self.setupUi(self)
        self.setCorner(Qt.BottomRightCorner,Qt.RightDockWidgetArea)
        self.setCorner(Qt.BottomLeftCorner,Qt.LeftDockWidgetArea)
        self.mooseHandler = MooseHandler()
        #self.mdiArea.setBackground(QtGui.QBrush(QtGui.QImage(os.path.join(config.KEY_ICON_DIR,'QMdiBackground.png'))))

        #other variables
        self.currentTime = 0.0
        self.allCompartments = []
        self.allIntFires = []
        self.modelHasCompartments = False
        self.modelHasIntFires = False
        self.modelHasLeakyIaF = False
        self.modelPathsModelTypeDict = {}
        self.activeWindow = 'None'
        #prop Editor variables
        self.propEditorCurrentSelection = None
        self.propEditorChildrenIdDict = {}

        #plot config variables
        self.plotConfigCurrentSelection = None
        self.plotConfigAcceptPushButton.setEnabled(False)
        self.plotWindowFieldTableDict = {} #guiPlotWinowName:[mooseTable]
        self.plotNameWinDict = {} #guiPlotWindowName:moosePlotWindow

        self.defaultDockState()
#        self.resizeCentralWidgets()
        #connections
        self.connectElements()

        self.defaultKKITSolver = 'rk5'


#    def resizeCentralWidgets(self):
#        widthOfEach =  int((self.layoutWidget.width()+self.plotMdiArea.width())/2)
#        self.layoutWidget.resize(widthOfEach, self.layoutWidget.height())
#        self.plotMdiArea.resize(widthOfEach, self.layoutWidget.height())

    def setAllToStartState(self):
        self.currentTime = 0.0
        self.allCompartments = []
        self.allIntFires = []
        self.modelHasCompartments = False
        self.modelHasIntFires = False
        self.modelHasLeakyIaF = False
        self.activeWindow = 'None'
        self.propEditorCurrentSelection = None
        self.propEditorChildrenIdDict = {}
        self.plotConfigCurrentSelection = None
        self.plotConfigAcceptPushButton.setEnabled(False)
        self.plotWindowFieldTableDict = {} #guiPlotWinowName:[mooseTable]
        self.plotNameWinDict = {} #guiPlotWindowName:moosePlotWindow
        print 'reset all to start state'

    def defaultDockState(self):
        #this will eventually change corresponding to the "mode" of operation - Edit/Plot/Run
        self.moosePopulationEditDock.setVisible(False)
        self.mooseLibraryDock.setVisible(False)
        self.mooseConnectDock.setVisible(False)
        self.mooseShellDockWidget.setVisible(False)
        self.menuHelp.setVisible(False)
        self.menuHelp.setEnabled(False)
        self.menuView.setEnabled(True)
        self.menuClasses.setEnabled(False)
        self.menuClasses.setVisible(False)

        checked = False
        self.propEditorChildListWidget.setVisible(checked)
        self.label_13.setVisible(checked)
        self.label_14.setVisible(checked)
        self.label_15.setVisible(checked)
        self.simControlPlotdtLineEdit.setVisible(checked)
        self.simControlSimdtLineEdit.setVisible(checked)
        self.simControlUpdatePlotdtLineEdit.setVisible(checked)

    def connectElements(self):
        #gui connections
        self.connect(self.actionLoad_Model,QtCore.SIGNAL('triggered()'), self.popupLoadModelDialog)
        self.connect(self.actionQuit,QtCore.SIGNAL('triggered()'),self.doQuit)
        #self.connect(self.mdiArea,QtCore.SIGNAL('subWindowActivated(QMdiSubWindow)'),self.plotConfigCurrentPlotWinChanged)
        #plotdock connections
        self.connect(self.plotConfigAcceptPushButton,QtCore.SIGNAL('pressed()'),self.addFieldToPlot)
        self.connect(self.plotConfigNewWindowPushButton,QtCore.SIGNAL('pressed()'),self.plotConfigAddNewPlotWindow)
        #self.connect(self.plotConfigWinSelectionComboBox,QtCore.SIGNAL('currentIndexChanged(int)'),self.activateSubWindow)
        self.connect(self.plotConfigDockWidget,QtCore.SIGNAL('visibilityChanged(bool)'),self.actionPlot_Config.setChecked)
        #propEditor connections
        self.connect(self.propEditorSelParentPushButton,QtCore.SIGNAL('pressed()'),self.propEditorSelectParent)
        self.connect(self.propEditorChildListWidget,QtCore.SIGNAL('itemClicked(QListWidgetItem *)'),self.propEditorSelectChild)
        self.connect(self.mooseObjectEditDock,QtCore.SIGNAL('visibilityChanged(bool)'),self.actionProperty_Editor.setChecked)
        self.connect(self.propEditorSeeChildrenPushButton,QtCore.SIGNAL('pressed()'),self.propEditorChildListToggleVisibility)
        #internal connections
        self.connect(self.mooseHandler, QtCore.SIGNAL('updatePlots(float)'), self.updatePlots)
        #run
        #self.connect(self.simControlRunPushButton,QtCore.SIGNAL('clicked()'),self.activeMdiWindow)
        self.connect(self.simControlRunPushButton, QtCore.SIGNAL('clicked()'), self._resetAndRunSlot)
        self.connect(self.simControlAdvancedPushButton,QtCore.SIGNAL('clicked()'),self.simControlToggleAdvanced)
        self.connect(self.simControlAdvancedToolButton,QtCore.SIGNAL('clicked()'),self.simControlToggleAdvanced)

        #self.connect(self.actionRun,QtCore.SIGNAL('triggered()'),self.activeMdiWindow)
        self.connect(self.actionRun,QtCore.SIGNAL('triggered()'),self._resetAndRunSlot)
        self.connect(self.simControlDockWidget,QtCore.SIGNAL('visibilityChanged(bool)'),self.actionSimulation_Control.setChecked)

        #self.connect(self.simControlContinuePushButton, QtCore.SIGNAL('clicked()'), self._continueSlot)
        #self.connect(self.actionContinue,QtCore.SIGNAL('triggered()'),self._continueSlot)
        self.connect(self.simControlResetPushButton, QtCore.SIGNAL('clicked()'), self._resetSlot)
        self.connect(self.actionReset,QtCore.SIGNAL('triggered()'),self._resetSlot)
        self.connect(self.actionStop,QtCore.SIGNAL('triggered()'),self._stopSlot)
        self.connect(self.simControlStopPushButton, QtCore.SIGNAL('clicked()'),self._stopSlot)

        #self.connect(self.actionMoose_Shell,QtCore.SIGNAL('triggered(bool)'),self.toggleMooseShellDockVisibility)
        self.connect(self.actionProperty_Editor,QtCore.SIGNAL('triggered(bool)'),self.togglePropEditorDockVisibility)
        self.connect(self.actionPlot_Config,QtCore.SIGNAL('triggered(bool)'),self.togglePlotConfigDockVisibility)
        self.connect(self.actionSimulation_Control,QtCore.SIGNAL('triggered(bool)'),self.toggleSimControlDoctVisibility)
        self.connect(self.actionViewAsTabs,QtCore.SIGNAL('triggered(bool)'),self.switchTabbedView)
        self.connect(self.actionViewAsSubWindows,QtCore.SIGNAL('triggered(bool)'),self.switchSubWindowView) 
        #solver
        self.connect(self.actionRK5,QtCore.SIGNAL('triggered()'), self.changeToRK5)
        #self.connect(self.actionEE, QtCore.SIGNAL('triggered()'), self.changeToEE)
        self.connect(self.actionGillespie,QtCore.SIGNAL('triggered()'), self.changeToGill)

    def changeToRK5(self):
        print 'Changing to RK5'
        #self.actionEE.setChecked(False)
        self.actionGillespie.setChecked(False)
        self.actionRK5.setChecked(True)
        #for model in self.modelPathsModelTypeDict.keys():
        #    moose.element(model).method = 'rk5'
        #moose.reinit()
        #self._resetSlot()
        self.defaultKKITSolver = 'rk5'
            
    #def changeToEE(self):
    #    print 'Changing to EE'
    #    self.actionRK5.setChecked(False)
    #    self.actionGillespie.setChecked(False)
    #    self.actionEE.setChecked(True)
        #for model in self.modelPathsModelTypeDict.keys():
        #    moose.element(model).method = 'ee'
        #moose.reinit()
        #self._resetSlot()
    #    self.defaultKKITSolver = 'ee'

    def changeToGill(self):
        print 'Changing to Gillespie'
        self.actionRK5.setChecked(False)
        #self.actionEE.setChecked(False)
        self.actionGillespie.setChecked(True)
        #for model in self.modelPathsModelTypeDict.keys():
        #    moose.element(model).method = 'gssa'
        #moose.reinit()
        #self._resetSlot()
        self.defaultKKITSolver = 'gssa'

    def propEditorChildListToggleVisibility(self):
        checked = not(self.propEditorChildListWidget.isVisible())
        self.propEditorChildListWidget.setVisible(checked)
        if checked:
            self.propEditorSeeChildrenPushButton.setText('Hide Children')
        else:
            self.propEditorSeeChildrenPushButton.setText('See  Children')

    def simControlToggleAdvanced(self):
        checked = not(self.label_13.isVisible())
        self.label_13.setVisible(checked)
        self.label_14.setVisible(checked)
        self.label_15.setVisible(checked)
        self.simControlPlotdtLineEdit.setVisible(checked)
        self.simControlSimdtLineEdit.setVisible(checked)
        self.simControlUpdatePlotdtLineEdit.setVisible(checked)
        if checked:
            self.simControlAdvancedToolButton.setText('>')
        else:
            self.simControlAdvancedToolButton.setText('V')

    def toggleMooseShellDockVisibility(self, checked):
        self.mooseShellDockWidget.setVisible(checked)

    def togglePropEditorDockVisibility(self, checked):
        self.mooseObjectEditDock.setVisible(checked)
            
    def togglePlotConfigDockVisibility(self, checked):
        self.plotConfigDockWidget.setVisible(checked)

    def toggleSimControlDoctVisibility(self, checked):
        self.simControlDockWidget.setVisible(checked)

    def switchSubWindowView(self, checked):
        self.actionViewAsTabs.setChecked(not checked)
        self.actionViewAsSubWindows.setChecked(checked)
        if checked:
            self.mdiArea.setViewMode(self.mdiArea.SubWindowView)
            self.mdiArea.cascadeSubWindows()
        else:
            self.mdiArea.setViewMode(self.mdiArea.TabbedView)

    def switchTabbedView(self, checked):
        self.actionViewAsTabs.setChecked(checked)
        self.actionViewAsSubWindows.setChecked(not checked)
        if checked:
            self.mdiArea.setViewMode(self.mdiArea.TabbedView)

        else:
            self.mdiArea.setViewMode(self.mdiArea.SubWindowView)
            self.mdiArea.casadeSubWindows()

#    def activateSubWindow(self,number):
#        allList = self.mdiArea.subWindowList()
#        self.activeWindow = allList[number+1]
#        self.activeMdiWindow()

    def popupLoadModelDialog(self):
        fileDialog = QtGui.QFileDialog(self)
        fileDialog.setFileMode(QtGui.QFileDialog.ExistingFile)
        fileDialog.setToolTip("<font color='white'> Select a model Neural / KKit to open. Try Mitral.g / Kholodenko.g from DEMOS> mitral-ee / kholodenko folders </font>")        
        ffilter =''
        for key in sorted(self.mooseHandler.fileExtensionMap.keys()):
            ffilter = ffilter + key + ';;'
        ffilter = ffilter[:-2]
        fileDialog.setFilter(self.tr(ffilter))
        fileDialog.setWindowTitle('Open File')

        targetPanel = QtGui.QFrame(fileDialog)
        targetPanel.setLayout(QtGui.QVBoxLayout())

        currentPath = self.mooseHandler._current_element.path
        
        targetLabel = QtGui.QLabel('Target Element')
        targetText = QtGui.QLineEdit(fileDialog)
        
        targetText.setText(currentPath)
        targetText.setText('/')
        targetText.setReadOnly(1)
        targetPanel.layout().addWidget(targetLabel)
        targetPanel.layout().addWidget(targetText)
        layout = fileDialog.layout()
        layout.addWidget(targetPanel)
        self.connect(fileDialog, QtCore.SIGNAL('currentChanged(const QString &)'), lambda path:(targetText.setText(os.path.basename(str(path)).rpartition('.')[0])) )
        #self.connect(targetText, QtCore.SIGNAL('textChanged(const QString &)'), self.getElementpath)

        if fileDialog.exec_():
            if self.modelPathsModelTypeDict.keys():
                self.setAllToStartState()
                while self.mdiArea.currentSubWindow():
                    self.mdiArea.removeSubWindow(self.mdiArea.currentSubWindow())
                #self.objFieldEditModel.mooseObject = None
                self.mooseHandler.clearPreviousModel(self.modelPathsModelTypeDict)

            fileNames = fileDialog.selectedFiles()
            fileFilter = fileDialog.selectedFilter()
            fileType = self.mooseHandler.fileExtensionMap[str(fileFilter)]

            if fileType == self.mooseHandler.type_all:
                reMap = str(fileNames[0]).rsplit('.')[1]
                if reMap == 'g':
                    fileType = self.mooseHandler.type_genesis
                elif reMap == 'py':
                    fileType = self.mooseHandler.type_python
                else:
                    fileType = self.mooseHandler.type_xml

            directory = fileDialog.directory()
            self.statusBar.showMessage('Loading model, please wait')
            app = QtGui.qApp
            app.setOverrideCursor(QtGui.QCursor(Qt.BusyCursor)) #shows a hourglass - or a busy/working arrow
            for fileName in fileNames:
                if(((str(targetText.text())) == '/') or ((str(targetText.text())) == ' ') ):
                    modelpath = os.path.basename(str(fileName)).rpartition('.')[0] #case when users delibaretly want to load model under / or leave blank
                    
                else:
                    #modelpath = str(targetText.text()).partition('.')[-1]
                    modelpath = str(targetText.text())
                modeltype = self.mooseHandler.loadModel(str(fileName), str(fileType), modelpath, self.defaultKKITSolver)
                if modeltype == MooseHandler.type_kkit:
                    modelpath = '/KKIT/'+modelpath
                    #self.menuSolver.setEnabled(1)
                    try:
                        self.addKKITLayoutWindow(modelpath)
                        #self.actionLoad_Model.setEnabled(0) #to prevent multiple loads
                        
                    except kineticlayout.Widgetvisibility:
                    #except kl.Widgetvisibility:
                        print 'No kkit layout for: %s' % (str(fileName))
                    #print moose.element(modelpath).getField('method')
                    self.populateKKitPlots(modelpath)
                #else:
                    #self.menuSolver.setEnabled(0)
                #print modelpath,modeltype,'hello'
                self.modelPathsModelTypeDict[modelpath] = modeltype
                self.populateDataPlots(modelpath)
                self.updateDefaultTimes(modeltype,modelpath)
            #self.enableControlButtons()
            
            self.checkModelForNeurons()
            if self.modelHasCompartments or self.modelHasIntFires or self.modelHasLeakyIaF:
                self.addGLWindow()
            self.assignClocks(modelpath,modeltype)
            print 'Loaded model',  fileName, 'of type', modeltype, 'under Element path ',modelpath
            app.restoreOverrideCursor()

    def assignClocks(self,modelpath,modeltype):
        if modeltype == MooseHandler.type_kkit:
            #self.mooseHandler.updateClocks(MooseHandler.DEFAULT_SIMDT_KKIT, MooseHandler.DEFAULT_PLOTDT_KKIT)
            #script auto asigns clocks!
            pass
        elif modeltype == MooseHandler.type_neuroml:
            #self.mooseHandler.updateClocks(MooseHandler.DEFAULT_SIMDT, MooseHandler.DEFAULT_PLOTDT)
            #use Aditya's method to assign clocks - also reinits!
            mooseUtils.resetSim(['/cells','/elec'], MooseHandler.DEFAULT_SIMDT, MooseHandler.DEFAULT_PLOTDT)

        elif modeltype == MooseHandler.type_python:
            #specific for the hopfield tutorial!
            #self.mooseHandler.updateClocks(MooseHandler.DEFAULT_SIMDT, MooseHandler.DEFAULT_PLOTDT)
            clock = 1e-4
            self.simControlSimdtLineEdit.setText(str(clock))
            self.simControlPlotdtLineEdit.setText(str(clock))
            self.mooseHandler.updateClocks(clock, clock) #simdt,plotdt
            moose.useClock(1, '/hopfield/##[TYPE=IntFire]', 'process')
            moose.useClock(2, '/hopfield/##[TYPE=PulseGen]', 'process')
            moose.useClock(2, '/hopfield/##[TYPE=SpikeGen]', 'process')
            moose.useClock(4, '/hopfield/##[TYPE=Table]', 'process') 
        else:
            print 'Clocks have not been assigned! - GUI does not supported this format yet'

    def checkModelForNeurons(self):
        self.allCompartments = mooseUtils.findAllBut('/##[TYPE=Compartment]','library')
        self.allIntFires = mooseUtils.findAllBut('/##[TYPE=IntFire]','library')
        self.allLeakyIaF = mooseUtils.findAllBut('/##[TYPE=LeakyIaF]','library')
        if self.allCompartments:
            self.modelHasCompartments = True
        if self.allIntFires:
            self.modelHasIntFires = True
        if self.allLeakyIaF:
            self.modelHasLeakyIaF = True

    def addGLWindow(self):
	vizWindow = newGLSubWindow()
        vizWindow.setWindowTitle("GL Window")
        vizWindow.setObjectName("GLWindow")
        self.mdiArea.addSubWindow(vizWindow)
        self.mdiArea.setActiveSubWindow(vizWindow)
        self.layoutWidget = updatepaintGL(parent=vizWindow)
        self.layoutWidget.setObjectName('Layout')
        vizWindow.setWidget(self.layoutWidget)

        QtCore.QObject.connect(self.layoutWidget,QtCore.SIGNAL('compartmentSelected(QString)'),self.pickCompartment)
        self.updateCanvas()

    def pickCompartment(self, name):
        self.makeObjectFieldEditor(moose.element(str(name)))

    def updateVisualization(self):
        self.layoutWidget.updateViz()

    def updateCanvas(self):
        cellNameComptDict = {}
        self.layoutWidget.viz = 1 #not the brightest way to go about
        if self.modelHasCompartments:
            for compartment in self.allCompartments: 
                cellName = compartment.parent[0].path #enforcing a hierarchy /cell/compartment! - not a good idea 
                if cellName in cellNameComptDict:
                    cellNameComptDict[cellName].append(compartment)
                else:
                    cellNameComptDict[cellName] = [compartment]
            for cellName in cellNameComptDict:
                self.layoutWidget.drawNewCell(cellName)
            self.layoutWidget.updateGL()
            self.layoutWidget.setColorMap()
#        elif self.modelHasLeakyIaF:
#            print 'modelHasIntFires:', len(self.allLeakyIaF)
        elif self.modelHasIntFires or self.modelHasLeakyIaF:
            #print 'modelHasIntFires:', len(self.allIntFires)
            #sideSquare = self.nearestSquare(len(self.allIntFires))
            sideSquare = self.nearestSquare(len(self.allLeakyIaF))
            #for intFire in self.allIntFires:
            intFireCellNumber = 1 
            #self.layoutWidget.drawNewCell(cellName='/cells/LIFs_'+str(intFireCellNumber-1), style=3, cellCentre=[0.1, 0.1, 0.0])
            for yAxis in range(sideSquare):
                for xAxis in range(sideSquare):
                    print 'we are here'
                    self.layoutWidget.drawNewCell(cellName='/cells/LIFs_'+str(intFireCellNumber-1), style=3, cellCentre=[xAxis*0.5,yAxis*0.5,0.0])
                    intFireCellNumber += 1
            self.layoutWidget.updateGL()
            self.layoutWidget.setColorMap(vizMinVal=-0.07,vizMaxVal=-0.04)
#setColorMap(self,vizMinVal=-0.1,vizMaxVal=0.07,moosepath='',variable='Vm',cMap='jet')
                
    def nearestSquare(self, n):	#add_chait
    	i = 1
	while i * i < n:
            i += 1
	return i

    def getElementpath(self,text):
        #print "here",text
        if(text == '/' or text == ''):
            QtGui.QMessageBox.about(self,"My message box","Target Element path should not be \'/\' or \'null\', filename will be selected as Element path")


    def resizeEvent(self, event):
        QtGui.QWidget.resizeEvent(self, event)

    def addKKITLayoutWindow(self,modelpath):
        centralWindowsize =  self.mdiArea.size()
        self.sceneLayout = kineticlayout.KineticsWidget(centralWindowsize,modelpath,self.mdiArea)

        #self.sceneLayout = kl.KineticsWidget(centralWindowsize,modelpath,self.mdiArea)
        self.connect(self.sceneLayout, QtCore.SIGNAL("itemDoubleClicked(PyQt_PyObject)"), self.makeObjectFieldEditor)
        KKitWindow = self.mdiArea.addSubWindow(self.sceneLayout)
        KKitWindow.setWindowTitle("KKit Layout")
        KKitWindow.setObjectName("KKitLayout")
        self.activeWindow = KKitWindow
        self.sceneLayout.show()
        # centralWindowsize =  self.layoutWidget.size()
        # layout = QtGui.QHBoxLayout(self.layoutWidget)
        # self.sceneLayout = kineticlayout.kineticsWidget(centralWindowsize,modelpath,self.layoutWidget)
        # self.sceneLayout.setSizePolicy(QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding,QtGui.QSizePolicy.Expanding))
        # self.connect(self.sceneLayout, QtCore.SIGNAL("itemDoubleClicked(PyQt_PyObject)"), self.makeObjectFieldEditor)
        # layout.addWidget(self.sceneLayout)
        # self.layoutWidget.setLayout(layout)
        # self.sceneLayout.show()

        #property editor dock related

    def refreshObjectEditor(self,item,number):
        self.makeObjectFieldEditor(item.getMooseObject())

    def makeObjectFieldEditor(self, obj):
        self.propEditorChildren(obj)
        self.propEditorSelectionNameLabel.setText(str(obj.getField('name')))
        if obj.class_ == 'Shell' or obj.class_ == 'PyMooseContext' or obj.class_ == 'GenesisParser':
            print '%s of class %s is a system object and not to be edited in object editor.' % (obj.path, obj.class_)
            return
        self.propEditorCurrentSelection = obj
        try:
            self.objFieldEditModel = self.objFieldEditorMap[obj.getId()]
        except KeyError:
            self.objFieldEditModel = ObjectFieldsModel(obj)
            self.objFieldEditorMap[obj.getId()] = self.objFieldEditModel

        self.mTable.setObjectName(str(obj.getId()))
        self.mTable.setModel(self.objFieldEditModel)
        if hasattr(self, 'sceneLayout'):
            self.connect(self.objFieldEditModel,QtCore.SIGNAL('objectNameChanged(PyQt_PyObject)'),self.sceneLayout.updateItemSlot)
        self.updatePlotDockFields(obj)


    def propEditorSelectParent(self):
        if self.propEditorCurrentSelection:
            self.makeObjectFieldEditor(self.propEditorCurrentSelection.getField('parent'))

    def propEditorSelectChild(self,item):
        formattedText = str(item.text()).lstrip('    ')
        self.makeObjectFieldEditor(moose.Neutral(self.propEditorChildrenIdDict[formattedText]))
        
    def propEditorChildren(self,obj):
        allChildren = obj.getField('children')
        self.propEditorChildrenIdDict = {}
        self.propEditorChildListWidget.clear()

#        self.propEditorChildListWidget.addItem(obj.getField('name'))
#        self.propEditorChildrenIdDict[obj.getField('name')] = obj.getId()

        for child in allChildren:
            self.propEditorChildrenIdDict[moose.Neutral(child).getField('name')] = child
            self.propEditorChildListWidget.addItem('    '+moose.Neutral(child).getField('name'))

        #plot config dock related 
    def updatePlots(self,currentTime):
        #updates plots every update_plot_dt time steps see moosehandler.
        for plotWinName,plotWin in self.plotNameWinDict.iteritems():
            plotWin.plot.updatePlot(currentTime)
        self.updateCurrentTime(currentTime)
        if self.modelHasCompartments or self.modelHasIntFires or self.modelHasLeakyIaF:
            self.updateVisualization()
        QtCore.QCoreApplication.processEvents() 
                     
    def updatePlotDockFields(self,obj):
        #add plot-able elements according to predefined  ('%.3f' %self.currentTime)
        self.plotConfigCurrentSelectionLabel.setText(str(obj.getField('name')+'-'+obj.getField('class')))
        fieldType = obj.getField('class')
#        self.plotConfigCurrentSelectionTypeLabel.setText(fieldType)
        self.plotConfigFieldSelectionComboBox.clear()
        try: 
            self.plotConfigFieldSelectionComboBox.addItems(defaults.PLOT_FIELDS[fieldType])
            self.plotConfigCurrentSelection = obj
            self.plotConfigAcceptPushButton.setEnabled(True)
        except KeyError:
            #undefined field - see PLOT_FIELDS variable in defaults.py
            self.plotConfigFieldSelectionComboBox.clear()
            self.plotConfigCurrentSelection = None
            self.plotConfigAcceptPushButton.setEnabled(False)
    
#    def plotConfigCurrentPlotWinChanged(self,qSubWin):
#        number = self.plotConfigWinSelectionComboBox.findText(qSubWin.windowTitle())
#        print number
#        if number != -1:
#            self.plotConfigWinSelectionComboBox.currentIndex(number)

    def populateDataPlots(self,modelpath):
        """Create plots for all Table objects in /data element"""

    def addFieldToPlot(self):
        #creates tables - called when 'Okay' pressed in plotconfig dock
        dataNeutral = moose.Neutral(self.plotConfigCurrentSelection.getField('path')+'/data')
        newTable = moose.Table(self.plotConfigCurrentSelection.getField('path')+'/data/'+str(self.plotConfigFieldSelectionComboBox.currentText()))
        moose.connect(newTable,'requestData', self.plotConfigCurrentSelection,'get_'+str(self.plotConfigFieldSelectionComboBox.currentText()))
        moose.useClock(4, newTable.getField('path'), 'process') #assign clock after creation itself 

        if str(self.plotConfigWinSelectionComboBox.currentText()) in self.plotWindowFieldTableDict:
            #case when plotwin already exists - append new table to mooseplotwin
            self.plotWindowFieldTableDict[str(self.plotConfigWinSelectionComboBox.currentText())].append(newTable)
            #select the corresponding plot (mooseplot) from the plotwindow (mooseplotwindow) 
            plotWin = self.plotNameWinDict[str(self.plotConfigWinSelectionComboBox.currentText())] 
            plotWin.plot.addTable(newTable,self.plotConfigCurrentSelection.getField('name')+'.'+newTable.getField('name'))
            plotWin.plot.nicePlaceLegend()
            plotWin.plot.axes.figure.canvas.draw()    
            self.activeWindow = plotWin

        else:
            #no previous mooseplotwin - so create, and add table to corresp dict
            self.plotWindowFieldTableDict[str(self.plotConfigWinSelectionComboBox.currentText())] = [newTable]
            plotWin = newPlotSubWindow(self.mdiArea)
            plotWin.setWindowTitle(str(self.plotConfigWinSelectionComboBox.currentText()))
            plotWin.plot.addTable(newTable,self.plotConfigCurrentSelection.getField('name')+'.'+newTable.getField('name'))
            plotWin.plot.nicePlaceLegend()
            plotWin.plot.axes.figure.canvas.draw()
            plotWin.show()
            self.plotNameWinDict[str(self.plotConfigWinSelectionComboBox.currentText())] = plotWin
            
            #self.mdiArea.addSubWindow(plotWin)
            self.activeWindow = plotWin

    def plotConfigAddNewPlotWindow(self):
        #called when new plotwindow pressed in plotconfig dock
        count = len(self.plotNameWinDict)
        if count != 0:
            self.plotConfigWinSelectionComboBox.addItem('Plot Window '+str(count+1))
        self.plotConfigWinSelectionComboBox.setCurrentIndex(count)

        plotWin = newPlotSubWindow(self)
        plotWin.setWindowTitle(str(self.plotConfigWinSelectionComboBox.currentText()))

        self.plotNameWinDict[str(self.plotConfigWinSelectionComboBox.currentText())] = plotWin
        self.plotWindowFieldTableDict[str(self.plotConfigWinSelectionComboBox.currentText())] = []

        #plotWin.plot.nicePlaceLegend()

        self.mdiArea.addSubWindow(plotWin)
        self.mdiArea.setActiveSubWindow(plotWin)
        plotWin.show()        

        self.activeWindow = plotWin

        #general
    def updateCurrentTime(self,currentTime):
        self.currentTime = currentTime
        self.simControlCurrentTimeLabel.setText(str('%.3f' %self.currentTime))

    def doQuit(self):
        QtGui.qApp.closeAllWindows()

    def _resetAndRunSlot(self): #called when run is pressed
        self.mooseHandler.stopSimulation = 0
        try:
            runtime = float(str(self.simControlRunTimeLineEdit.text()))
        except ValueError:
            runtime = MooseHandler.runtime
            self.simControlRunTimeLineEdit.setText(str(runtime))
        self.mooseHandler.doResetAndRun(runtime,float(self.simControlSimdtLineEdit.text()),float(self.simControlPlotdtLineEdit.text()),float(self.simControlUpdatePlotdtLineEdit.text()))
        self.simControlSimdtLineEdit.setEnabled(False)
        self.simControlPlotdtLineEdit.setEnabled(False)
        self.simControlUpdatePlotdtLineEdit.setEnabled(False)

    def _stopSlot(self):
        self.mooseHandler.stopSimulation = 1
    
    def _resetSlot(self): #called when reset is pressed
        self.mooseHandler.stopSimulation = 0
        try:
            runtime = float(str(self.simControlRunTimeLineEdit.text()))
        except ValueError:
            runtime = MooseHandler.runtime
            self.simControlRunTimeLineEdit.setText(str(runtime))
        self.mooseHandler.doReset(float(self.simControlSimdtLineEdit.text()),float(self.simControlPlotdtLineEdit.text()))
        self.updatePlots(self.mooseHandler.getCurrentTime()) #clears the plots
        self.simControlSimdtLineEdit.setEnabled(True)
        self.simControlPlotdtLineEdit.setEnabled(True)
        self.simControlUpdatePlotdtLineEdit.setEnabled(True)

    def _continueSlot(self): #called when continue is pressed
        self.mooseHandler.stopSimulation = 0
        try:
            runtime = float(str(self.simControlRunTimeLineEdit.text()))
        except ValueError:
            runtime = MooseHandler.runtime
            self.simControlRunTimeLineEdit.setText(str(runtime))

        self.mooseHandler.doRun(float(str(self.simControlRunTimeLineEdit.text()))) 
        self.updatePlots(self.mooseHandler.getCurrentTime()) #updates the plots
        
    def updateDefaultTimes(self, modeltype,modelpath): 
        if(modeltype == MooseHandler.type_kkit):
            self.mooseHandler.updateDefaultsKKIT(modelpath)

        self.simControlSimdtLineEdit.setText(str(self.mooseHandler.simdt))
        self.simControlPlotdtLineEdit.setText(str(self.mooseHandler.plotdt))
        self.simControlUpdatePlotdtLineEdit.setText(str(self.mooseHandler.plotupdate_dt)) 
        self.simControlRunTimeLineEdit.setText(str(self.mooseHandler.runtime)) #default run time taken
            
        '''
        if (modeltype == MooseHandler.type_kkit) or (modeltype == MooseHandler.type_sbml):
            self.simdtText.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_SIMDT_KKIT)))
            self.plotdtText.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_PLOTDT_KKIT)))
            #self.gldtText.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_GLDT_KKIT)))
            self.runtimeText.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_RUNTIME_KKIT)))
            #harsha run from menu takes from self.runtimeText and run for toolbar takes from self.runTimeEditToolbar
            self.runTimeEditToolbar.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_RUNTIME_KKIT)))
            self.updateTimeText.setText(QtCore.QString('%1.3e' % (MooseHandler.DEFAULT_PLOTUPDATE_DT_KKIT)))
            '''
    def populateKKitPlots(self,path):
        graphs = self.getKKitGraphs(path) #clocks are assigned in the script itself!
        #currently putting all plots on Plot Window 1 by default - perhaps not the nicest way to do it.
        self.plotWindowFieldTableDict['Plot Window 1'] = graphs
        plotWin = newPlotSubWindow(self.mdiArea)
        #plotWin = MoosePlotWindow()
        #plot = MoosePlot(plotWin)
        #plot.setObjectName("plot")
        #plotWin.setWidget(plot)
        self.mdiArea.addSubWindow(plotWin)
        #self.mdiArea.setActiveSubWindow(plotWin)
        plotWin.setWindowTitle('Plot Window 1')
        for graph in graphs:
            tab = moose.element(graph)
            tableObject = tab.getNeighbors('requestData')
            if(len(tableObject) > 0):
                iteminfo = tableObject[0].path+'/info'
                c = moose.Annotator(iteminfo).getField('color')
                textColor = self.colorCheck(c)
            else: textColor = 'blue'
            plotWin.plot.addTable(graph,graph.getField('name'),textColor)
        plotWin.show()
        plotWin.plot.nicePlaceLegend()
        plotWin.plot.axes.figure.canvas.draw()
        self.plotNameWinDict['Plot Window 1'] = plotWin
        self.mdiArea.setActiveSubWindow(self.activeWindow)
        self.activeWindow = plotWin

    def activeMdiWindow(self):
        self.mdiArea.setActiveSubWindow(self.activeWindow)

    def getKKitGraphs(self,path):
        tableList = []
        for child in moose.wildcardFind(path+'/graphs/#/##[TYPE=Table],'+path+'/moregraphs/#/##[TYPE=Table]'):
            tableList.append(moose.Table(child))
        return tableList

    def colorCheck(self,textColor):
        pkl_file = open(os.path.join(PATH_KKIT_COLORMAPS,'rainbow2.pkl'),'rb')
        picklecolorMap = pickle.load(pkl_file)
        hexchars = "0123456789ABCDEF"
        if (textColor == ''): textColor = 'blue'
        if(isinstance(textColor,(list,tuple))):
            r,g,b = textColor[0],textColor[1],textColor[2]
            textColor = "#"+ hexchars[r / 16] + hexchars[r % 16] + hexchars[g / 16] + hexchars[g % 16] + hexchars[b / 16] + hexchars[b % 16]
        elif ((not isinstance(textColor,(list,tuple)))):
            if textColor.isdigit():
                tc = int(textColor)
                tc = (tc * 2 )
                r,g,b = picklecolorMap[tc]
                textColor = "#"+ hexchars[r / 16] + hexchars[r % 16] + hexchars[g / 16] + hexchars[g % 16] + hexchars[b / 16] + hexchars[b % 16]
        return(textColor)
# create the GUI application
app = QtGui.QApplication(sys.argv)
icon = QtGui.QIcon(os.path.join(config.KEY_ICON_DIR,'moose_icon.png'))
app.setWindowIcon(icon)
# instantiate the main window
dmw = DesignerMainWindow()
dmw.setWindowState(Qt.WindowMaximized)
# show it
dmw.show()
# start the Qt main loop execution, exiting from this script
#http://code.google.com/p/subplot/source/browse/branches/mzViewer/PyMZViewer/mpl_custom_widget.py
#http://eli.thegreenplace.net/files/prog_code/qt_mpl_bars.py.txt
#http://lionel.textmalaysia.com/a-simple-tutorial-on-gui-programming-using-qt-designer-with-pyqt4.html
#http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg13241.html
# with the same return code of Qt application
sys.exit(app.exec_())



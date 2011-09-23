import sys
from PyQt4 import QtCore, QtGui
from PyGLWidget import PyGLWidget
from oglfunc.sobjects import *
from oglfunc.camera import *
from numpy import arange,digitize
import pickle

class newGLWindow(QtGui.QMainWindow):
    def __init__(self, parent = None):
        # initialization of the superclass
        super(newGLWindow, self).__init__(parent)
        # setup the GUI --> function generated by pyuic4
        self.name = 'GL Window '
        self.setupUi(self)

    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.setWindowModality(QtCore.Qt.NonModal)
        MainWindow.resize(500, 500)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.MinimumExpanding, QtGui.QSizePolicy.MinimumExpanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(MainWindow.sizePolicy().hasHeightForWidth())
        MainWindow.setSizePolicy(sizePolicy)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setObjectName("horizontalLayout")
        self.mgl = updatepaintGL(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.mgl.sizePolicy().hasHeightForWidth())
        self.mgl.setSizePolicy(sizePolicy)
        self.mgl.setObjectName("mgl")
        self.verticalLayout.addWidget(self.mgl)
	        
        MainWindow.setCentralWidget(self.centralwidget)
        MainWindow.setWindowTitle(QtGui.QApplication.translate("MainWindow", self.name, None, QtGui.QApplication.UnicodeUTF8))


class updatepaintGL(PyGLWidget):
    #camera = 1
    camera = Camera()
    def paintGL(self):
        PyGLWidget.paintGL(self)
	self.render()

    def setSelectionMode(self,mode):	
	self.selectionMode = mode	
	
    def render(self):

#        if self.camera:
#            self.camera.setLens(float(self.width()), max(1.0,float(self.height())))
#            self.camera.setView()
#        else:
        self.renderAxis()	#draws 3 axes at origin
 
        if self.lights:
		glMatrixMode(GL_MODELVIEW)
		glEnable(GL_LIGHTING)
		glEnable(GL_LIGHT0)
		glEnable(GL_COLOR_MATERIAL)

		light0_pos = 200.0, 200.0, 600.0, 0
		diffuse0 = 1.0, 1.0, 1.0, 1.0
		specular0 = 1.0, 1.0, 1.0, 1.0
		ambient0 = 0,0,0, 1

		glMatrixMode(GL_MODELVIEW)
		glLightfv(GL_LIGHT0, GL_POSITION, light0_pos)
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0)
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular0)
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0)

	
	for obj in self.sceneObjects:
	    obj.render()
	    
	for obj in self.vizObjects:
	    obj.render()
	
	self.selectedObjects.render()

        
    
    def setColorMap(self,vizMinVal=-0.1,vizMaxVal=0.07,moosepath='',variable='Vm',cMap='/home/chaitu/Desktop/GuiOG/oglfunc/colors/jet'):
    	self.colorMap=[]
    	self.stepVals=[]
    	self.moosepath=moosepath
    	self.variable=variable
    	if cMap=='':
    		steps = 64
    		for x in range(0,steps):
	 		r=max((2.0*x)/steps-1,0.0)
			b=max((-2.0*x)/steps+1,0.0)
			g=min((2.0*x)/steps,(-2.0*x)/steps+2)
			self.colorMap.append([r,g,b])
	else:
		f = open(cMap,'r')
		self.colorMap = pickle.load(f)
		steps = len(self.colorMap)
		f.close()
	self.stepVals = arange(vizMinVal,vizMaxVal,(vizMaxVal-vizMinVal)/steps)
	
    def setColorMap_2(self,vizMinVal_2=-0.1,vizMaxVal_2=0.07,moosepath_2='',variable_2='Vm'):	#colormap for the radius - grid view case
    	self.moosepath_2 = moosepath_2
    	self.variable_2 = variable_2
    	self.stepVals_2 = arange(vizMinVal_2,vizMaxVal_2,(vizMaxVal_2-vizMinVal_2)/30)		#assigned a default of 30 steps
    	self.indRadius = arange(0.05,0.20,0.005)						#radius equivalent colormap 

if __name__ == '__main__':
        app = QtGui.QApplication(sys.argv)
        widget = newGLWindow()
        widget.show()
        app.exec_()

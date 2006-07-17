"""
Kross Script Editor

Description:
This script provides a in python written script editor.

Author:
Sebastian Sauer <mail@dipe.org>

Copyright:
Dual-licensed under LGPL v2+higher and the BSD license.
"""

import os, sys

try:
	import qt
except (ImportError):
	raise "Failed to import the required PyQt python module."

class Widget(qt.QHBox):
	def __init__(self, parentwidget, label = None):
		self.parentwidget = parentwidget
		import qt
		qt.QHBox.__init__(self, parentwidget)
		self.setMargin(4)
		self.setSpacing(4)
		if label != None: qt.QLabel(label, self)
	def value(self):
		return None

class ListWidget(Widget):
	def __init__(self, parentwidget, label):
		import qt
		global Widget
		Widget.__init__(self, parentwidget, label)
		self.combo = qt.QComboBox(self)
		self.combo.setEditable(True)
		self.setStretchFactor(self.combo,1)
	def value(self):
		return self.combo.currentText()

class EditWidget(Widget):
	def __init__(self, parentwidget, label):
		import qt
		global Widget
		Widget.__init__(self, parentwidget, label)
		self.edit = qt.QLineEdit(self)
		self.setStretchFactor(self.edit,1)
	def value(self):
		return self.edit.text()

class FileWidget(Widget):
	def __init__(self, parentwidget, label, filtermask):
		self.filtermask = filtermask
		import qt
		global Widget
		Widget.__init__(self, parentwidget, label)
		self.edit = qt.QLineEdit(self)
		self.setStretchFactor(self.edit,1)
		btn = qt.QPushButton("...",self)
		qt.QObject.connect(btn, qt.SIGNAL("clicked()"), self.btnClicked)
	def btnClicked(self):
		import qt
		text = str( self.edit.text() )
		filename = str( qt.QFileDialog.getOpenFileName(text, self.filtermask, self.parentwidget) )
		if filename != "": self.edit.setText( filename )
	def value(self):
		return self.edit.text()

class Samples:

	class KexiDB:
		def __init__(self, parentwidget):
			self.parentwidget = parentwidget

		class _ProjectWidget(FileWidget):
			def __init__(self, parentwidget):
				global FileWidget
				FileWidget.__init__(self, parentwidget, "Project File:", "*.kexi *.kexis *.kexic;;*")

		class _DriverWidget(ListWidget):
			def __init__(self, parentwidget):
				global ListWidget
				ListWidget.__init__(self, parentwidget, "Driver:")
				import krosskexidb
				for driver in krosskexidb.DriverManager().driverNames():
					self.combo.insertItem(driver)

		class _TableWidget(ListWidget):
			def __init__(self, parentwidget):
				global ListWidget
				ListWidget.__init__(self, parentwidget, "Table:")

		class PrintDriverDetails:
			"""  Print a the list of available KexiDB drivers and print details about one of them. """
			name = "Details about a driver"
			def __init__(self, parent):
				self.widgets = {
					"DriverName" : Samples.KexiDB._DriverWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'import krosskexidb',
					'drivermanager = krosskexidb.DriverManager()',
					'print "drivernames: %s" % drivermanager.driverNames()',
					'',
					'driver = drivermanager.driver( \"{DriverName}\" )',
					'print "driver: {DriverName}"',
					'print "version=%s.%s" % (driver.versionMajor(),driver.versionMinor())',
					'print "mimetype=%s" % driver.fileDBDriverMimeType()',
					'print "filedriver=%s" % driver.isFileDriver()',
				)

		class ConnectWithFile:
			""" Connect with a KexiDB database by using a Kexi Connection Project File. """
			name = "Connect with file"
			def __init__(self, parent):
				self.widgets = {
					"ProjectFile" : Samples.KexiDB._ProjectWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'# Import the KexiDB module.',
					'import krosskexidb',
					'drivermanager = krosskexidb.DriverManager()',
					'',
					'# Get the connectiondata from the project file.',
					'connectiondata = drivermanager.createConnectionDataByFile( "{ProjectFile}" )',
					'print "Connectiondata: %s" % connectiondata.serverInfoString()',
					'',
					'# Create the driver for the database backend.',
					'driver = drivermanager.driver( connectiondata.driverName() )',
					'',
					'# Create and establish the connection to the database.',
					'connection = driver.createConnection(connectiondata)',
					'if not connection.isConnected():',
					'	if not connection.connect():',
					'		raise "Failed to connect"',
					'',
					'# Open the database for usage.',
					'print "Databases: %s" % connection.databaseNames()',
					'if not connection.isDatabaseUsed():',
					'	if not connection.useDatabase( connectiondata.databaseName() ):',
					'		if not connection.useDatabase( connectiondata.fileName() ):',
					'			raise "Failed to use database"',
					'',
					'# Print some infos.',
					'print "All tables: %s" % connection.allTableNames()',
					'print "Tables: %s" % connection.tableNames()',
					'print "Queries: %s" % connection.queryNames()',
				)

		class IterateThroughTable:
			""" Iterate through a table within a connected KexiDB database. """
			name = "Iterate through table"
			def __init__(self, parent):
				self.widgets = {
					"ProjectFile" : Samples.KexiDB._ProjectWidget( parent.parentwidget ),
					"TableName" : Samples.KexiDB._TableWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'# Import the KexiDB module.',
					'import krosskexidb',
					'drivermanager = krosskexidb.DriverManager()',
					'',
					'# Get the connectiondata from the project file.',
					'connectiondata = drivermanager.createConnectionDataByFile( "{ProjectFile}" )',
					'print "Connectiondata: %s" % connectiondata.serverInfoString()',
					'',
					'# Create the driver for the database backend.',
					'driver = drivermanager.driver( connectiondata.driverName() )',
					'',
					'# Create and establish the connection to the database.',
					'connection = driver.createConnection(connectiondata)',
					'if not connection.isConnected():',
					'	if not connection.connect():',
					'		raise "Failed to connect"',
					'',
					'# Open the database for usage.',
					'if not connection.isDatabaseUsed():',
					'	if not connection.useDatabase( connectiondata.databaseName() ):',
					'		if not connection.useDatabase( connectiondata.fileName() ):',
					'			raise "Failed to use database"',
					'',
					'# Get the table and create a query for it.',
					'table = connection.tableSchema( \"{TableName}\" )',
					'query = table.query()',
					'',
					'# Create a cursor to walk through the records.',
					'cursor = connection.executeQuerySchema( query )',
					'if not cursor:',
					'	raise "Failed to create cursor."',
					'',
					'# Iterate through the records.',
					'if not cursor.moveFirst():',
					'	raise "The cursor has no records to read from."',
					'while not cursor.eof():',
					'	for i in range( cursor.fieldCount() ):',
					'		print "%s" % cursor.value(i)',
					'	cursor.moveNext()',
				)

	class KSpread:
		def __init__(self, parentwidget):
			self.parentwidget = parentwidget

		class _SheetWidget(ListWidget):
			def __init__(self, parentwidget):
				global ListWidget
				ListWidget.__init__(self, parentwidget, "Sheet:")

				try:
					import krosskspreadcore
					document = krosskspreadcore.get("KSpreadDocument")
					for sheetname in document.sheetNames():
						self.combo.insertItem(sheetname)
				except ImportError:
					pass

		class _CellsWidget(ListWidget):
			def __init__(self, parentwidget):
				global ListWidget
				ListWidget.__init__(self, parentwidget, "Cells (col1:row1 - col2:row2):")
				self.combo.insertItem( "1:1 - %s:%s" % (5,5) )
				self.combo.insertItem( "1:1 - %s:%s" % (256,256) )
				self.combo.insertItem( "1:1 - %s:%s" % (32767,32767) )
			def value(self):
				return [ [ int(i) for i in item.split(':') ] for item in str( ListWidget.value(self) ).split('-') ]

		class _ValueWidget(EditWidget):
			def __init__(self, parentwidget):
				global EditWidget
				EditWidget.__init__(self, parentwidget, "Value:")
				self.edit.setText("Some text")

		class _ColorWidget(EditWidget):
			def __init__(self, parentwidget,label,color):
				global EditWidget
				EditWidget.__init__(self, parentwidget, "%s (RGB):" % label)
				self.edit.setText(color)
			def value(self):
				return "#%s" % EditWidget.value(self)
		class SetTextOfCells:
			""" Set the text of the defined cells. """
			name = "Set text of cells"
			def __init__(self, parent):
				pass
				self.widgets = {
					"SheetName" : Samples.KSpread._SheetWidget( parent.parentwidget ),
					"Cells" : Samples.KSpread._CellsWidget( parent.parentwidget ),
					"Value" : Samples.KSpread._ValueWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'# Import the KSpread module.',
					'import krosskspreadcore',
					'',
					'# Get the current document.',
					'document = krosskspreadcore.get("KSpreadDocument")',
					'',
					'# Get the sheet defined by the sheetname.',
					'sheet = document.sheetByName( \"{SheetName}\" )',
					'if not sheet:',
					'	raise "No such sheet {SheetName} %s" % document.sheetNames()',
					'',
					'( (col1,row1),(col2,row2) ) = {Cells}',
					'for c in range(col1,col2):',
					'	for r in range(col1,col2):',
					'		cell = sheet.cell(c,r)',
					'		print "cell c=%s r=%s v=%s" % (c,r,cell.value())',
					'		cell.setText( \"{Value}\" )',
				)

		class SetColorsOfCells:
			""" Set the colors of the defined cells. """
			name = "Set colors of cells"
			def __init__(self, parent):
				pass
				self.widgets = {
					"SheetName" : Samples.KSpread._SheetWidget( parent.parentwidget),
					"Cells" : Samples.KSpread._CellsWidget( parent.parentwidget ),
					"TextColor" : Samples.KSpread._ColorWidget( parent.parentwidget, "Textcolor", "ff0000" ),
					"BackgroundColor" : Samples.KSpread._ColorWidget( parent.parentwidget, "Backgroundcolor", "c0c0c0" ),
				}
			def getCode(self):
				return (
					'# Import the KSpread module.',
					'import krosskspreadcore',
					'',
					'# Get the current document.',
					'document = krosskspreadcore.get("KSpreadDocument")',
					'',
					'# Get the sheet defined by the sheetname.',
					'sheet = document.sheetByName( \"{SheetName}\" )',
					'if not sheet:',
					'	raise "No such sheet {SheetName} %s" % document.sheetNames()',
					'',
					'( (col1,row1),(col2,row2) ) = {Cells}',
					'for c in range(col1,col2):',
					'	for r in range(col1,col2):',
					'		cell = sheet.cell(c,r)',
					'		cell.setTextColor( \"{TextColor}\" )',
					'		cell.setBackgroundColor( \"{BackgroundColor}\" )',
				)

		class IterateThroughCellsWithContent:
			""" Iterate over all cells in a sheet that have content (aka that are not empty). """
			name = "Iterate through cells"
			def __init__(self, parent):
				pass
				self.widgets = {
					"SheetName" : Samples.KSpread._SheetWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'# Import the KSpread module.',
					'import krosskspreadcore',
					'',
					'# Get the current document.',
					'document = krosskspreadcore.get("KSpreadDocument")',
					'',
					'# Get the sheet defined by the sheetname.',
					'sheet = document.sheetByName( \"{SheetName}\" )',
					'if not sheet:',
					'	raise "No such sheet {SheetName} %s" % document.sheetNames()',
					'',
					'# Iterate through the cells that have content (aka that are not empty).',
					'cell = sheet.firstCell()',
					'while cell:',
					'	print "col=%s row=%s value=%s" % (cell.column(),cell.row(),cell.value())',
					'	cell = cell.nextCell()',
				)

		class PrintSheetDetails:
			""" Print details abvout the current sheet. """
			name = "Details about a sheet"
			def __init__(self, parent):
				pass
				self.widgets = {
					"SheetName" : Samples.KSpread._SheetWidget( parent.parentwidget ),
				}
			def getCode(self):
				return (
					'# Import the KSpread module.',
					'import krosskspreadcore',
					'',
					'# Get the current document.',
					'document = krosskspreadcore.get("KSpreadDocument")',
					'',
					'# Get the sheet defined by the sheetname.',
					'sheet = document.sheetByName( \"{SheetName}\" )',
					'if not sheet:',
					'	raise "No such sheet {SheetName} %s" % document.sheetNames()',
					'',
					'print "name=%s" % sheet.name()',
					'print "maxcolumns=%s maxrows=%s" % (sheet.maxColumn(),sheet.maxRow())',
				)

class SampleDialog(qt.QDialog):
	def __init__(self, parent, sampleclazz, samplechildclazz):
		import qt
		qt.QDialog.__init__(self, parent, "SampleDialog", 1)

		layout = qt.QVBoxLayout(self)
		box = qt.QVBox(self)
		box.setMargin(4)
		box.setSpacing(10)
		layout.addWidget(box)

		self.scrollview = qt.QScrollView(box)
		self.scrollview.setResizePolicy(qt.QScrollView.AutoOne)
		#self.scrollview.setFrameStyle(qt.QFrame.NoFrame);
		self.scrollview.setResizePolicy(qt.QScrollView.AutoOneFit);
		self.scrollview.viewport().setPaletteBackgroundColor(self.paletteBackgroundColor())
		mainbox = qt.QVBox( self.scrollview.viewport() )
		mainbox.setMargin(6)
		mainbox.setSpacing(6)
		desclabel = qt.QLabel(mainbox)
		qt.QFrame(mainbox).setFrameStyle( qt.QFrame.HLine | qt.QFrame.Sunken )

		self.sample = sampleclazz( mainbox )
		self.samplechild = samplechildclazz( self.sample )

		desclabel.setText( "<qt>%s</qt>" % self.samplechild.__doc__ )
		mainbox.setStretchFactor(qt.QWidget(mainbox), 1)
		mainbox.show()
		self.scrollview.addChild(mainbox)

		btnbox = qt.QHBox(box)
		btnbox.setMargin(6)
		btnbox.setSpacing(6)
		okbtn = qt.QPushButton(btnbox)
		okbtn.setText("Ok")
		qt.QObject.connect(okbtn, qt.SIGNAL("clicked()"), self.okClicked)
		cancelbtn = qt.QPushButton(btnbox)
		cancelbtn.setText("Cancel")
		qt.QObject.connect(cancelbtn, qt.SIGNAL("clicked()"), self.close)

		self.setCaption("Sample: %s" % self.samplechild.name)
		box.setMinimumSize(qt.QSize(500,340))

	def okClicked(self):
		self.code = self.samplechild.getCode()
		self.close()

	def getCode(self):
		if not hasattr(self,"code"): return None
		code = "\n".join( self.code )
		for widgetname in self.samplechild.widgets.keys():
			print ".............. %s" % widgetname
			widget = self.samplechild.widgets[widgetname]
			value = widget.value()
			if value != None:
				code = code.replace("{%s}" % widgetname, str(value))
		return code

class MainDialog(qt.QDialog):
	def __init__(self, scriptpath, parent):
		self.scriptpath = scriptpath
		self.filename = self.getFileName("myscript.py")

		import krosskspreadcore
		self.doc = krosskspreadcore.get("KSpreadDocument")

		import os, qt
		qt.QDialog.__init__(self, parent, "MainDialog", 1, qt.Qt.WDestructiveClose)
		self.setCaption("Actions")
		
		layout = qt.QVBoxLayout(self)
		box = qt.QVBox(self)
		box.setMargin(4)
		box.setSpacing(10)
		layout.addWidget(box)

		menu = qt.QMenuBar(box)

		splitter = qt.QSplitter(box)
		splitter.setOrientation(qt.Qt.Vertical)

		self.scripttext = qt.QTextEdit(splitter)
		self.scripttext.setWordWrap( qt.QTextEdit.NoWrap )
		self.scripttext.setTextFormat( qt.Qt.PlainText )
		qt.QObject.connect(self.scripttext, qt.SIGNAL("cursorPositionChanged(int,int)"),self.cursorPositionChanged)

		self.console = qt.QTextBrowser(splitter)
		splitter.setResizeMode(self.console, qt.QSplitter.KeepSize)

		statusbar = qt.QStatusBar(box)
		self.messagestatus = qt.QLabel(statusbar)
		statusbar.addWidget(self.messagestatus,1)
		self.cursorstatus = qt.QLabel(statusbar)
		statusbar.addWidget(self.cursorstatus)
		self.cursorPositionChanged()

		box.setMinimumSize( qt.QSize(680,540) )

		filemenu = qt.QPopupMenu(menu)
		menu.insertItem("&File", filemenu)
		
		newaction = qt.QAction("New", qt.QKeySequence("CTRL+N"), self)
		qt.QObject.connect(newaction, qt.SIGNAL("activated()"), self.newFile)
		newaction.addTo(filemenu)

		openaction = qt.QAction("Open...", qt.QKeySequence("CTRL+O"), self)
		qt.QObject.connect(openaction, qt.SIGNAL("activated()"), self.openFileAs)
		openaction.addTo(filemenu)

		saveaction = qt.QAction("Save", qt.QKeySequence("CTRL+S"), self)
		qt.QObject.connect(saveaction, qt.SIGNAL("activated()"), self.saveFile)
		saveaction.addTo(filemenu)

		saveasaction = qt.QAction("Save as...", qt.QKeySequence("CTRL+A"), self)
		qt.QObject.connect(saveasaction, qt.SIGNAL("activated()"), self.saveFileAs)
		saveasaction.addTo(filemenu)

		filemenu.insertSeparator()
		
		quitaction = qt.QAction("Quit", qt.QKeySequence("CTRL+Q"), self)
		qt.QObject.connect(quitaction, qt.SIGNAL("activated()"), self.close)
		quitaction.addTo(filemenu)

		editmenu = qt.QPopupMenu(menu)
		menu.insertItem("&Edit", editmenu)
		editmenu.insertItem("Undo", self.scripttext.undo, qt.QKeySequence("CTRL+Z"))
		editmenu.insertItem("Redo", self.scripttext.redo, qt.QKeySequence("CTRL+Shift+Z"))
		editmenu.insertSeparator()
		editmenu.insertItem("Cut", self.scripttext.cut, qt.QKeySequence("CTRL+X"))
		editmenu.insertItem("Copy", self.scripttext.copy, qt.QKeySequence("CTRL+C"))
		editmenu.insertItem("Paste", self.scripttext.paste, qt.QKeySequence("CTRL+V"))
		editmenu.insertItem("Clear", self.scripttext.clear, qt.QKeySequence("CTRL+Shift+X"))
		editmenu.insertSeparator()
		editmenu.insertItem("Select All", self.scripttext.selectAll)

		scriptmenu = qt.QPopupMenu(menu)
		menu.insertItem("&Script", scriptmenu)

		compileaction = qt.QAction("Compile", qt.QKeySequence("F9"), self)
		qt.QObject.connect(compileaction, qt.SIGNAL("activated()"), self.compileScript)
		compileaction.addTo(scriptmenu)

		executeaction = qt.QAction("Execute", qt.QKeySequence("F10"), self)
		qt.QObject.connect(executeaction, qt.SIGNAL("activated()"), self.executeScript)
		executeaction.addTo(scriptmenu)

		self.samplemenu = qt.QPopupMenu(menu)
		menu.insertItem("&Samples", self.samplemenu)
		itemid = 500
		global Samples
		for samplename in dir(Samples):
			if samplename.startswith("_"): continue
			itemid += 1
			menu = qt.QPopupMenu(self.samplemenu)
			qt.QObject.connect(menu, qt.SIGNAL("activated(int)"), self.sampleActivated)
			self.samplemenu.insertItem(samplename, menu)
			attr = getattr(Samples,samplename)
			for a in dir(attr):
				if a.startswith("_"): continue
				itemid += 1
				child = getattr(attr,a)
				itemid = menu.insertItem(child.name, itemid)
				menu.setWhatsThis(itemid,"%s/%s" % (samplename,a))

		if os.path.exists(self.filename):
			self.openFile(self.filename)

	def getFileName(self, filename):
		import os
		try:
			homepath = os.getenv("HOME")
			if not homepath:
				import pwd
				user = os.getenv("USER") or os.getenv("LOGNAME")
				if not user:
					pwent = pwd.getpwuid(os.getuid())
				else:
					pwent = pwd.getpwnam(user)
				homepath = pwent[6]
		except (KeyError, ImportError):
			homepath = os.curdir
		return os.path.join(homepath, filename)

	def cursorPositionChanged(self,para = 0,pos = 0):
		self.cursorstatus.setText( "Line: %s Col: %s" % (para+1,pos+1) )

	def sampleActivated(self, index):
		global Samples
		sampleid = str( self.sender().whatsThis(index) )
		sampleidlist = sampleid.split('/')
		sampleclazz = getattr( Samples,sampleidlist[0] )
		samplechildclazz = getattr( sampleclazz, sampleidlist[1] )

		global SampleDialog
		dialog = SampleDialog(self, sampleclazz, samplechildclazz)
		dialog.exec_loop()
		code = dialog.getCode()
		if code != None:
			self.scripttext.append( code )

	def execCode(self,function):
		import sys, StringIO
		codeOut = StringIO.StringIO()
		codeErr = StringIO.StringIO()
		sys.stdout = codeOut
		sys.stderr = codeErr

		try:
			function(self)
		except:
			import traceback
			trace = "".join( traceback.format_exception(sys.exc_info()[0],sys.exc_info()[1],sys.exc_info()[2]) )
			sys.stderr.write(trace)

		sys.stdout = sys.__stdout__
		sys.stderr = sys.__stderr__

		s = codeErr.getvalue()
		if s:
			print "ERROR:\n%s\n" % s
			self.console.append(s)

		s = codeOut.getvalue()
		if s:
			print s
			self.console.append(s)

		codeOut.close()
		codeErr.close()

	def compileScript(self):
		self.console.clear()
		code = str( self.scripttext.text() )
		def docompile(self):
			compile(code, self.filename, 'exec')
		self.execCode(docompile)
		self.console.append("<b>Compiled!</b>")

	def executeScript(self):
		self.console.clear()
		def doexecute(self):
			code = str( self.scripttext.text() )
			exec code in globals(), locals()
		self.execCode(doexecute)
		self.console.append("<b>Execution done!</b>")

	def newFile(self):
		self.console.clear()
		#if qt.QMessageBox.warning(self,"Remove?","Remove the selected item?",qt.QMessageBox.Yes,qt.QMessageBox.Cancel) != qt.QMessageBox.Yes:
		self.scripttext.clear()

	def openFile(self, filename):
		self.filename = None
		try:
			file = open(filename, "r")
			self.scripttext.setText( str( file.read() ) )
			file.close()
			self.filename = filename
		except IOError, (errno, strerror):
			qt.QMessageBox.critical(self,"Error","<qt>Failed to open script file \"%s\"<br><br>%s</qt>" % (filename,strerror))

	def openFileAs(self):
		import qt
		self.console.clear()
		filename = str( qt.QFileDialog.getOpenFileName(self.filename,"*.py;;*", self) )
		if filename == "": return
		self.openFile(filename)

	def saveFile(self):
		try:
			file = open(self.filename, "w")
			file.write( str( self.scripttext.text() ) )
			file.close()
		except IOError, (errno, strerror):
			qt.QMessageBox.critical(self,"Error","<qt>Failed to open script file \"%s\"<br><br>%s</qt>" % (self.filename,strerror))

	def saveFileAs(self):
		import qt
		filename = str( qt.QFileDialog.getSaveFileName(self.filename,"*.py;;*", self) )
		if filename == "": return
		self.filename = filename
		self.saveFile()

if __name__ == "__main__":
	scriptpath = os.getcwd()
	qtapp = qt.QApplication(sys.argv)
else:
	scriptpath = os.path.dirname(__name__)
	qtapp = qt.qApp

MainDialog = MainDialog(scriptpath, qtapp.mainWidget())
MainDialog.exec_loop()

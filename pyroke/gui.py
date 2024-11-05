#! cd .. && python main.py

import os, sys
import datetime
from PyQt6.QtCore import *
from PyQt6.QtWidgets import *
from PyQt6.QtGui import *
from .TableView import ListView, TableView, StyledItemDelegate, RowValueRole
from .libroke import RokeException, default_config_dir, \
    build, rebuild, build_cancel, rename_index, \
    find, index_info, \
    ROKE_CASE_SENSITIVE, ROKE_CASE_INSENSITIVE, \
    ROKE_DEFAULT, ROKE_GLOB, ROKE_REGEX
import json
import traceback
import subprocess
import time

"""
bool    unifiedTitleAndToolBarOnMac() const
        setUnifiedTitleAndToolBarOnMac(True)
void    removeToolBar(QToolBar *toolbar)

        print(help(self.findChildren))
        for child in self.findChildren(QObject):
            print(child)

"""
def open_native(path):

    if sys.platform == "win32":
        args = ["explorer", path]
        print(args)
        subprocess.Popen(args)

class ContextMenu(QMenu):

    def __init__(self, parent):
        super(ContextMenu, self).__init__(parent)
        self.actions = dict()

    def add(self, text, fptr):
        """ associate a function with an action """
        self.actions[self.addAction(text)] = fptr

    def exec_(self, position):

        action = super().exec_(position)
        if action:
            self.actions[action]()

class RokeConfig(object):
    def __init__(self, path):
        super(RokeConfig, self).__init__()
        self.path = path
        self.data = {}
        self._dirty = False

    @staticmethod
    def getConfig(config_dir):

        if not os.path.exists(config_dir):
            os.makedirs(config_dir)

        path = os.path.join(config_dir, "roke.json")

        cfg = RokeConfig(path)

        if os.path.exists(path):
            with open(path) as rf:
                cfg.data = json.load(rf)
            return cfg

        cfg.data = {
            "search_flags": 0,
            "search_limit": 1000,
        }

        return cfg;

    def save(self):
        if self._dirty:
            with open(self.path, "w") as wf:
                json.dump(self.data, wf)
        self._dirty = False

    def getSearchFlags(self):
        return self.data.get('search_flags', 0)

    def setSearchFlags(self, flags):
        self.data['search_flags'] = flags
        self._dirty = True

    def getSearchLimit(self):
        return self.data.get('search_limit', 0)

    def setSearchLimit(self, limit):
        self.data['search_limit'] = limit
        self._dirty = True

class BuildThread(QThread):

    statusText = pyqtSignal(str)

    def __init__(self, config_dir, name, path):
        super(BuildThread, self).__init__()
        self.config_dir = config_dir
        self.name = name
        self.path = path

    def run(self):
        try:
            for line in build(self.config_dir, self.name, self.path):
                self.statusText.emit(line)
        except RokeException as e:
            print(e)

class RebuildAllThread(QThread):

    statusText = pyqtSignal(str)

    def __init__(self, config_dir):
        super(RebuildAllThread, self).__init__()
        self.config_dir = config_dir

    def run(self):

        items = []
        if os.path.exists(self.config_dir):
            for name in os.listdir(self.config_dir):
                if name.endswith(".d.bin"):
                    items.append(name.replace(".d.bin", ""))

        try:
            for name in items:
                for line in rebuild(self.config_dir, name):
                    self.statusText.emit("%s: %s" % (name, line))
        except RokeException as e:
            print(e)

class RokeIndexDelegate(StyledItemDelegate):
    """ A text edit delegate which validates the name of an index

    Handles the renaming of an index
    """
    def __init__(self, config_dir, parent=None):
        super(RokeIndexDelegate,self).__init__(parent)
        self.config_dir = config_dir

    def createEditor(self,parent,option,index):
        return QLineEdit(parent)

    def setEditorData(self,editor,index):
        editor.setText(str(index.data()))

    def setModelData(self,editor,model,index):
        old_name = str(index.data())
        new_name = editor.text().strip()

        if new_name != old_name:
            print("rename %s -> %s" % (old_name, new_name))
            if rename_index(self.config_dir, old_name, new_name):
                model.setData(index, new_name)

class OptionsDialog(QDialog):
    def __init__(self, config_dir, cfg):
        super(QDialog, self).__init__()
        self.setWindowTitle("Options")

        self.table = TableView(self)
        self.table.setColumnHeaderClickable(True)
        self.table.setSortingEnabled(True)
        self.table.setVerticalHeaderVisible(False)
        self.table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        self.table.MouseReleaseRight.connect(self.onShowContextMenu)
        model = self.table.baseModel()
        model.addColumn(0, "Age", editable=False)
        model.addColumn(1, "Name", editable=True)
        model.addColumn(2, "Root Directory", editable=False)
        model.addColumn(3, "Number of Files", editable=False)
        model.addColumn(4, "Number of Directories", editable=False)

        self.table.setDelegate(1, RokeIndexDelegate(config_dir, self.table))

        self.lbl_limit = QLabel("Search Results Limit:")
        self.spin_limit = QSpinBox(self)
        self.spin_limit.setRange(50, 10000)
        self.spin_limit.setSingleStep(50)

        self.btn_new = QPushButton("New")
        self.btn_refresh = QPushButton("Rebuild All")
        self.btn_save = QPushButton("Save")
        self.btn_cancel = QPushButton("Cancel")

        self.hbox = QHBoxLayout()
        self.hbox.addStretch(1)
        self.hbox.addWidget(self.btn_new)
        self.hbox.addWidget(self.btn_refresh)

        self.hbox2 = QHBoxLayout()
        self.hbox2.addStretch(1)
        self.hbox2.addWidget(self.btn_save)
        self.hbox2.addWidget(self.btn_cancel)

        self.hbox3 = QHBoxLayout()
        self.hbox3.addWidget(self.lbl_limit)
        self.hbox3.addStretch(1)
        self.hbox3.addWidget(self.spin_limit)

        self.vbox = QVBoxLayout(self)
        self.vbox.addLayout(self.hbox3)
        self.vbox.addLayout(self.hbox)
        self.vbox.addWidget(self.table)
        self.vbox.addLayout(self.hbox2)

        # --

        self.btn_new.clicked.connect(self.onCreateClicked)
        self.btn_refresh.clicked.connect(self.onRefreshClicked)
        self.btn_save.clicked.connect(self.accept)
        self.btn_cancel.clicked.connect(self.reject)

        self._parseConfig(cfg)

        self.config_dir = config_dir
        self.items = []

        self.resize(512, 400)
        self.reload()

    def _parseConfig(self, cfg):

        limit = cfg.getSearchLimit()
        self.spin_limit.setValue(limit)

    def onRadioButtonClicked(self, checked=False):
        pass

    def onShowContextMenu(self, event):

        menu = ContextMenu(self)
        menu.add("Rebuild Index", self.actionRebuildSelection)
        menu.add("Delete Index", self.actionDeleteSelection)
        menu.exec_(event.globalPos())

    def onCreateClicked(self):
        dialog = NewIndexDialog(self.config_dir)
        dialog.exec_()
        self.reload()

    def onRefreshClicked(self):
        dialog = RebuildIndexDialog(self.config_dir)
        dialog.exec_()
        self.reload()

    def reload(self):

        items = []
        if os.path.exists(self.config_dir):
            for name in os.listdir(self.config_dir):
                if name.endswith(".d.bin"):
                    name = name.replace(".d.bin", "")
                    data = self.get_index_info(name)
                    items.append(data)

        self.items = []
        self.table.setNewData(items)

    def get_index_info(self, name):

        mtime = None
        stime = ""
        root = ""
        nfiles = 0
        ndirs = 0

        try:
            st = index_info(self.config_dir, name)
            mtime = st.mtime
            root = st.root
            nfiles = st.nfiles
            ndirs = st.ndirs
        except Exception as e:
            print(e)

        try:
            if mtime is not None:
                dt1 = datetime.datetime.fromtimestamp(mtime)
                dtn = datetime.datetime.now()
                delta = dtn - dt1

                if delta.days == 0:
                    hours = delta.total_seconds() / 3600
                    stime = "%.2f hours" % (hours)
                else:
                    stime = "%d days" % (delta.days)

        except OSError as e:
            print(e)
            stime = "error"

        return [stime, name, root, nfiles, ndirs]

    def getConfigDir(self):
        return self.config_dir

    def getFlags(self):

        flags = 0

        if not self.chk_case.isChecked():
            flags |= ROKE_CASE_INSENSITIVE

        if self.rad_glob.isChecked():
            flags |= ROKE_GLOB

        if self.rad_regex.isChecked():
            flags |= ROKE_REGEX

        return flags

    def getSearchLimit(self):

        return self.spin_limit.value()

    def actionRebuildSelection(self):

        for item in self.table.getSelectedRowData():
            name = item[1]
            st = index_info(self.config_dir, name)
            # dialog = RebuildIndexDialog(self.config_dir, name)
            dialog = NewIndexDialog(self.config_dir, name, st.root)
            dialog.exec_()
            self.reload()

    def actionDeleteSelection(self):

        for item in self.table.getSelectedRowData():
            base = os.path.join(self.config_dir, item[1])

            for ext in [".d.idx", ".d.bin", ".f.idx", ".f.bin", ".err"]:
                path = base + ext
                print(path)
                if os.path.exists(path):
                    os.remove(path)
        self.reload()

class StoppableDialog(QDialog):

    abortThread = pyqtSignal()

    def __init__(self):
        super(StoppableDialog, self).__init__()
        self.thread = None

    def accept(self):
        if not self._finished():
            return
        super().accept()

    def reject(self):
        if not self._finished():
            self.abortThread.emit()
            return
        super().reject()

    def _finished(self):
        if self.thread is not None:
            return self.thread.isFinished()
        return True

class NewIndexDialog(StoppableDialog):
    """docstring for NewIndexDialog"""
    def __init__(self, config_dir, name="", root=""):
        super(NewIndexDialog, self).__init__()
        self.setWindowTitle("Create Index")

        self.config_dir = config_dir

        self.vbox = QVBoxLayout(self)
        self.grid = QGridLayout()

        self.lbl_name = QLabel("Name:")
        self.lbl_path = QLabel("Directory:")
        self.edit_name = QLineEdit(name, self)
        self.edit_path = QLineEdit(root, self)
        self.btn_opendir = QToolButton(self)
        self.btn_start = QPushButton("Create")
        self.btn_cancel = QPushButton("Cancel")
        self.pbar = QProgressBar(self)
        self.txt_status = QLabel("", self)

        self.btn_opendir.clicked.connect(self.onOpenDirClicked)
        icon = self.style().standardIcon(QStyle.SP_DirOpenIcon)
        self.btn_opendir.setIcon(icon)
        self.btn_start.clicked.connect(self.onStartClicked)
        self.btn_cancel.clicked.connect(self.reject)

        self.pbar.setRange(0, 1)
        self.pbar.setValue(0)
        self.pbar.setTextVisible(False)
        row = 0
        self.grid.addWidget(self.lbl_name,row,0,1,1,Qt.AlignLeft)
        self.grid.addWidget(self.edit_name,row,1,1,5,Qt.AlignLeft)

        row += 1
        self.grid.addWidget(self.lbl_path,row,0,1,1,Qt.AlignLeft)
        self.grid.addWidget(self.edit_path,row,1,1,4,Qt.AlignLeft)
        self.grid.addWidget(self.btn_opendir,row,5,1,1,Qt.AlignRight)

        row += 1
        self.grid.addWidget(self.btn_start,row,4,1,1,Qt.AlignLeft)
        self.grid.addWidget(self.btn_cancel,row,5,1,1,Qt.AlignLeft)

        self.vbox.addLayout(self.grid)
        self.vbox.addWidget(self.pbar)
        self.vbox.addWidget(self.txt_status)

        self.thread = None
        self.aborted = False

        self.abortThread.connect(self.onAbortThread)

    def onOpenDirClicked(self):

        path = self.edit_path.text().strip()
        path = QFileDialog.getExistingDirectory(self, "Open Directory", path)

        if path:
            self.edit_path.setText(path)

    def onAbortThread(self):

        if not self.aborted:
            build_cancel()
            self.aborted = True

    def onStartClicked(self):

        name = self.edit_name.text().strip()
        path = self.edit_path.text().strip()

        if not name:
            return

        if not path:
            return

        self.btn_start.setEnabled(False)
        self.btn_opendir.setEnabled(False)
        self.edit_name.setEnabled(False)
        self.edit_path.setEnabled(False)

        self.pbar.setRange(0, 0)

        self.thread = BuildThread(self.config_dir, name, path)
        self.thread.finished.connect(self.onThreadFinished)
        self.thread.statusText.connect(self.txt_status.setText)
        self.thread.start()

    def onThreadFinished(self):
        self.thread = None

        self.pbar.setRange(0, 1)
        self.pbar.setValue(1)

        if self.aborted:
            self.reject()
        else:
            self.btn_cancel.setText("Close")

class RebuildIndexDialog(StoppableDialog):
    """docstring for NewIndexDialog"""
    def __init__(self, config_dir):
        super(RebuildIndexDialog, self).__init__()
        self.setWindowTitle("Rebuilding All Indexes")

        self.config_dir = config_dir

        self.vbox = QVBoxLayout(self)
        self.pbar = QProgressBar(self)
        self.pbar.setRange(0, 0)
        self.pbar.setValue(0)
        self.pbar.setTextVisible(False)
        self.txt_status = QLabel("", self)

        self.vbox.addWidget(self.pbar)
        self.vbox.addWidget(self.txt_status)

        self.thread = RebuildAllThread(self.config_dir)
        self.thread.statusText.connect(self.txt_status.setText)
        self.thread.finished.connect(self.onThreadFinished)
        self.thread.start()

        self.aborted = False

        self.abortThread.connect(self.onAbortThread)

    def onAbortThread(self):
        if not self.aborted:
            build_cancel()
            self.aborted = True

    def onThreadFinished(self):
        self.thread = None
        self.pbar.setRange(0,1)
        self.pbar.setValue(1)

class RokeTableView(TableView):

    def onMouseDoubleClick(self, index):
        row = index.data(RowValueRole)
        dir_path = row[0]
        file_name = row[1]
        path = os.path.join(dir_path, file_name)
        args = ["xdg-open", path]
        subprocess.run(args)

class MainWindow(QMainWindow):
    def __init__(self):
        super(MainWindow, self).__init__()

        self.setWindowTitle("Roke")

        if sys.platform == "darwin":
            self.setUnifiedTitleAndToolBarOnMac(True)

        self.config_dir = default_config_dir()
        self.config = RokeConfig.getConfig(self.config_dir)

        self.btn_mode = QComboBox(self)
        self.btn_mode.addItem("Default", ROKE_DEFAULT)
        self.btn_mode.addItem("Glob", ROKE_GLOB)
        self.btn_mode.addItem("Regex", ROKE_REGEX)
        self.btn_mode.currentIndexChanged.connect(self.onSearchModeChanged)
        self.btn_mode.setToolTip("Select the search algorithm to use")

        self.edit = QLineEdit(self)
        self.edit.returnPressed.connect(self.onReturnPressed)
        self.edit.setToolTip("Enter a search term")

        self.btn_search = QPushButton("Search", self)
        self.btn_search.clicked.connect(self.onReturnPressed)
        self.btn_search.setToolTip("Execute a search")

        self.toolbar = QHBoxLayout()
        #self.toolbar.setMovable(False)
        #self.toolbar.setToolButtonStyle(Qt.ToolButtonStyle.ToolButtonIconOnly)
        #self.toolbar.setStyleSheet('QToolBar{spacing:4px;}')
        self.toolbar.layout().setContentsMargins(16, 4, 16, 4)
        self.toolbar.layout().setSpacing(8)
        #self.addToolBar(self.toolbar)

        #self.toolbar.addWidget(self.btn_mode)
        self.toolbar.addWidget(QLabel(self))

        self.btn_options = QPushButton(self._makeOptionsIcon(), "", self)
        self.btn_options.clicked.connect(self.showOptionDialog)

        self.btn_case = QPushButton(self._makeCaseIcon(), "", self)
        self.btn_case.clicked.connect(self.onSearchCaseClicked)
        self.btn_case.setCheckable(True)

        # self.btn_options = self.toolbar.addAction(self._makeOptionsIcon(), "Options",
        #     self.showOptionDialog)
        # self.btn_options.setToolTip("Manage Roke Options")

        # self.btn_case = self.toolbar.addAction(self._makeCaseIcon(), "Case",
        #     self.onSearchCaseClicked)
        # self.btn_case.setToolTip("Toggle case-insensitive search")
        # self.btn_case.setCheckable(True)

        self.toolbar.addWidget(self.btn_options)
        self.toolbar.addWidget(self.btn_case)
        self.toolbar.addWidget(self.btn_mode)
        self.toolbar.addWidget(self.edit)
        self.toolbar.addWidget(self.btn_search)
        self.toolbar.addWidget(QLabel(self))

        self.table = RokeTableView(self)
        self.table.setColumnHeaderClickable(True)
        self.table.setLastColumnExpanding(True)
        self.table.setSortingEnabled(True)
        self.table.setVerticalHeaderVisible(False)
        self.table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
        self.table.MouseReleaseRight.connect(self.onShowContextMenu)
        model = self.table.baseModel()
        model.addColumn(1, "Name", editable=False)
        model.addColumn(0, "Path", editable=False)

        self.centralWidget = QWidget(self)

        self.vbox = QVBoxLayout(self.centralWidget)
        self.vbox.addLayout(self.toolbar)
        self.vbox.addWidget(self.table)
        # l t r b
        self.vbox.setContentsMargins(4,0,4,0)

        self.setCentralWidget(self.centralWidget)

        self.statusbar = QStatusBar(self)
        self.statusbar_count = QLabel("", self)
        self.statusbar_path = QLabel("", self)
        self.statusbar.addWidget(self.statusbar_count)
        self.statusbar.addWidget(self.statusbar_path)

        self.statusbar_path.setText(self.config_dir)

        self.setStatusBar(self.statusbar)
        self.resize(60 * 16, 60 * 9)

        self.updateFromConfig()

    def _makeOptionsIcon(self):
        # no icon? make an icon!
        # 3 horizontal bars, "hamburger icon" to open the options
        pixmap = QPixmap(64,64)
        pixmap.fill(QColor(0,0,0,0))
        painter = QPainter(pixmap)
        font = painter.font()
        font.setPixelSize(int(pixmap.height()*.75))
        painter.setFont(font)
        h=int(pixmap.height()/5)
        path= QPainterPath();
        path.addRoundedRect(0,0,pixmap.width(),h,h/2,h/2)
        path.addRoundedRect(0,h*2,pixmap.width(),h,h/2,h/2)
        path.addRoundedRect(0,h*4,pixmap.width(),h,h/2,h/2)
        scale = 0.8
        xt = (1-scale)*pixmap.height() / 2
        t = QTransform().translate(xt, xt).scale(scale,scale)
        painter.setTransform(t)
        painter.fillPath(path, Qt.GlobalColor.black)
        painter.drawPath(path)
        painter.end()
        return QIcon(pixmap)

    def _makeCaseIcon(self):
        # no icon? make an icon!
        # an icon with Aa inscribed -- use case-insensitive searching
        pixmap = QPixmap(64,64)
        pixmap.fill(QColor(0,0,0,0))
        painter = QPainter(pixmap)
        font = painter.font()
        font.setPixelSize(int(pixmap.height()*.75))
        painter.setFont(font)
        painter.drawText(0,0,pixmap.width(), pixmap.height(),Qt.AlignmentFlag.AlignHCenter|Qt.AlignmentFlag.AlignVCenter,"Aa")
        painter.end()
        return QIcon(pixmap)

    def updateFromConfig(self):
        flags = self.config.getSearchFlags()

        with QSignalBlocker(self.btn_case):
            self.btn_case.setChecked(flags&ROKE_CASE_INSENSITIVE)

        with QSignalBlocker(self.btn_mode):
            if flags&ROKE_REGEX:
                self.btn_mode.setCurrentIndex(2)
            elif flags&ROKE_GLOB:
                self.btn_mode.setCurrentIndex(1)
            else:
                self.btn_mode.setCurrentIndex(0)

    def showOptionDialog(self):

        dialog = OptionsDialog(self.config_dir, self.config)
        if dialog.exec():
            self.config.setSearchLimit(dialog.getSearchLimit())
            self.updateFromConfig()

    def onReturnPressed(self):

        text = self.edit.text()

        gen_results = find(self.config_dir, [text],
            self.config.getSearchFlags(),
            self.config.getSearchLimit())
        data = [os.path.split(p) for p in gen_results]
        self.table.setNewData(data)

        self.table.resizeColumnToContents(0)

        self.statusbar_count.setText(" %d files " % len(data))

    def onShowContextMenu(self, event):

        menu = ContextMenu(self)
        menu.add("Open", self.actionOpenPath)
        menu.addSeparator()
        menu.add("Copy File Name", self.actionCopyName)
        menu.add("Copy File Path", self.actionCopyPath)
        menu.exec(event.globalPos())

    def onSearchCaseClicked(self, checked=False):
        flags = 0
        if self.btn_case.isChecked():
            flags |= ROKE_CASE_INSENSITIVE

        flags |= self.btn_mode.currentData()

        self.config.setSearchFlags(flags)

    def onSearchModeChanged(self, index):

        flags = 0
        if self.btn_case.isChecked():
            flags |= ROKE_CASE_INSENSITIVE

        flags |= self.btn_mode.itemData(index)

        self.config.setSearchFlags(flags)

    def actionOpenPath(self):
        for path, name in self.table.getSelectedRowData():
            filepath = os.path.join(path, name)
            open_native(filepath)

    def actionCopyName(self):
        for path, name in self.table.getSelectedRowData():
            QApplication.clipboard().setText(name)

    def actionCopyPath(self):

        for path, name in self.table.getSelectedRowData():
            QApplication.clipboard().setText(path)

    def closeEvent(self, event):
        sys.stdout.write("Closing Application\n")

        self.config.save()

def handle_exception(exc_type, exc_value, exc_traceback):
    for line in traceback.format_exception(exc_type, exc_value, exc_traceback):
        sys.stderr.write(line)

def main():

    app = QApplication(sys.argv)

    app.setQuitOnLastWindowClosed(True)

    window = MainWindow()
    window.show()

    sys.excepthook = handle_exception

    sys.exit(app.exec())

if __name__ == '__main__':
    main()
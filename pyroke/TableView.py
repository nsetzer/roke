
import sys
import math
from PyQt6.QtCore import *
from PyQt6.QtWidgets import *
from PyQt6.QtGui import *

# for testing
import string
import random
import traceback
import base64
import zlib
# http://pyqt.sourceforge.net/Docs/PyQt4/qt.html#ItemDataRole-enum

RowValueRole = Qt.ItemDataRole.UserRole + 1000
RowValueIndexRole = Qt.ItemDataRole.UserRole + 1001
RowSortValueRole = Qt.ItemDataRole.UserRole + 1002

class TableError(Exception):
    pass

class StyledItemDelegate(QStyledItemDelegate):

    def paintInit(self, painter, option, index):
        try:

            style = QApplication.style()
            # if (option.state & QStyle.State_Selected):
            # Whitee pen while selection
            pen = painter.pen()
            brush = painter.brush()

            if (option.state & QStyle.State.State_Selected):
                #pen = index.data(Qt.ItemDataRole.ForegroundRole)
                # if pen is not None:
                #    painter.setPen(pen)
                # else:
                painter.setPen(Qt.GlobalColor.white)
                painter.setBrush(option.palette.highlightedText())
            else:
                #painter.setPen(QPen(option.palette.color(QPalette.Foreground), 0));
                # set the pen to a custom color, or reset the pen to default
                pen   = index.data(Qt.ItemDataRole.ForegroundRole)
                if pen is not None:
                    painter.setPen(pen)
                else:
                    painter.setPen(QPen(option.palette.color(QPalette.ColorRole.WindowText), 0))
                # draw the background color
                brush = index.data(Qt.ItemDataRole.BackgroundRole)
                if brush is not None:
                    painter.fillRect(option.rect, brush)

            # This call will take care to draw, dashed line while selecting
            style.drawControl(QStyle.ControlElement.CE_ItemViewItem, option, painter, option.widget)
            # else:

            #painter.setPen(QPen(option.palette.color(QPalette.Foreground), 0));
            #brush = index.data(Qt.ItemDataRole.ForegroundRole)
            # if brush is not None:
            #    painter.setBrush(brush);
            # painter.setPen(pen)
            # painter.setBrush(brush)
            return
        except BaseException as e:
            print("caught error in paintInit", e)

class ComboBoxTextDelegate(StyledItemDelegate):

    def __init__(self, parent, slItems):
        super(ComboBoxTextDelegate, self).__init__(parent)

        self.slItems = slItems

    def createEditor(self, parent, option, index):
        combo = QComboBox(parent)
        for item in self.slItems:
            combo.addItem(item)
        return combo

    def setEditorData(self, editor, index):
        x = str(index.data())
        editor.setCurrentText(x)

    def setModelData(self, editor, model, index):
        model.setData(index, editor.currentText())

class DualTextDelegate(StyledItemDelegate):
    """
    display text on the right and left side of a column cell
    left text is taken from the data model for the given column
    right text it generated using a transform function
        this function is given an index object

    """

    def __init__(self, rTransform, parent):
        """
        rTransform : lambda index -> QVariant
        """
        super(DualTextDelegate, self).__init__(parent)
        self.flagsL = Qt.AlignmentFlag.AlignVCenter | Qt.AlignmentFlag.AlignLeft
        self.flagsR = Qt.AlignmentFlag.AlignVCenter | Qt.AlignmentFlag.AlignRight
        self.rTransform = rTransform

    def paint(self, painter, option, index):

        textL = str(index.data(Qt.ItemDataRole.DisplayRole))
        textR = str(self.rTransform(index))

        self.paintInit(painter, option, index)

        painter.drawText(option.rect, self.flagsR, textR)

        fm = QFontMetrics(painter.font())
        w = fm.boundingRect(textR).width()
        # prevent left text from overlapping right text
        clipRect = option.rect
        clipRect.setWidth(max(0, clipRect.width() - w))
        painter.drawText(option.rect, self.flagsL, textL)

class ImageDelegate(StyledItemDelegate):
    """
    display text on the right and left side of a column cell
    left text is taken from the data model for the given column
    right text it generated using a transform function
        this function is given an index object

    """

    def __init__(self, parent):
        super(ImageDelegate, self).__init__(parent)

        self.trasformMode = Qt.TransformationMode.SmoothTransformation  # FastTransformation

    def paintImageCentered(self, painter, rect, image):

        rect = QRect(rect)
        size = QSize(rect.width(), rect.height())

        # shrink the image if we need to
        if image.width() > rect.width() or image.height() > rect.height():
            #Qt.IgnoreAspectRatio, Qt.KeepAspectRatio, Qt.KeepAspectRatioByExpanding
            aspect = Qt.AspectRatioMode.KeepAspectRatio
            mode = Qt.TransformationMode.SmoothTransformation
            image = image.scaled(size, aspect, self.trasformMode)

        # center horizontally
        if image.width() < rect.width():
            dw = rect.width() - image.width()
            rect.setX(rect.x() + dw // 2)
            rect.setWidth(image.width())

        # center vertically
        if image.height() < rect.height():
            dh = rect.height() - image.height()
            rect.setY(rect.y() + dh // 2)
            rect.setHeight(image.height())

        painter.drawImage(rect, image)

    def paint(self, painter, option, index):

        self.paintInit(painter, option, index)

        image = index.data(Qt.ItemDataRole.DisplayRole)

        if image is None:
            return

        if not isinstance(image, (QImage,)):
            painter.drawText(option.rect, 0, "Error:" + str(image))
            return

        self.paintImageCentered(painter, option.rect, image)

class StarRating(QObject):
    """ handles rendering the editable stars """

    ModeEdit = 1
    ModeDisplay = 2

    scaleFactor = 20

    def __init__(self, parent=None):
        super(StarRating, self).__init__(parent)

        self.num_stars = 0
        self.max_stars = 5

        rotation = .33
        points = []
        for i in reversed(list(range(0, 11))):
            # if you want the star centered at a point,
            # remove the +1 from the points
            w = 2 * (i + rotation) * math.pi / 10
            r = (i % 2 + 1) / 2  # every other point has max or min radius set
            points.append((1.0 + r * math.sin(w), 1.0 + r * math.cos(w)))

        self.points = points
        print(["%.1f,%.1f" % p for p in points])
        self.polyStar = QPolygonF([QPointF(*x) for x in points])

        path = QPainterPath(self.scaleFactor * QPointF(*self.points[0]))
        for p in self.points[1:]:
            path.lineTo(self.scaleFactor * QPointF(*p))
        self.pathStar = path

        points = [(1.0, 0.5), (1.75, 1.0), (1.0, 1.5), (0.25, 1.0), ]
        self.pointsDiamond = points

    def setValue(self, value):
        self.num_stars = value

    def value(self):
        return self.num_stars

    def sizeHint(self):
        return self.scaleFactor * QSize(self.max_stars, 1)

    def paint(self, painter, rect, palette, mode):

        painter.save()

        painter.setRenderHint(QPainter.RenderHint.Antialiasing, True)
        # painter.setPen(Qt.NoPen)

        pen = QPen(QColor(0, 128, 128))
        pen.setWidth(1)
        painter.setPen(pen)
        # painter.setBrush(QColor(0,0,255))
        if mode == StarRating.ModeEdit:
            painter.setBrush(palette.highlight())
        else:
            painter.setBrush(palette.color(QPalette.ColorRole.WindowText))

        scaleFactorY = rect.height() * 0.8 / 2
        scaleFactorX = (rect.width() / 5) * 0.8 / 2
        scaleFactor = min(scaleFactorX, scaleFactorY)
        xOffset = (rect.width() - 2 * 5 * scaleFactor) / 2
        yOffset = (rect.height() - 2 * scaleFactor) / 2
        painter.translate(rect.x(), rect.y())
        painter.translate(xOffset, yOffset)

        path = QPainterPath(scaleFactor * QPointF(*self.points[0]))
        for p in self.points[1:]:
            path.lineTo(scaleFactor * QPointF(*p))
        pathStar = path

        path = QPainterPath(scaleFactor * QPointF(*self.pointsDiamond[0]))
        for p in self.pointsDiamond[1:]:
            path.lineTo(scaleFactor * QPointF(*p))
        pathDiamong = path

        for i in range(self.max_stars):
            if i < self.num_stars:
                painter.fillPath(pathStar, painter.brush())
            else:
                painter.fillPath(pathDiamong, painter.brush())
            painter.translate(2 * scaleFactor, 0.0)

        painter.restore()

class StarEdit(QWidget):
    """ an edit widget for editing stars """
    # emit editingFinished();

    editingFinished = pyqtSignal()

    def __init__(self, parent=None):
        super(StarEdit, self).__init__(parent)
        self._value = 0
        self.starrating = StarRating(self)

        self.setMouseTracking(True)
        self.setAutoFillBackground(True)

    def setValue(self, value):
        self._value = value

    def value(self):
        return self._value

    def sizeHint(self):
        return self.starrating.sizeHint()

    def paintEvent(self, event):
        painter = QPainter(self)
        self.starrating.paint(painter, self.rect(), self.palette(), StarRating.ModeDisplay)

    def mouseMoveEvent(self, event):

        print("StarEdit mouseMoveEvent", event.position())

    def mouseReleaseEvent(self, event):
        self.editingFinished.emit()

    def starAtPosition(self, x):
        return 0

class StarDelegate(StyledItemDelegate):

    def __init__(self, parent=None):
        super(StarDelegate, self).__init__(parent)

        self.starrating = StarRating(self)

    def sizeHint(self):
        return self.starrating.sizeHint()

    def paint(self, painter, option, index):

        self.starrating.setValue(index.data())
        self.starrating.paint(painter, option.rect, option.palette, StarRating.ModeDisplay)

    def createEditor(self, parent, option, index):
        edit = StarEdit(parent)
        return edit

    def setEditorData(self, editor, index):
        editor.setValue(int(index.data()))

    def setModelData(self, editor, model, index):
        v = editor.value()
        model.setData(index, v)

class EditItemDelegate(StyledItemDelegate):
    """
    This is a hakc since ::EditNextItem and ::EditPreviousItem are not working
    This preserves the index that is being modified, then intercepts
    key presses to determines when the next index should be modified

    with no model reference, it emits logical row/col instead of a QModelIndex
    """

    editRow = pyqtSignal(int, int)
    openEditor = pyqtSignal(object)
    editorFinished = pyqtSignal(object, object, object, object)  # index, EndEditHint

    persistData = pyqtSignal(object, str, str)

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._index = None

    def eventFilter(self, editor, event):

        try:
            type_ = event.type()
        except ValueError:  # qt bug (type_ = int 213, not an enum)
            return super().eventFilter(editor, event)

        if type_ == QEvent.Type.KeyPress:
            key = Qt.Key(event.key()) # qt6 returns int, cast back to enum
            if key == Qt.Key.Key_Down or event.key() == Qt.Key.Key_Tab:
                self.commitData.emit(editor)
                print("emit close editor down")
                self.closeEditor.emit(editor, QAbstractItemDelegate.EndEditHint.EditNextItem)
                #self.editRow.emit(self._index.row()+1, self._index.column())
                event.accept()
                return True

            elif key == Qt.Key.Key_Up or event.key() == Qt.Key.Key_Backtab:
                self.commitData.emit(editor)
                print("emit close editor up")
                self.closeEditor.emit(editor, QAbstractItemDelegate.EndEditHint.EditPreviousItem)
                #self.editRow.emit(self._index.row()-1, self._index.column())
                event.accept()
                return True

            elif key == Qt.Key.Key_Home:
                editor.setCursorPosition(0)
                event.accept()
                return True

            elif key == Qt.Key.Key_End:
                editor.setCursorPosition(len(editor.text()))
                event.accept()
                return True
            elif key == Qt.Key.Key_Return or key == Qt.Key.Key_Enter:
                super().eventFilter(editor, event)
                return True

        return super().eventFilter(editor, event)

    def createEditor(self, parent, option, index):
        self._index = index
        new_value = str(index.data())

        edit = QLineEdit(parent)
        edit.setText(new_value)
        self.openEditor.emit(edit)
        return edit

    def setEditorData(self, editor, index):
        new_value = str(index.data())
        row_info = index.data(RowValueRole)
        row_idx = index.data(RowValueIndexRole)

        editor.setText(new_value)
        self._finished = False

    def setModelData(self, editor, model, index):
        org_value = str(index.data())
        new_value = editor.text()
        if org_value != new_value:
            row_info = index.data(RowValueRole)
            row_idx = index.data(RowValueIndexRole)
            self.persistData.emit(index, org_value, new_value)
            # self.parent().baseModel()
            check_index = model.index(index.row(), index.column())
            check_info = check_index.data(RowValueRole)
            check_idx = check_index.data(RowValueIndexRole)
            if row_info is not check_info:
                print("!! bug")
                column = index.column()
                index = None
                for i in range(model.rowCount(QModelIndex())):
                    check_index = model.index(i, column)
                    if check_index.data(RowValueRole) is row_info:
                        index = check_index
                        print("!! fixed")
                if index is None:
                    # looks like _persist_create is running too early
                    print("fix failed")
                    return
            model.setData(index, editor.text())

class TableColumn(QObject):
    def __init__(self, model, key, name, editable=False):
        # parent should be the model
        super(TableColumn, self).__init__(model)

        self._key = key
        self._decoration_key = None
        self._name = name
        self.__cached_name = None
        self.__cached_name_length = 0
        self._shortName = ""
        self._editable = editable
        self._text_alignment = Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter

        self._sort_transform = None

        self._display_name = None

        self._default_width = 100
        self._width_initialized = False

    def key(self):
        return self._key

    def name(self):
        return self._name

    def displayName(self, width=0):

        if self._name != self.__cached_name:
            # TODO need to come up with a better way to handle margins
            fm = QFontMetrics(QApplication.font())
            self.__cached_name_length = fm.boundingRect("   " + self._name + "   ").width()
            self.__cached_name = self._name
        x = self.__cached_name_length
        if 0 < width < x and self._shortName:
            return self._shortName

        if self._display_name is not None:
            return self._display_name

        return self._name

    def setName(self, name):
        self._name = name

    def setShortName(self, name):
        self._shortName = name

    def setDecorationKey(self, key):
        self._decoration_key = key

    def isEditable(self):
        return self._editable

    def data(self, tabledata, row, role=Qt.ItemDataRole.DisplayRole):
        _data = tabledata[row]

        if isinstance(_data, dict) and self._key not in _data:
            return None

        if role == Qt.ItemDataRole.DecorationRole:
            if self._decoration_key is not None:
                return _data[self._decoration_key]
            return None
        else:
            return _data[self._key]

    def setData(self, tabledata, row, value):
        print("set column data", row, value)
        tabledata[row][self._key] = value
        return True

    def select(self, rowdata):
        return rowdata[self._key]

    def setDefaultTextAlignment(self, alignment):
        self._text_alignment = alignment

    def textAlignment(self, index):
        return self._text_alignment

    def setSortTransform(self, transform):

        self._sort_transform = transform

    def sortValue(self, tabledata, row):
        if self._sort_transform is not None:
            return self._sort_transform(tabledata, row)
        return self.data(tabledata, row)

    def setDisplayName(self, name):
        self._display_name = name

    def setDefaultWidth(self, width):
        self._default_width = width

    def defaultWidth(self):
        return self._default_width

    def initWidth(self, parent, section):

        if not self._width_initialized:
            parent.setColumnWidth(section, self._default_width)
            self._width_initialized = True

class TableListColumn(TableColumn):
    """
    a keyless TableColumn for displaying List Data
    """

    def __init__(self, model, name, editable=False):
        # parent should be the model
        super(TableListColumn, self).__init__(model, None, name, editable=editable)

    def data(self, tabledata, row, role):
        return tabledata[row]

    def select(self, rowdata):
        return rowdata

class TransformTableColumn(TableColumn):
    def __init__(self, model, key, name, fTransform, rTransform=None):
        """
        fTransform : fn(tabledata,row,key) -> QVariant
        rTransform : fn(QVariant) -> QVariant

        forward transform can be used to format the data in the cell
        reverse transform is used to set the data for a cell given
            input from a user

        todo: forward/backward transformation for editng
        """
        editable = rTransform is not None
        super(TransformTableColumn, self).__init__(model, key, name, editable=editable)
        self.fTransform = fTransform
        self.rTransform = rTransform

    def data(self, tabledata, row, role=Qt.ItemDataRole.DisplayRole):
        return self.fTransform(tabledata, row, self._key)

    def setData(self, tabledata, row, value):
        try:
            tabledata[row][self._key] = self.rTransform(tabledata, row, self._key, value)
        except Exception as e:
            print(type(tabledata))
            print(type(tabledata[row]))
            print("error setting transform result")
            print(e)
            return False
        return True

class TransformRule(QObject):
    """docstring for TransformRule

        A transform rule returns a value given the
        result of some function on the data

        name: unique identifier for this rule
        transform : fn(index,col) -> QVariant
            index: QModelIndex
            col  : TableColumn

    """

    def __init__(self, parent, name, transform):
        super(TransformRule, self).__init__(parent)
        self.transform = transform
        self.name = name

    def apply(self, index, col):
        return self.transform(index, col)

class TableModel(QAbstractTableModel):

    def __init__(self, parent):
        super(TableModel, self).__init__(parent)

        self.tabledata = []

        self._columns = []

        self._forgroundRules = []
        self._backgroundRules = []

    def rowCount(self, index):

        if index is None or not index.isValid():
            return len(self.tabledata)

        return 0

    def columnCount(self, index):

        if index is None or not index.isValid():
            return len(self._columns)

        return len(self._columns)

    def columnIndex(self, key):
        """ returns the index for a column, using the key as a look up
        not a 1-to-1 mapping,not invertible, can't be implemented
        a user can define two columns with the same key and differnt transforms
        """
        raise NotImplementedError()

    def data(self, index, role):


        if not index.isValid():
            return QVariant()
        
        i = index.row()
        j = index.column()
        col = self._columns[j]

        #print("data", i, j, col)

        if self.parent().state()==3 and role == Qt.ItemDataRole.EditRole:
            print("data get edit role", self.tabledata[i], i, j)
            # self._xy = (i, j)

        #if hasattr(self, '_xy') and self._xy == (i, j):
        #    print(role)

        if role == Qt.ItemDataRole.DisplayRole or role == Qt.ItemDataRole.EditRole or role == Qt.ItemDataRole.DecorationRole:
            data = col.data(self.tabledata, i, role)
            #print(i,j,  role, data)
            return data
        elif role == Qt.ItemDataRole.TextAlignmentRole:
            return col.textAlignment(index)
        elif role == RowValueIndexRole:
            return i
        elif role == RowValueRole:
            return self.tabledata[i]
        elif role == RowSortValueRole:
            return col.sortValue(self.tabledata, i)
        elif role == Qt.ItemDataRole.ForegroundRole:
            # return QBrush(QColor(0,0,255))
            for rule in self._forgroundRules:
                result = rule.apply(index, col)
                if result is not None:
                    return result
        elif role == Qt.ItemDataRole.BackgroundRole:
            for rule in self._backgroundRules:
                result = rule.apply(index, col)
                if result is not None:
                    return result
        return None

    def setData(self, index, value, role=Qt.ItemDataRole.EditRole):

        if role != Qt.ItemDataRole.EditRole:
            return False

        if index.row() > len(self.tabledata) or index.row() < 0:
            return False

        #if not self.parent().onCommitValidateData(index, value):
        #    return False

        i = index.row()
        j = index.column()
        c = self._columns[j]
        k = c.key()
        success = self.setModelData(i, c, value)
        if success:
            self.dataChanged.emit(index, index, [role, ])

        return success

    def headerData(self, section, orientation, role):

        view = self.parent()

        orientation = Qt.Orientation(orientation)
        role = Qt.ItemDataRole(role)

        if orientation == Qt.Orientation.Horizontal:
            if role == Qt.ItemDataRole.DisplayRole:
                return self._columns[section].displayName(view.columnWidth(section))
            elif role == Qt.ItemDataRole.SizeHintRole:
                self._columns[section].initWidth(self.parent(), section)

        else:
            if role == Qt.ItemDataRole.DisplayRole:
                return section + 1

        return super().headerData(section, orientation, role)

        # print(section, orientation, role)

    def flags(self, index):
        if not index.isValid():
            return Qt.ItemFlags.NoItemFlags

        i = index.row()
        j = index.column()
        c = self._columns[j]
        k = c.key()

        flags = Qt.ItemFlag.ItemIsEnabled | Qt.ItemFlag.ItemIsSelectable
        if c.isEditable():
            flags = flags | Qt.ItemFlag.ItemIsEditable

        return flags

    def _x_parent(self, index=None):
        
        if index is None: # return the parent widget
            return super().parent()

        if not index.isValid():
            return QModelIndex()

        item = index.internalPointer()

        print("get parent", index.row())
        # 
        # return self.createIndex(item->parent->row, col, item)
        return QModelIndex()


    def _x_index(self, row, col, parent=None):
        if col < 0 or col >= len(self._columns):
            return QModelIndex()
        if row < 0 or row >= len(self.tabledata):
            return QModelIndex()
        if parent.isValid():
            item = parent.internalPointer()
        else:
            item = None
        return self.createIndex(row, col, item)
    # -----------------------------------------------------------------------
    # Unimplemented
    # these methods could be implemented depending on the model
    # however the default use-case does not generalize to removing data

    #def removeColumn(self, idx, parent=QModelIndex()):
    #    super().removeColumn(idx, 1, parent)

    def removeColumns(self, idx, count, parent=QModelIndex()):
        raise TableError("not implemented")

    def removeRow(self, idx, parent=QModelIndex()):

        if idx < 0 or idx > len(self.tabledata):
            return False

        self.beginRemoveRows(parent, idx, idx)
        self.tabledata.pop(idx)
        self.endRemoveRows()
        return True

    def removeRows(self, idx, count, parent=QModelIndex()):

        s = idx
        e = idx + count

        if s < 0 or e > len(self.tabledata):
            return False

        self.beginRemoveRows(parent, idx, idx+count-1)

        if s == 0:
            self.tabledata = self.tabledata[e:]
        elif e >= len(self.tabledata)-1:
            self.tabledata = self.tabledata[:e]
        else:
            self.tabledata = self.tabledata[:s] + self.tabledata[e:]

        self.endRemoveRows()

        return True

    # -----------------------------------------------------------------------
    # helper functions

    def setModelData(self, rowidx, col, value):
        """ reimplement to enable editing data

        rowidx : index of the row to modify (integer)
        col    : column instance (TableColumn)
        """
        # unclear if this should update the model, or if it should return
        # true and have the caller update the model. depends on usecase
        # with some real examples
        return col.setData(self.tabledata, rowidx, value)

    def setNewData(self, tabledata):

        self.beginResetModel()
        self.tabledata = tabledata
        self.endResetModel()

    def replaceRow(self, row, rowdata):

        indexFrom = self.index(row, 0)
        indexTo = self.index(row, len(self._columns)-1)

        self.tabledata[row] = rowdata

        role=Qt.ItemDataRole.EditRole
        self.dataChanged.emit(indexFrom, indexTo, [role, ])

    def insertRow(self, row, rowdata):
        index = QModelIndex()  # self.createIndex(row, 0)
        self.beginInsertRows(index, row, row)
        self.tabledata.insert(row, rowdata)
        self.endInsertRows()

    def appendRows(self, rows):
        index = QModelIndex()  # self.createIndex(row, 0)
        row = self.rowCount(index)
        start = row
        end = row+len(rows)-1
        self.beginInsertRows(index, row, end)
        for rowdata in rows:
            self.tabledata.insert(row, rowdata)
            row += 1
        self.endInsertRows()

    def forceReset(self):
        """force all cells to be repainted"""
        tl = self.index(0, 0)
        br = self.index(self.rowCount(tl) - 1, self.columnCount(tl) - 1)
        self.dataChanged.emit(tl, br, [Qt.ItemDataRole.DisplayRole, Qt.ItemDataRole.ForegroundRole, Qt.ItemDataRole.BackgroundRole])

    def getData(self):
        """ returns the underlying data source for this model """
        return self.tabledata

    def getColumn(self, idx):
        return self._columns[idx]

    def getColumnIndexByName(self, name):
        for idx, col in enumerate(self._columns):
            if col._name == name:
                return idx
        raise KeyError(name)

    def addColumn(self, key, name, editable=False):
        """ returns the index of the new column """
        col = TableColumn(self, key, name, editable=editable)
        self._columns.append(col)
        return len(self._columns) - 1

    def addTableColumn(self, col):
        """ returns the index of the new column """
        self._columns.append(col)
        return len(self._columns) - 1

    def addTransformColumn(self, key, name, fTransform, rTransform=None):
        """ returns the index of the new column
        fTransform : fn(tabledata, row, key) -> QVariant
        rTransform : fn(QVariant) -> QVariant
        """
        col = TransformTableColumn(self, key, name, fTransform, rTransform)
        self._columns.append(col)
        return len(self._columns) - 1

    def addForegroundRule(self, name, transform):
        """
        name: the name of the rule
                (unused, could be used to remove)
        transform: the transform function
        """
        self._forgroundRules.append(TransformRule(self, name, transform))

    def addBackgroundRule(self, name, transform):
        """
        name: the name of the rule
                (unused, could be used to remove)
        transform: the transform function
        """
        self._backgroundRules.append(TransformRule(self, name, transform))

    def getColumn(self, j):
        return self._columns[j]

    def setColumnShortName(self, idx, name):
        self.getColumn(idx).setShortName(name)

class SortProxyModel(QSortFilterProxyModel):
    def sort(self, column, order=Qt.SortOrder.AscendingOrder):
        """reimplement to disable sorting for specific columns

        DescendingOrder, AscendingOrder
        column: integer column index
        order : the sorting direction

        an example reimplementation would invert sort order for
        specific columns, based on a boolean embedded in TableColumn
        """
        super().sort(column, order)

class AbstractHeaderView(QHeaderView):

    showContextMenu = pyqtSignal(object)  # event

    def __init__(self, orientation, parent=None):
        super(AbstractHeaderView, self).__init__(orientation, parent)

        #self.setSectionsClickable(True)
        #self.setHighlightSections(True)

        self.sectionResized.connect(self.onSectionResized)

    def onSectionResized(self, index, old, new):
        #traceback.print_stack()
        #print("resize", index, old, new)
        pass

    def defaultSectionSize(self):
        return 400

    def mousePressEvent(self, event):
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):

        if event.button() == Qt.MouseButton.RightButton:
            self.showContextMenu.emit(event)
        else:
            super().mouseReleaseEvent(event)

class AbstractTableView(QTableView):

    MouseDoubleClick = pyqtSignal(QModelIndex)
    MouseReleaseRight = pyqtSignal(object)
    MouseReleaseMiddle = pyqtSignal(object)
    MouseReleaseBack = pyqtSignal(object)
    MouseReleaseForward = pyqtSignal(object)
    selectionChangedEvent = pyqtSignal()
    rowChanged = pyqtSignal(int)  # index of the row that changed

    def __init__(self, parent):
        super(AbstractTableView, self).__init__(parent)

        self._baseModel = None
        # SelectItems, SelectRows, SelectColumns
        # SingleSelection, ContiguousSelection, ExtendedSelection, MultiSelection, NoSelection
        self.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.setSelectionMode(QAbstractItemView.SelectionMode.ExtendedSelection)
        self.setAlternatingRowColors(True)
        # TabFocus ClickFocus StrongFocus WheelFocus NoFocus
        #self.setFocusPolicy(Qt.ClickFocus );

        header = AbstractHeaderView(Qt.Orientation.Horizontal, self)
        header.showContextMenu.connect(self.onShowHeaderContextMenu)
        self.setHorizontalHeader(header)

        #  ScrollPerItem, ScrollPerPixel
        self.setHorizontalScrollMode(QAbstractItemView.ScrollMode.ScrollPerPixel)

        self.verticalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Fixed)

        self.MouseDoubleClick.connect(self.onMouseDoubleClick)
        self.MouseReleaseRight.connect(self.onMouseReleaseRight)
        self.MouseReleaseMiddle.connect(self.onMouseReleaseMiddle)
        self.MouseReleaseBack.connect(self.onMouseReleaseBack)
        self.MouseReleaseForward.connect(self.onMouseReleaseForward)
        self.horizontalHeader().sectionClicked.connect(self.onHeaderClicked)

        self._edit_index = None  # TODO: remove?
        self._mouse_position_start = QPointF(-1,-1)  # TODO: remove?

    def selectionChanged(self, *args):
        super().selectionChanged(*args)
        self.selectionChangedEvent.emit()

    def mouseDoubleClickEvent(self, event):
        index = self.indexAt(event.position().toPoint())
        col = self.baseModel().getColumn(index.column())
        triggers = self.editTriggers()&QAbstractItemView.EditTrigger.DoubleClicked

        if event.button() != Qt.MouseButton.LeftButton:
            return

        if not (col.isEditable() and triggers != QAbstractItemView.EditTrigger.NoEditTriggers ):
            self.MouseDoubleClick.emit(index)
        else:
            super(AbstractTableView, self).mouseDoubleClickEvent(event)

    def mousePressEvent(self, event):
        #index = self.indexAt(event.position())
        # print(index.row(),index.column())
        #if event.button() != Qt.LeftButton:
        #    return

        #if event.button() == Qt.MouseButton.BackButton:
        #    pass
        #elif event.button() == Qt.MouseButton.ForwardButton:
        #    pass
        #else:
        super().mousePressEvent(event)
        self._mouse_position_start = event.position()

    def mouseMoveEvent(self, event):
        self._mouse_position = event.position()

        distance = (self._mouse_position - self._mouse_position_start).manhattanLength()
        if distance > QApplication.startDragDistance():
            self.onDragBegin()

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MouseButton.RightButton:
            self.MouseReleaseRight.emit(event)
            return
        elif event.button() == Qt.MouseButton.MiddleButton:
            self.MouseReleaseMiddle.emit(event)
            return
        elif event.button() == Qt.MouseButton.BackButton:
            print("go back")
            self.MouseReleaseBack.emit(event)
            return
        elif event.button() == Qt.MouseButton.ForwardButton:
            print("go forward")
            self.MouseReleaseForward.emit(event)
            return

        super(AbstractTableView, self).mouseReleaseEvent(event)

    def keyPressEvent(self, event):

        """
        if self._edit_index:
            if self.isPersistentEditorOpen(self._edit_index):

                if event.key() in (Qt.Key_Down, Qt.Key_Tab):
                    row = self._edit_index.row()
                    col = self._edit_index.column()
                    row += 1
                    if row < self.rowCount():
                        self.closeEditor(QAbstractItemDelegate.SubmitModelCache)
                        index = self.model().index(row, col)
                        self.edit(index)
                        self._edit_index = index
                        return
                elif event.key() == Qt.Key_Up:
                    row = self._edit_index.row()
                    col = self._edit_index.column()
                    row -= 1
                    if row >= 0:
                        self.closeEditor(QAbstractItemDelegate.SubmitModelCache)
                        index = self.model().index(row, col)
                        self.edit(index)
                        self._edit_index = index
                        return
        """

        super(AbstractTableView, self).keyPressEvent(event)

    def keyReleaseEvent(self, event):
        super(AbstractTableView, self).keyReleaseEvent(event)

    # -----------------------------------------------------------------------
    # public slots

    def onShowHeaderContextMenu(self, event):

        # index = self.horizontalHeader().logicalIndexAt(event.x(), event.y())
        print("AbstractTableView: Show Header Context Menu")

    def onMouseDoubleClick(self, index):
        """
        mouse event for double click.
        emitted when the user double clicks on a cell that is not editable.
        """
        print("AbstractTableView:  Double Click event")

    def onMouseReleaseRight(self, event):
        """
        activated when the user right clicks somewhere in the table
        """
        #index = self.indexAt(event.position())
        # print(index.row(),index.column())
        print("AbstractTableView:  Release Right event", event)

    def onMouseReleaseMiddle(self, event):
        """
        activated when the user middle clicks somewhere in the table
        """
        #index = self.indexAt(event.position())
        # print(index.row(),index.column())
        print("AbstractTableView: Release Middle event")

    def onMouseReleaseBack(self, event):
        print("AbstractTableView: Release Back event")

    def onMouseReleaseForward(self, event):
        print("AbstractTableView: Release Forward event")

    def onHeaderClicked(self, idx):
        """
        activated when the user clicks a header
        """
        print("AbstractTableView: Header Clicked %d" % idx)

    # -----------------------------------------------------------------------
    # drag and drop operations

    def onDragBegin(self):
        pass

    # -----------------------------------------------------------------------
    # helper functions

    def _setColumnOrder(self, order):


        h = self.horizontalHeader()

        if len(order) != h.count():
            return

        for i in range(h.count()):
            item = h.visualIndex(i)
            # if this item is not in the correct position
            # find the element that should be in this position
            # and swap them
            if item != order[i]:
                for j in range(i, h.count()):
                    if h.visualIndex(j) == order[i]:
                        h.swapSections(i, j)

    def _getColumnOrder(self):
        h = self.horizontalHeader()
        return [h.visualIndex(i) for i in range(h.count())]

    def _setHiddenColumns(self, hidden):
        h = self.horizontalHeader()
        hidden = set(hidden)
        for i in range(h.count()):
            h.setSectionHidden(i, i in hidden)

    def _getHiddenColumns(self):
        h = self.horizontalHeader()
        return [i for i in range(h.count()) if h.isSectionHidden(i)]

    def _setColumnWidths(self, widths):

        h = self.horizontalHeader()
        if len(widths) != h.count():
            return

        for i, w in enumerate(widths):
            if w != 0:
                self.setColumnWidth(i, w)

    def _getColumnWidths(self):
        h = self.horizontalHeader()
        return [self.columnWidth(i) for i in range(h.count())]

    def getColumnState(self):

        state = {
            "order": self._getColumnOrder(),
            "hidden": self._getHiddenColumns(),
            "widths": self._getColumnWidths(),
        }

        return state

    def setColumnState(self, state):

        if not state or not isinstance(state, dict):
            return

        self._setColumnOrder(state.get("order", []))
        self._setHiddenColumns(state.get("hidden", []))
        self._setColumnWidths(state.get("widths", []))

    def __getState(self):
        """ return a representation of the current state """
        headerState = self.horizontalHeader().saveState()
        state = headerState.data()
        state = zlib.compress(state)
        state =  base64.b64encode(state).decode("utf-8")
        return state

    def __setState(self, state):
        """ restore state given the representation """
        print("restoring", state)
        if not state:
            print("error restoring state")
            return
        try:
            state = base64.b64decode(state.encode("utf-8"))
            state = zlib.decompress(state)
        except Exception as e:
            print("error restoring state", e)
            return
        state = QByteArray(state)
        result = self.horizontalHeader().restoreState(state)
        return result

    def setModel(self, model):
        self._baseModel = model  # save a reference to the non-proxy model
        super(AbstractTableView, self).setModel(model)
        model.dataChanged.connect(self.onDataChanged)

    def baseModel(self):
        return self._baseModel

    def setNewData(self, data):
        self.baseModel().setNewData(data)

        # todo: revist this, may not always change
        self.selectionChangedEvent.emit()

    def replaceRow(self, row, rowdata):
        self.baseModel().replaceRow(row, rowdata)

    def insertRow(self, row, rowdata):
        self.baseModel().insertRow(row, rowdata)

    def appendRow(self, rowdata):
        self.baseModel().insertRow(self.rowCount(), rowdata)

    def appendRows(self, rows):
        self.baseModel().appendRows(rows)

    def data(self, row_index):
        """ get row data at index, used in conjunction with transforms """
        # note: this uncovers an interesting implementation bug with
        # the layered proxy models. background transforms paint
        # using the base model, which can be confusing...
        return self.baseModel().data(
            self.baseModel().index(row_index, 0), RowValueRole)

    def forceReset(self):
        self.baseModel().forceReset()

    def getData(self):
        return self.baseModel().getData()

    def rowCount(self):
        return self.baseModel().rowCount(None)

    def columnCount(self):
        return self.baseModel().columnCount(None)

    def setAlternatingRowColors(self, bEnable):
        super(AbstractTableView, self).setAlternatingRowColors(bEnable)

    def getSortProxyModel(self):
        """
        returns a new instance of SortProxyModel, used for
        sorting the baseModel()
        """
        model = SortProxyModel(self)
        model.setSourceModel(self.baseModel())
        return model

    def setSortingEnabled(self, bEnabled):
        """ set a proxy model to enable sorting
            Re-implement getSortProxyModel to have more control over
            how sorting is performed
        """
        if bEnabled:
            super(AbstractTableView, self).setModel(self.getSortProxyModel())
        else:
            super(AbstractTableView, self).setModel(self.baseModel())
        super().setSortingEnabled(bEnabled)

    def setHorizontalHeaderVisible(self, bVisible):
        self.horizontalHeader().setVisible(bVisible)

    def setVerticalHeaderVisible(self, bVisible):
        self.verticalHeader().setVisible(bVisible)

    # def setStretchLastSection(self,bStretch):
    #    self.horizontalHeader().setStretchLastSection(bStretch)

    def setDelegate(self, colidx, delegate):
        if self._baseModel is None:
            raise TableError("No Model Set")
        self.setItemDelegateForColumn(colidx, delegate)

    # --------------------------------------------------------------------------
    # editor

    #def commitData(self, editor):
    #    super().commitData(editor)

    #def onCommitValidateData(self, index, value):
    #    return True

    # --------------------------------------------------------------------------
    # row operations

    def setRowHeight(self, iSize):
        self.verticalHeader().setDefaultSectionSize(iSize)

    # --------------------------------------------------------------------------
    # column operations

    def setColumnsMovable(self, bMove):
        self.horizontalHeader().setSectionsMovable(bMove)

    def setColumnHidden(self, logicalIndex):
        self.horizontalHeader().setSectionHidden(logicalIndex)

    def getHiddenColumns(self):

        header = self.horizontalHeader()

        hidden = []
        for i in range(header.length()):
            if header.isSectionHidden(i):
                hidden.append(i)
        return hidden

    def setColumnHeaderClickable(self, bClickable):
        self.horizontalHeader().setSectionsClickable(bClickable)

    def setColumnSortIndicatorShown(self, bVisible):
        self.horizontalHeader().setSortIndicatorShown(bVisible)

    def setColumnSortIndicator(self, idx, order=Qt.SortOrder.AscendingOrder):
        """
        idx : tjhe column index
        order: the qt Sort order, one of:
            Qt.SortOrder.AscendingOrder
            Qt.SortOrder.DescendingOrder
        """
        self.horizontalHeader().setSortIndicator(idx, order)

    def setColumnName(self, idx, name):
        self.baseModel().getColumn(idx).setName(name)

    def setColumnHidden(self, idx, bHidden):
        self.horizontalHeader().setSectionHidden(idx, bHidden)

    def scrollToRow(self, row, column=0, hint=QAbstractItemView.ScrollHint.PositionAtCenter):
        index = self.model().index(row, column)
        super(AbstractTableView, self).scrollTo(index, hint)

    def resizeColumnToContents(self, index):
        super(AbstractTableView, self).resizeColumnToContents(index)

    def setColumnWidth(self, index, width):
        super(AbstractTableView, self).setColumnWidth(index, width)
        #self.horizontalHeader().resizeSection(index, width)
        #print(self.horizontalHeader().sectionResizeMode(index), width)

    def setLastColumnExpanding(self, bExpand):
        self.horizontalHeader().setStretchLastSection(bExpand)

    # --------------------------------------------------------------------------
    # selection model

    def hasSelection(self):
        model = self.selectionModel()
        return model.hasSelection()

    def getSelectedRows(self):
        """ return indices of selected rows
            valid only if select by rows is enabled
        """
        if self.selectionBehavior() != QAbstractItemView.SelectionBehavior.SelectRows:
            raise TableError("Selection Behavior does not support selecting rows.")
        row_indices = self.selectionModel().selectedRows()
        model = self.model()
        return [model.data(index, RowValueIndexRole) for index in row_indices]

    def getSelectionCount(self):
        return len(self.selectionModel().selectedRows())

    def getSelection(self):

        row_indices = self.selectionModel().selectedRows()

        model = self.model()
        # the column model has a custom role, RowValueRole
        # which is used to return the unmodified row data
        return [model.data(index, RowValueRole) for index in row_indices]

    def setSelectedRows(self, rows):
        """
        rows : list or iterable of integer indices.

        set the given set or rows to be selected.

        the current selection mode must support multi select, or
        non-contiguous selection depending on the contents of rows
        for the requested selection to take place.
        """
        if self.selectionBehavior() != QAbstractItemView.SelectionBehavior.SelectRows:
            raise TableError("Selection Behavior does not support selecting rows.")

        model = self.selectionModel()

        flags = QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select | QItemSelectionModel.SelectionFlag.Rows
        if rows:
            index = self.model().index(rows[0], 0)
            model.setCurrentIndex(index, flags)

        flags = QItemSelectionModel.SelectionFlag.Select | QItemSelectionModel.SelectionFlag.Rows
        for row in rows[1:]:
            model.select(self.model().index(row, 0), flags)

    def setSelectedRowsByIndex(self, indices):
        model = self.selectionModel()

        flags = QItemSelectionModel.SelectionFlag.Clear | QItemSelectionModel.SelectionFlag.Select | QItemSelectionModel.SelectionFlag.Rows
        if indices:
            model.setCurrentIndex(indices[0], flags)

        flags = QItemSelectionModel.SelectionFlag.Select | QItemSelectionModel.SelectionFlag.Rows
        for index in indices[1:]:
            model.select(index, flags)

    def getSelectedColumns(self):
        """ return indices of selected columns
            valid only if select by columns is enabled
        """
        if self.selectionBehavior() != QAbstractItemView.SelectionBehavior.SelectColumns:
            raise TableError("Selection Behavior does not support selecting columns.")
        model = self.selectionModel()
        return [i.column() for i in model.selectedColumns()]

    def onDataChanged(self, topLeft, bottomRight, roles):

        row_start = topLeft.row()
        row_end = bottomRight.row()
        col_start = topLeft.column()
        col_end = bottomRight.column()
        #print("onDataChanged", topLeft.row(), topLeft.column(),
        #   bottomRight.row(), bottomRight.column(), roles)

        for row in range(row_start, row_end + 1):
            self.rowChanged.emit(row)

class TableView(AbstractTableView):
    """

    A TableView displays 2-dimensional data.

    setNewData accepts a list-of-items. items must be indexable.
        e.g. list-of-lists, list-of-tuples, or list-of-dicts
    """

    def __init__(self, parent=None):
        super(TableView, self).__init__(parent)
        self.setModel(TableModel(self))

class ListView(AbstractTableView):
    """
    a ListView is a TableView with a single column

    this is a convenience class to handle a common case

    setNewData accepts a list of items to display
        e.g. a list-of-strings.
    This differs from a TableView, which would accept 2 dimensional data.
    """

    def __init__(self, parent):
        super(ListView, self).__init__(parent)
        self.setModel(TableModel(self))

        self.setLastColumnExpanding(True)
        self.baseModel().addTableColumn(TableListColumn(self.baseModel(), ""))

    def setHeaderText(self, text):
        self.setColumnName(0, text)

class _DemoMainWindow(QMainWindow):
    """docstring for MainWindow"""

    def __init__(self):
        super(_DemoMainWindow, self).__init__()

        img = QImage(48, 48, QImage.Format.Format_RGB32)
        img.fill(qRgb(0, 0, 0))

        # Build a random table of data to display
        data = []
        for i in range(50):
            data.append([random.randint(1, 5), ] + [''.join([random.choice(string.ascii_lowercase) for i in range(random.randint(3, 6))]) for i in range(3)])

        self.table = TableView(self)
        model = self.table.baseModel()
        model.addColumn(0, "File Name", editable=True)
        model.addColumn(1, "File Size", editable=False)
        model.addTransformColumn(None, "transform", lambda data, row, key: "test")
        model.addForegroundRule("1", lambda index, col: QColor(255, 0, 0) if index.row() == 1 else None)
        model.addBackgroundRule("2", lambda index, col: QColor(204, 204, 255) if index.row() == 2 else None)

        model.setColumnShortName(0, "sn")
        #d = DualTextDelegate(lambda index: str(index.column()), self)
        d = StarDelegate(self)
        self.table.setDelegate(0, d)
        #d = ComboBoxTextDelegate(self,["1","23","abc","six"])
        # self.table.setDelegate(1,d)
        # self.table.setRowHeight(20);
        # self.table.setRowHeight(20);

        self.table.setColumnsMovable(True)
        self.table.setNewData(data)

        self.list = ListView(self)
        self.list.setHeaderText("Woah")
        self.list.setNewData([0, 1, 2, 3, 4, 5])

        #self.model = ListModel([0,2,3,4],self)
        #self.table = ListView(self.model,self)
        self.pbGenData = QPushButton(self)
        self.pbGenData.clicked.connect(self.randomData)

        self.centralWidget = QWidget(self)
        self.vbox = QVBoxLayout(self.centralWidget)
        self.vbox.addWidget(self.table)
        self.vbox.addWidget(self.list)
        self.vbox.addWidget(self.pbGenData)
        self.setCentralWidget(self.centralWidget)

    def randomData(self):

        print(self.table.selectedIndexes())
        print(len(self.table.selectedIndexes()))
        model = self.table.selectionModel()
        g = lambda index: (index.row(), index.column())
        print(model.hasSelection())
        print([g(i) for i in model.selectedRows()])

        cols = 5
        rows = 30000
        data = [list(range(cols)) for i in range(rows)]
        self.table.baseModel().setNewData(data)

def main():

    app = QApplication(sys.argv)

    window = _DemoMainWindow()
    window.show()

    app.exec()


if __name__ == '__main__':
    main()

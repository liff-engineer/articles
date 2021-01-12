import sys
from PySide2.QtWidgets import QGraphicsView, QGraphicsScene, QApplication
from PySide2.QtCore import *
from PySide2.QtGui import *


class GraphicsView(QGraphicsView):

    def __init__(self, parent=None):
        super().__init__(parent)

        # 画布视图尺寸
        self.w = 64000.0
        self.h = 32000.0

        # 缩放相关
        self.zoomInFactor = 1.25
        self.zoomClamp = True
        self.zoom = 10
        self.zoomStep = 1
        self.zoomRange = [0, 20]

        self.setRenderHints(QPainter.Antialiasing | QPainter.HighQualityAntialiasing |
                            QPainter.TextAntialiasing | QPainter.SmoothPixmapTransform)
        self.setViewportUpdateMode(QGraphicsView.FullViewportUpdate)
        self.setVerticalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setHorizontalScrollBarPolicy(Qt.ScrollBarAlwaysOff)
        self.setTransformationAnchor(QGraphicsView.AnchorUnderMouse)
        self.setDragMode(QGraphicsView.RubberBandDrag)

        self.setScene(QGraphicsScene())
        self.setSceneRect(-self.w/2, -self.h/2, self.w, self.h)

    def zoomImpl(self, bigOrSmall: bool):
        zoomOutFactor = 1 / self.zoomInFactor
        zoomFactor = zoomOutFactor
        if bigOrSmall:
            zoomFactor = self.zoomInFactor
            self.zoom += self.zoomStep
        else:
            zoomFactor = zoomOutFactor
            self.zoom -= self.zoomStep
        clamped = False

        if self.zoom < self.zoomRange[0]:
            self.zoom, clamped = self.zoomRange[0], True
        if self.zoom > self.zoomRange[1]:
            self.zoom, clamped = self.zoomRange[1], True

        if not clamped or self.zoomClamp is False:
            self.scale(zoomFactor, zoomFactor)

    def panBeginImpl(self, event):
        releaseEvent = QMouseEvent(QEvent.MouseButtonRelease, event.localPos(), event.screenPos(),
                                   Qt.LeftButton, Qt.NoButton, event.modifiers())
        super().mouseReleaseEvent(releaseEvent)
        self.setDragMode(QGraphicsView.ScrollHandDrag)
        fakeEvent = QMouseEvent(event.type(), event.localPos(), event.screenPos(),
                                Qt.LeftButton, event.buttons() | Qt.LeftButton, event.modifiers())
        super().mousePressEvent(fakeEvent)

    def panEndImpl(self, event):
        fakeEvent = QMouseEvent(event.type(), event.localPos(), event.screenPos(),
                                Qt.LeftButton, event.buttons() & ~Qt.LeftButton, event.modifiers())
        super().mouseReleaseEvent(fakeEvent)
        self.setDragMode(QGraphicsView.RubberBandDrag)

    def keyPressEvent(self, event):
        if event.matches(QKeySequence.ZoomIn):
            self.zoomImpl(True)
        elif event.matches(QKeySequence.ZoomOut):
            self.zoomImpl(False)
        else:
            super().keyPressEvent(event)

    def wheelEvent(self, event):
        if self.dragMode() == QGraphicsView.ScrollHandDrag:
            return
        return self.zoomImpl(event.angleDelta().y() > 0)

    def mousePressEvent(self, event):
        if event.button() == Qt.MiddleButton:
            return self.panBeginImpl(event)
        super().mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        if event.button() == Qt.MiddleButton:
            return self.panEndImpl(event)
        super().mouseReleaseEvent(event)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    appView = GraphicsView()
    appView.scene().addSimpleText('liff.engineer@gmail.com')
    appView.scene().addRect(-200, -150, 400, 300)
    appView.show()
    sys.exit(app.exec_())

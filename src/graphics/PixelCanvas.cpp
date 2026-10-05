#include "PixelCanvas.h"
#include <QPainter>
#include <algorithm>

namespace Graphics {

PixelCanvas::PixelCanvas(Framebuffer& framebuffer, Core::InputController& inputController, QWidget* parent)
    : QWidget(parent)
    , m_framebuffer(framebuffer)
    , m_input(inputController)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
}

void PixelCanvas::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false); // Keep pixel-art crisp!

    int widgetW = width();
    int widgetH = height();
    int virtW = m_framebuffer.width();
    int virtH = m_framebuffer.height();

    // Aspect-ratio letterbox calculation
    float scaleX = static_cast<float>(widgetW) / virtW;
    float scaleY = static_cast<float>(widgetH) / virtH;
    float scale = std::min(scaleX, scaleY);

    int destW = static_cast<int>(virtW * scale);
    int destH = static_cast<int>(virtH * scale);
    int destX = (widgetW - destW) / 2;
    int destY = (widgetH - destH) / 2;

    // Fill black letterbox bars if window aspect ratio doesn't match 16:9
    if (destX > 0 || destY > 0) {
        painter.fillRect(rect(), Qt::black);
    }

    // Blit internal software framebuffer in a single hardware call
    painter.drawImage(QRect(destX, destY, destW, destH), m_framebuffer.getImage());
}

void PixelCanvas::keyPressEvent(QKeyEvent* event) {
    m_input.onKeyPress(event);
}

void PixelCanvas::keyReleaseEvent(QKeyEvent* event) {
    m_input.onKeyRelease(event);
}

void PixelCanvas::mouseMoveEvent(QMouseEvent* event) {
    m_input.onMouseMove(event, m_framebuffer.width(), m_framebuffer.height(), width(), height());
}

void PixelCanvas::mousePressEvent(QMouseEvent* event) {
    m_input.onMousePress(event, m_framebuffer.width(), m_framebuffer.height(), width(), height());
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent* event) {
    m_input.onMouseRelease(event, m_framebuffer.width(), m_framebuffer.height(), width(), height());
}

} // namespace Graphics

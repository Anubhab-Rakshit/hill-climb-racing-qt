#pragma once

#include <QWidget>
#include "Framebuffer.h"
#include "InputController.h"

namespace Graphics {

/**
 * @brief Qt Widget that displays the pure-raster Framebuffer with integer letterboxing.
 * Relays input events to the InputController.
 */
class PixelCanvas : public QWidget {
    Q_OBJECT
public:
    explicit PixelCanvas(Framebuffer& framebuffer, Core::InputController& inputController, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    Framebuffer& m_framebuffer;
    Core::InputController& m_input;
};

} // namespace Graphics

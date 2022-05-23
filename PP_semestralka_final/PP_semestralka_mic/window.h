#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;

class RenderArea;

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

private:
    RenderArea *renderArea;
};
//! [0]

#endif // WINDOW_H

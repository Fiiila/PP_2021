#include "renderarea.h"
#include "window.h"

#include <QtWidgets>


Window::Window()
{
	renderArea = new RenderArea;

	QGridLayout *mainLayout = new QGridLayout(this);
	mainLayout->addWidget(renderArea);

	setWindowTitle(tr("Mic na civce UI"));
}
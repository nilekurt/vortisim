#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	QSurfaceFormat format;
	format.setSamples(4);
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(3, 2);
	format.setProfile(QSurfaceFormat::CoreProfile);
	QSurfaceFormat::setDefaultFormat(format);

	MainWindow win;
	win.show();

	return app.exec();
}

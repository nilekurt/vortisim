#include "mainwindow.h"

#include <QApplication>
#include <QSurfaceFormat>

int
main(int argc, char ** argv)
{
    QApplication app(argc, argv);

    QSurfaceFormat format{};

    constexpr int n_samples = 4;
    format.setSamples(n_samples);

    constexpr int depth_bits = 24;
    format.setDepthBufferSize(depth_bits);

    constexpr int stencil_bits = 8;
    format.setStencilBufferSize(stencil_bits);

    constexpr int gl_major = 3;
    constexpr int gl_minor = 2;
    format.setVersion(gl_major, gl_minor);

    format.setProfile(QSurfaceFormat::CoreProfile);

    QSurfaceFormat::setDefaultFormat(format);

    MainWindow win;
    win.show();

    return QApplication::exec();
}

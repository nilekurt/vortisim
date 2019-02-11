#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMouseEvent>

#include <glm/glm.hpp>

#include <vector>

class DisplayWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	DisplayWidget(QWidget* parent);

protected:
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	void mousePressEvent(QMouseEvent* event);

	QOpenGLContext* _context;

private:
	glm::mat4 _proj;
	glm::mat4 _modelview;
	glm::vec4 _viewport;

	GLint _mvp_location;

	QOpenGLBuffer _vbo;
	QOpenGLVertexArrayObject _vao;
	QOpenGLShaderProgram _prog;
	std::vector<glm::vec2> _points;
	int _width;
	int _height;
};

#endif // DISPLAYWIDGET_H

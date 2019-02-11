#include "displaywidget.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

DisplayWidget::DisplayWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	_vbo(QOpenGLBuffer::VertexBuffer),
	_modelview(1.0f)
{
	_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
}

void DisplayWidget::initializeGL()
{
	initializeOpenGLFunctions();

	// Set up OpenGL state
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH);
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Set up shader program
	_prog.create();
	_prog.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/default.v.glsl");
	_prog.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/default.f.glsl");
	_prog.link();
	_mvp_location = _prog.uniformLocation("mvp");

	// Set up VAO
	_vao.create();

	// Bind VAO
	/*-------------------------*/
	_prog.bind();
	_vao.bind();
	/*-------------------------*/

	// Set up VBO
	_vbo.create();
	_vbo.setUsagePattern(QOpenGLBuffer::StreamDraw);
	_vbo.bind();
	_vbo.allocate(256*sizeof(glm::vec2));
	_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 2);
	_prog.enableAttributeArray("in_position");
	_vbo.release();

	// Unbind VAO
	/*-------------------------*/
	_vao.release();
	_prog.release();
	/*-------------------------*/
}

void DisplayWidget::resizeGL(int w, int h)
{
	_width = w;
	_height = h;
	_viewport = {0, 0, w, h};
	glViewport(0, 0, w, h);
}

void DisplayWidget::paintGL()
{
	std::cout << "DisplayWidget::paintGL()" << std::endl;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_prog.bind();
	auto mvp = _proj * _modelview;
	glUniformMatrix4fv(_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

	// DRAW!
	_vao.bind();
	glDrawArrays(GL_POINTS, 0, _points.size());
	glDrawArrays(GL_LINE_STRIP, 0, _points.size());
	_vao.release();

	_prog.release();
}

void DisplayWidget::mousePressEvent(QMouseEvent* event)
{
	switch(event->button())
	{
		case Qt::LeftButton:
		{

			// TODO: Lines from pairs of points and drawing with GL_LINE instead
			// Looping for closed polygon detection (relational point<->line?)
			// Line intersection checking
			// Automatic continuation with dynamically rendered final line
			// + right click cancellation
			// CGAL for tesselation?
			glm::vec3 v(event->x(), _height - event->y(), 0.0f);
			glm::vec3 res = glm::unProject(v, _modelview, _proj, _viewport);

			auto size = _points.size();
			_points.emplace_back(res.x, res.y);

			_vbo.bind();
			_vbo.write(size * sizeof(glm::vec2), &_points.back(), sizeof(glm::vec2));
			_vbo.release();

			std::cout << "------------" << std::endl;
			for (auto& x : _points)
			{
				std::cout << x.x << " " << x.y << std::endl;

			}
			std::cout << "----- " << _points.size() << " -----" << std::endl;
		}
		break;

		case Qt::RightButton:
		{

		}
		break;

		default:
		break;
	}

	update();
}



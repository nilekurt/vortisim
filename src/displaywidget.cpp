#include "displaywidget.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <queue>

DisplayWidget::DisplayWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	_point_vbo(QOpenGLBuffer::VertexBuffer),
	_modelview(1.0f),
	_threshold(20),
	_mouse_threshold(2)
{
	_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	setMouseTracking(true);
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

	// Set up VAOs
	_point_vao.create();
	_line_vao.create();
	_guide_vao.create();

	_prog.bind();

	// Set up VBOs
	_point_vao.bind();
		_point_vbo.create();
		_point_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		_point_vbo.bind();
			_point_vbo.allocate(256*sizeof(v3));
			_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
			_prog.enableAttributeArray("in_position");
		_point_vbo.release();
	_point_vao.release();

	_line_vao.bind();
		_line_vbo.create();
		_line_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		_line_vbo.bind();
			_line_vbo.allocate(512*sizeof(v3));
			_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
			_prog.enableAttributeArray("in_position");
		_line_vbo.release();
	_line_vao.release();

	_guide_vao.bind();
		_guide_vbo.create();
		_guide_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
		_guide_vbo.bind();
			_guide_vbo.allocate(2*sizeof(v3));
			_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
			_prog.enableAttributeArray("in_position");
		_guide_vbo.release();
	_guide_vao.release();

	_prog.release();
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	_prog.bind();
	auto mvp = _proj * _modelview;
	glUniformMatrix4fv(_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

	_point_vao.bind();
		glDrawArrays(GL_POINTS, 0, _points.size());
	_point_vao.release();

	_line_vao.bind();
		glDrawArrays(GL_LINES, 0, 2 * _lines.size());
	_line_vao.release();

	if (_prev_point)
	{
		auto res = glm::unProject(_mousePos, _modelview, _proj, _viewport);
		_guide_vbo.bind();
			_guide_vbo.write(0, _prev_point.get(), sizeof(v3));
			_guide_vbo.write(sizeof(v3), &res, sizeof(v3));
		_guide_vbo.release();

		_guide_vao.bind();
			glDrawArrays(GL_LINES, 0, 2);
		_guide_vao.release();
	}

	_prog.release();
}

void DisplayWidget::mousePressEvent(QMouseEvent* event)
{
	_mousePos = {event->x(), _height - event->y(), 0.0f};

	switch(event->button())
	{
		case Qt::LeftButton:
		{
			// @TODO: Lines from pairs of points and drawing with GL_LINE instead
			// Looping for closed polygon detection (relational point<->line?)
			// Line intersection checking
			// CGAL for tesselation?
			// Repeat timer for continuous drawing?

			addPoint(_mousePos);
		}
		break;

		case Qt::RightButton:
		{
			_prev_point.reset();
		}
		break;

		default:
		break;
	}

	update();
}

void DisplayWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (_prev_point)
	{
		glm::vec3 res(event->x(), _height - event->y(), 0.0f);

		if (glm::length(_mousePos - res) > _mouse_threshold)
		{
			_mousePos = res;
			update();
		}
	}
}

void DisplayWidget::addPoint(const glm::vec3& point)
{
	// @TODO: Quadtree

	auto end = _points.end();
	auto res = glm::unProject(point, _modelview, _proj, _viewport);

	auto compare = [&](const v3_ptr& a, const v3_ptr& b)
	{
		return glm::length(res - *a) > glm::length(res - *b);
	};

	std::priority_queue<v3_ptr, std::vector<v3_ptr>, decltype(compare)> hits(_points.begin(), end, compare);
	if (hits.empty())
	{
		_prev_point = std::make_shared<v3>(res);
		_points.push_back(_prev_point);
		updatePoints();

		return;
	}

	auto max = hits.top();
	res = glm::project(*max, _modelview, _proj, _viewport);
	if (glm::length(point - res) < _threshold)
	{
		if (_prev_point)
		{
			if (max != _prev_point)
			{
				addLine(_prev_point, max);
				_prev_point.reset();

				addPolygon(max);
			}
		}
		else
		{
			_prev_point = max;
		}
	}
	else
	{
		auto res = glm::unProject(point, _modelview, _proj, _viewport);
		auto new_point = std::make_shared<v3>(res);

		if (_prev_point)
		{
			addLine(_prev_point, new_point);
		}

		_points.push_back(new_point);
		_prev_point = new_point;
		updatePoints();
	}
}

void DisplayWidget::addLine(v3_ptr& a, v3_ptr& b)
{
	// @TODO: Add to unordered maps for indexing

	auto size = _lines.size();
	auto line = std::make_shared<line3>(a, b);
	_lines.push_back(line);

	_line_vbo.bind();
		_line_vbo.write(2*size * sizeof(v3), a.get(), sizeof(v3));
		_line_vbo.write((2*size + 1) * sizeof(v3), b.get(), sizeof(v3));
	_line_vbo.release();
}

void DisplayWidget::addPolygon(v3_ptr &p)
{
	// @TODO: Walk polygon to find closed loop
}

void DisplayWidget::updatePoints()
{
	_point_vbo.bind();
		_point_vbo.write((_points.size() - 1) * sizeof(v3), _points.back().get(), sizeof(v3));
	_point_vbo.release();
}

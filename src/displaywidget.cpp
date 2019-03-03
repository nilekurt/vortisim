#include "displaywidget.h"

#include <QOpenGLTexture>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Eigen/Dense>

#include <iostream>
#include <memory>
#include <queue>

DisplayWidget::DisplayWidget(QWidget* parent) :
	QOpenGLWidget(parent),
	_point_vbo(QOpenGLBuffer::VertexBuffer),
	_modelview(1.0f),
	_threshold(10),
	_mouse_threshold(2),
	_vinf(1.0f, 0.0f),
	_has_polygon(false)
{
	_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	setMouseTracking(true);
}

void DisplayWidget::initializeGL()
{
	initializeOpenGLFunctions();

	// Set up OpenGL statefloat
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH);
	glEnable(GL_PROGRAM_POINT_SIZE);

	// Set up shaders
	_edit_prog.create();
	_edit_prog.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/default.v.glsl");
	_edit_prog.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/default.f.glsl");
	_edit_prog.link();
	_edit_mvp_location = _edit_prog.uniformLocation("mvp");

	_field_prog.create();
	_field_prog.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/arrows.v.glsl");
	_field_prog.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/arrows.f.glsl");
	_field_prog.link();
	_field_mvp_location = _field_prog.uniformLocation("mvp");
	_field_inverse_mvp_location = _field_prog.uniformLocation("inverse_mvp");
	_viewport_location = _field_prog.uniformLocation("viewport");
	_lines_location = _field_prog.uniformLocation("lines");
	_strengths_location = _field_prog.uniformLocation("strengths");
	_n_lines_location = _field_prog.uniformLocation("n_lines");
	_vinf_location = _field_prog.uniformLocation("vinf");

	// Set up VAOs
	_point_vao.create();
	_line_vao.create();
	_guide_vao.create();
	_field_vao.create();

	_field_prog.bind();
	{
		constexpr GLfloat square[] = {-1.0, 1.0,
								   1.0, 1.0,
								   1.0, -1.0,
								   -1.0, -1.0};

		_field_vao.bind();
		{
			_field_vbo.create();
			_field_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
			_field_vbo.bind();
			{
				_field_vbo.allocate(square, sizeof(square));
				_field_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 2);
				_field_prog.enableAttributeArray("in_position");
			}
			_field_vbo.release();
		}
		_field_vao.release();

		glUniform2fv(_vinf_location, 1, (GLfloat*)&_vinf);
	}
	_field_prog.release();


	_edit_prog.bind();
	{
		_point_vao.bind();
		{
			_point_vbo.create();
			_point_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
			_point_vbo.bind();
			{
				_point_vbo.allocate(256*sizeof(v3));
				_edit_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
				_edit_prog.enableAttributeArray("in_position");
			}
			_point_vbo.release();
		}
		_point_vao.release();

		_line_vao.bind();
		{
			_line_vbo.create();
			{
			_line_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
			_line_vbo.bind();
				_line_vbo.allocate(512*sizeof(v3));
				_edit_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
				_edit_prog.enableAttributeArray("in_position");
			}
			_line_vbo.release();
		}
		_line_vao.release();

		_guide_vao.bind();
		{
			_guide_vbo.create();
			_guide_vbo.setUsagePattern(QOpenGLBuffer::DynamicDraw);
			_guide_vbo.bind();
			{
				_guide_vbo.allocate(2*sizeof(v3));
				_edit_prog.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
				_edit_prog.enableAttributeArray("in_position");
			}
			_guide_vbo.release();
		}
		_guide_vao.release();
	}
	_edit_prog.release();
}

void DisplayWidget::resizeGL(int w, int h)
{
	_width = w;
	_height = h;
	_viewport = {0, 0, w, h};
	glViewport(0, 0, w, h);

	_field_prog.bind();
	{
		glUniform4f(_viewport_location, 0, 0, w, h);
	}
	_field_prog.release();
}

void DisplayWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	auto mvp = _proj * _modelview;
	auto inverse_mvp = glm::inverse(mvp);

	if (_has_polygon)
	{
		_field_prog.bind();
		{
			glUniformMatrix4fv(_field_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
			glUniformMatrix4fv(_field_inverse_mvp_location, 1, GL_FALSE, glm::value_ptr(inverse_mvp));

			_field_vao.bind();
			{
				glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
			}
			_field_vao.release();
		}
		_field_prog.release();
	}

	_edit_prog.bind();
	{
		glUniformMatrix4fv(_edit_mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));

		_point_vao.bind();
		{
			glDrawArrays(GL_POINTS, 0, _points.size());
		}
		_point_vao.release();

		_line_vao.bind();
		{
			glDrawArrays(GL_LINES, 0, 2 * _lines.size());
		}
		_line_vao.release();

		if (_prev_point)
		{
			auto res = glm::unProject(_mousePos, _modelview, _proj, _viewport);
			_guide_vbo.bind();
			{
				_guide_vbo.write(0, _prev_point.get(), sizeof(v3));
				_guide_vbo.write(sizeof(v3), &res, sizeof(v3));
			}
			_guide_vbo.release();

			_guide_vao.bind();
			{
				glDrawArrays(GL_LINES, 0, 2);
			}
			_guide_vao.release();
		}
	}
	_edit_prog.release();
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

#define INTEGRATOR_STEPS (30)
#define TAU (2.0*3.14159265358979)

double DisplayWidget::integrate(const v3& p1, const v3& diff, const v2& n, const v3& p)
{
	auto delta = diff / (float)INTEGRATOR_STEPS;

	double ans = 0.0;
	for (unsigned int i = 0; i < INTEGRATOR_STEPS; ++i)
	{
		auto current_pos = p1 + (float)i * delta;

		auto r = p - current_pos;
		auto denom = glm::dot(r,r);

		glm::vec2 grad(current_pos.y - p.y, p.x - current_pos.x);
		grad /= denom;

		ans += glm::dot(grad, n);
	}
	return ans / (double)INTEGRATOR_STEPS;
}

void DisplayWidget::addPolygon(v3_ptr &p)
{
	// @TODO: Keep finished polygons separate in order to facilitate multiple Kutta conditions
	// @TODO: Walk polygon to find closed loop
	(void)p;
	// Assume closed polygon with lines in order
	std::vector<v3> centers;
	for (const auto& l : _lines)
	{
		auto p1 = l->first;
		auto p2 = l->second;
		auto c = (*p1 + *p2) / 2.0f;
		centers.push_back(c);
	}

	// Anderson p. 387

	const glm::mat3 ccw_rotation(v3(0.0f, -1.0f, 0.0f),
								 v3(1.0f, 0.0f, 0.0f),
								 v3(0.0f, 0.0f, 1.0f));

	auto n_lines = _lines.size();
	Eigen::MatrixXd A(n_lines, n_lines);
	Eigen::MatrixXd b(n_lines, 1);
	// For all line midpoints
	for (unsigned int i = 0; i < n_lines - 1; ++i)
	{
		auto& l = _lines[i];
		auto p1 = l->first;
		auto p2 = l->second;
		auto diff = *p2 - *p1;
		auto n = ccw_rotation * glm::normalize(diff);
		b(i) = glm::dot(n, v3(_vinf, 0.0f));

		for (unsigned int j = 0; j < n_lines; ++j)
		{
			auto& c = centers[j];

			if (i == j)
			{
				A(i,j) = 0.0f;
			}
			else
			{
				// Integrate over every line
				A(i,j) = integrate(*p1, diff, n, c);
			}
		}
	}
	A /= TAU;

	// Replace last panel strength with Kutta condition
	A(n_lines - 1, 0) = 1.0;
	A(n_lines - 1, n_lines - 1) = 1.0;
	b(n_lines - 1, 0) = 0;

	Eigen::MatrixXd x = A.fullPivHouseholderQr().solve(b);

	auto vorticity = x.col(0);

	std::cout << vorticity << std::endl;

	std::vector<GLfloat> lineData;
	lineData.reserve(2 * n_lines);
	for (auto& l_ptr : _lines)
	{
		auto p1 = l_ptr->first;
		auto p2 = l_ptr->second;

		lineData.push_back(p1->x);
		lineData.push_back(p1->y);
		lineData.push_back(p2->x);
		lineData.push_back(p2->y);
	}

	std::vector<GLfloat> strengthData;
	strengthData.reserve(n_lines);
	for (int i = 0; i < n_lines; ++i)
	{
		strengthData.push_back(vorticity(i));
	}

	_field_prog.bind();
	{
		glUniform4fv(_lines_location, n_lines, lineData.data());
		glUniform1fv(_strengths_location, n_lines, strengthData.data());
		glUniform1i(_n_lines_location, n_lines);
	}
	_field_prog.release();

	_has_polygon = true;
}

void DisplayWidget::updatePoints()
{
	_point_vbo.bind();
		_point_vbo.write((_points.size() - 1) * sizeof(v3), _points.back().get(), sizeof(v3));
	_point_vbo.release();
}

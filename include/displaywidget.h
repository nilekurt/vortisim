#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QMouseEvent>

#include <glm/glm.hpp>

#include <memory>
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
	void mouseMoveEvent(QMouseEvent* event);

	QOpenGLContext* _context;

private:
	typedef glm::vec3 v3;
	typedef std::shared_ptr<v3> v3_ptr;
	typedef std::pair<v3_ptr, v3_ptr> line3;
	typedef std::shared_ptr<line3> line3_ptr;

	void addPoint(const v3& point);
	void addLine(v3_ptr& a, v3_ptr& b);
	void addPolygon(v3_ptr& p);

	void updatePoints();

	QOpenGLShaderProgram _prog;
	QOpenGLBuffer _point_vbo;
	QOpenGLBuffer _line_vbo;
	QOpenGLBuffer _guide_vbo;
	QOpenGLVertexArrayObject _point_vao;
	QOpenGLVertexArrayObject _line_vao;
	QOpenGLVertexArrayObject _guide_vao;

	std::vector<v3_ptr> _points;
	std::vector<line3_ptr> _lines;
	std::unordered_map<v3_ptr, line3_ptr> _p2l;
	std::unordered_map<line3_ptr, v3_ptr> _l2p;

	glm::mat4 _proj;
	glm::mat4 _modelview;
	glm::vec4 _viewport;
	glm::vec3 _mousePos;

	v3_ptr _prev_point;

	float _threshold;
	float _mouse_threshold;

	GLint _mvp_location;

	int _width;
	int _height;
};

#endif // DISPLAYWIDGET_H

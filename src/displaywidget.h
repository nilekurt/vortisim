#ifndef DISPLAYWIDGET_H
#define DISPLAYWIDGET_H

#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <glm/glm.hpp>
#include <memory>
#include <variant>
#include <vector>

class DisplayWidget final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    using Mat3 = glm::mat3;
    using Mat4 = glm::mat4;

    using Vec2 = glm::vec2;
    using Vec3 = glm::vec3;
    using Vec4 = glm::vec4;
    using Vec3CRef = std::reference_wrapper<Vec3 const>;

    using Line3 = std::pair<Vec3CRef, Vec3CRef>;
    using Line3Ref = std::reference_wrapper<Line3 const>;

    explicit DisplayWidget(QWidget * parent);

protected:
    void
    initializeGL() final;

    void
    resizeGL(int w, int h) final;

    void
    paintGL() final;

    void
    mousePressEvent(QMouseEvent * event) final;

    void
    mouseMoveEvent(QMouseEvent * event) final;

    QOpenGLContext * context_{};

private:
    void
    initFlowField();

    void
    initEditor();

    void
    addPoint();

    void
    addLine(Vec3 const & a, Vec3 const & b);

    void
    addPolygon(Vec3 const & p);

    Vec3 const *
    getNearbyPoint(Vec3 const & v) const;

    void
    updatePoints();

    static double
    integrate(Vec3 const & p1,
              Vec3 const & diff,
              Vec2 const & n,
              Vec3 const & p);

    QOpenGLShaderProgram edit_prog_;
    QOpenGLShaderProgram field_prog_;

    QOpenGLBuffer point_vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer line_vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer guide_vbo_{QOpenGLBuffer::VertexBuffer};
    QOpenGLBuffer field_vbo_{QOpenGLBuffer::VertexBuffer};

    QOpenGLVertexArrayObject point_vao_{};
    QOpenGLVertexArrayObject line_vao_{};
    QOpenGLVertexArrayObject guide_vao_{};
    QOpenGLVertexArrayObject field_vao_{};

    std::variant<std::monostate, Vec3CRef, Vec3> prev_point_{};
    std::vector<Vec3>                            points_{};
    std::vector<Line3>                           lines_{};

    std::unordered_map<Vec3 *, Line3 *> p2l_{};
    std::unordered_map<Line3 *, Vec3 *> l2p_{};

    Mat4 proj_;
    Mat4 modelview_{1.0F};
    Vec4 viewport_{};

    Vec3 mouse_pos_{};

    Vec2 const vinf_{1.0F, 0.0F};

    float const threshold_{10.0F};
    float const mouse_threshold_{2.0F};

    GLint vinf_location_{};
    GLint edit_mvp_location_{};
    GLint field_inverse_mvp_location_{};
    GLint field_mvp_location_{};
    GLint lines_location_{};
    GLint strengths_location_{};
    GLint n_lines_location_{};
    GLint viewport_location_{};

    int width_{};
    int height_{};

    bool has_polygon_{false};
};

#endif // DISPLAYWIDGET_H

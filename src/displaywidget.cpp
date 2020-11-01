#include "displaywidget.h"

#include "overloaded.hh"

#include <Eigen/Dense>
#include <QOpenGLTexture>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <queue>

namespace {

template<typename T>
class BindOperation {
    T & target_;

public:
    explicit BindOperation(T & target) : target_{target}
    {
        target_.bind();
    }

    BindOperation() = delete;
    BindOperation(BindOperation const &) = delete;
    BindOperation(BindOperation &&) noexcept = delete;

    ~BindOperation()
    {
        target_.release();
    }

    BindOperation &
    operator=(BindOperation const &) = delete;
    BindOperation &
    operator=(BindOperation &&) noexcept = delete;
};

} // namespace

DisplayWidget::DisplayWidget(QWidget * parent)
    : QOpenGLWidget(parent),
      proj_{glm::ortho(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F)}
{
    setMouseTracking(true);

    constexpr int max_points = 512;
    constexpr int max_lines = max_points / 2;
    points_.reserve(max_points);
    lines_.reserve(max_lines);
}

void
DisplayWidget::initFlowField()
{
    // Shader program setup
    field_prog_.create();
    field_prog_.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                        ":/shaders/arrows.v.glsl");
    field_prog_.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                        ":/shaders/arrows.f.glsl");
    field_prog_.link();
    field_mvp_location_ = field_prog_.uniformLocation("mvp");
    field_inverse_mvp_location_ = field_prog_.uniformLocation("inverse_mvp");
    viewport_location_ = field_prog_.uniformLocation("viewport");
    lines_location_ = field_prog_.uniformLocation("lines");
    strengths_location_ = field_prog_.uniformLocation("strengths");
    n_lines_location_ = field_prog_.uniformLocation("n_lines");
    vinf_location_ = field_prog_.uniformLocation("vinf");

    // VAO and VBO setup
    field_vao_.create();
    field_vbo_.create();

    // Flow field program
    {
        BindOperation prog{field_prog_};

        // Canvas square
        {
            BindOperation vao{field_vao_};

            {
                BindOperation vbo{field_vbo_};

                constexpr std::array<GLfloat, 8> const square{
                    -1.0F, 1.0F, 1.0F, 1.0F, 1.0F, -1.0F, -1.0F, -1.0F};

                field_vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
                field_vbo_.allocate(square.data(),
                                    square.size() * sizeof(GLfloat));
                field_prog_.setAttributeBuffer("in_position", GL_FLOAT, 0, 2);
                field_prog_.enableAttributeArray("in_position");
            }
        }

        std::array<GLfloat, 2> tmp{vinf_.x, vinf_.y};
        glUniform2fv(vinf_location_, 1, tmp.data());
    }
}

void
DisplayWidget::initEditor()
{
    constexpr int max_points{256};
    constexpr int max_line_points{2 * max_points};

    // Shader program setup
    edit_prog_.create();
    edit_prog_.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                       ":/shaders/default.v.glsl");
    edit_prog_.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                       ":/shaders/default.f.glsl");
    edit_prog_.link();
    edit_mvp_location_ = edit_prog_.uniformLocation("mvp");

    // VAO and VBO setup
    point_vao_.create();
    line_vao_.create();
    guide_vao_.create();

    point_vbo_.create();
    line_vbo_.create();
    guide_vbo_.create();

    // Editor program
    {
        BindOperation prog{edit_prog_};

        // Points
        {
            BindOperation vao{point_vao_};

            {
                BindOperation vbo{point_vbo_};

                point_vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                point_vbo_.allocate(max_points * sizeof(Vec3));
                edit_prog_.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
                edit_prog_.enableAttributeArray("in_position");
                glVertexAttribPointer(
                    edit_prog_.attributeLocation("in_position"),
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    0,
                    nullptr);
            }
        }

        // Lines
        {
            BindOperation vao{line_vao_};

            {
                BindOperation vbo{line_vbo_};

                line_vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                line_vbo_.allocate(max_line_points * sizeof(Vec3));
                edit_prog_.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
                edit_prog_.enableAttributeArray("in_position");
                glVertexAttribPointer(
                    edit_prog_.attributeLocation("in_position"),
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    0,
                    nullptr);
            }
        }

        // Line guide
        {
            BindOperation vao{guide_vao_};

            {
                BindOperation vbo{guide_vbo_};

                guide_vbo_.setUsagePattern(QOpenGLBuffer::DynamicDraw);
                guide_vbo_.allocate(2 * sizeof(Vec3));
                edit_prog_.setAttributeBuffer("in_position", GL_FLOAT, 0, 3);
                edit_prog_.enableAttributeArray("in_position");
                glVertexAttribPointer(
                    edit_prog_.attributeLocation("in_position"),
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    0,
                    nullptr);
            }
        }
    }
}

void
DisplayWidget::initializeGL()
{
    initializeOpenGLFunctions();

    // Set up OpenGL state
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH);
    glEnable(GL_PROGRAM_POINT_SIZE);

    initEditor();
    initFlowField();
}

void
DisplayWidget::resizeGL(int w, int h)
{
    width_ = w;
    height_ = h;
    viewport_ = {0, 0, w, h};
    glViewport(0, 0, w, h);

    {
        BindOperation prog{field_prog_};

        glUniform4f(viewport_location_, 0, 0, w, h);
    }
}

void
DisplayWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 const mvp = proj_ * modelview_;
    Mat4 const inverse_mvp = glm::inverse(mvp);

    if (has_polygon_) {
        BindOperation prog{field_prog_};

        glUniformMatrix4fv(
            field_mvp_location_, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(field_inverse_mvp_location_,
                           1,
                           GL_FALSE,
                           glm::value_ptr(inverse_mvp));

        {
            BindOperation vao{field_vao_};

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }

    // Editor program
    {
        BindOperation prog{edit_prog_};

        glUniformMatrix4fv(
            edit_mvp_location_, 1, GL_FALSE, glm::value_ptr(mvp));

        // Points
        {
            BindOperation vao{point_vao_};

            glDrawArrays(GL_POINTS,
                         0,
                         points_.size() +
                             static_cast<int>(
                                 std::holds_alternative<Vec3>(prev_point_)));
        }

        // Lines
        {
            BindOperation vao{line_vao_};

            glDrawArrays(GL_LINES, 0, 2 * lines_.size());
        }

        std::visit(Overloaded{[](std::monostate const & /*unused*/) {},
                              [this](Vec3 const & v) {
                                  Vec3 const res = glm::unProject(
                                      mouse_pos_, modelview_, proj_, viewport_);

                                  {
                                      BindOperation vbo{guide_vbo_};

                                      guide_vbo_.write(0, &v, sizeof(Vec3));
                                      guide_vbo_.write(
                                          sizeof(Vec3), &res, sizeof(Vec3));
                                  }

                                  {
                                      BindOperation vao{guide_vao_};

                                      glDrawArrays(GL_LINES, 0, 2);
                                  }
                              }},
                   prev_point_);
    }
}

void
DisplayWidget::mousePressEvent(QMouseEvent * event)
{
    mouse_pos_ = {event->x(), height_ - event->y(), 0.0F};

    // NOLINTNEXTLINE
    switch (event->button()) {
        case Qt::LeftButton: {
            // @TODO: Lines from pairs of points and drawing with GL_LINE
            // instead Looping for closed polygon detection (relational
            // point<->line?) Line intersection checking CGAL for tesselation?
            // Repeat timer for continuous drawing?

            addPoint();
        } break;

        case Qt::RightButton: {
            prev_point_.emplace<std::monostate>();
        } break;

        default: break;
    }

    update();
}

void
DisplayWidget::mouseMoveEvent(QMouseEvent * event)
{
    if (!std::holds_alternative<std::monostate>(prev_point_)) {
        Vec3 res{event->x(), height_ - event->y(), 0.0F};

        if (glm::length(mouse_pos_ - res) > mouse_threshold_) {
            mouse_pos_ = std::move(res);
            update();
        }
    }
}

DisplayWidget::Vec3 const *
DisplayWidget::getNearbyPoint(Vec3 const & v) const
{
    // @TODO: Quadtree
    Vec3 const * prev = std::get_if<Vec3>(&prev_point_);

    if (prev == nullptr && points_.empty()) {
        return nullptr;
    }

    // Sort points by proximity to target
    auto ref_cmp = [v](Vec3CRef a, Vec3CRef b) {
        return glm::length(v - a.get()) > glm::length(v - b.get());
    };
    std::priority_queue<Vec3CRef, std::vector<Vec3CRef>, decltype(ref_cmp)> hits(
        ref_cmp);

    for (Vec3 const & p : points_) {
        hits.push(p);
    }
    if (prev != nullptr) {
        hits.push(*prev);
    }

    Vec3 const & closest = hits.top();
    Vec3 const   reproj = glm::project(closest, modelview_, proj_, viewport_);

    return (glm::length(mouse_pos_ - reproj) < threshold_) ? &closest : nullptr;
}

void
DisplayWidget::addPoint()
{
    Vec3 unproj = glm::unProject(mouse_pos_, modelview_, proj_, viewport_);

    auto close_shape = [this](Vec3 const & a, Vec3 const & b) {
        addLine(a, b);
        prev_point_.emplace<std::monostate>();
        addPolygon(b);
    };

    bool const point_added = std::visit(
        Overloaded{// There is no previous point
                   [this, &unproj](std::monostate & /*unused*/) mutable {
                       Vec3 const * maybe_nearby = getNearbyPoint(unproj);
                       if (maybe_nearby != nullptr) {
                           // Point to add is on already existing line
                           // New line from an old line
                           prev_point_.emplace<Vec3CRef>(*maybe_nearby);
                           return false;
                       }

                       // Point to add is novel
                       // New line from scratch
                       prev_point_.emplace<Vec3>(std::move(unproj));
                       return true;
                   },
                   // Previous point is part of an already existing line
                   [this, &unproj, &close_shape](Vec3CRef & prev_ref) mutable {
                       assert(!points_.empty());

                       Vec3 const & prev = prev_ref.get();

                       Vec3 const * maybe_nearby = getNearbyPoint(unproj);
                       if (maybe_nearby != nullptr) {
                           if (maybe_nearby != &prev) {
                               // Point to add is on already existing line
                               close_shape(prev, *maybe_nearby);
                           }

                           return false;
                       }

                       // Point to add is novel
                       // Old -> New
                       Vec3 const & b = points_.emplace_back(std::move(unproj));
                       addLine(prev, b);
                       prev_point_.emplace<Vec3CRef>(b);
                       return true;
                   },
                   // Previous point is beginning of new line
                   [this, &unproj, &close_shape](Vec3 & prev) mutable {
                       Vec3 const * maybe_nearby = getNearbyPoint(unproj);
                       if (maybe_nearby != nullptr) {
                           if (maybe_nearby != &prev) {
                               // Point to add is on already existing line
                               Vec3 const & a =
                                   points_.emplace_back(std::move(prev));
                               close_shape(a, *maybe_nearby);
                               return true;
                           }

                           return false;
                       }

                       // Point to add is novel
                       // New -> New
                       Vec3 const & a = points_.emplace_back(std::move(prev));
                       Vec3 const & b = points_.emplace_back(std::move(unproj));
                       addLine(a, b);
                       prev_point_.emplace<Vec3CRef>(b);
                       return true;
                   }},
        prev_point_);

    if (point_added) {
        updatePoints();
    }
}

void
DisplayWidget::addLine(Vec3 const & a, Vec3 const & b)
{
    // @TODO: Add to unordered maps for indexing

    std::size_t const size = lines_.size();
    lines_.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(a),
                        std::forward_as_tuple(b));

    {
        BindOperation vbo{line_vbo_};

        line_vbo_.write(
            static_cast<int>(2 * size * sizeof(Vec3)), &a, sizeof(Vec3));
        line_vbo_.write(
            static_cast<int>((2 * size + 1) * sizeof(Vec3)), &b, sizeof(Vec3));
    }
}

double
DisplayWidget::integrate(Vec3 const & p1,
                         Vec3 const & diff,
                         Vec2 const & n,
                         Vec3 const & p)
{
    constexpr int    integrator_steps = 30;
    constexpr double slice = 1.0 / static_cast<double>(integrator_steps);
    auto             delta = diff / static_cast<float>(integrator_steps);

    double ans = 0.0;
    for (int i = 0; i < integrator_steps; ++i) {
        Vec3 const current_pos = p1 + static_cast<float>(i) * delta;

        Vec3 const  r = p - current_pos;
        float const denom = glm::dot(r, r);

        Vec2 grad{current_pos.y - p.y, p.x - current_pos.x}; // NOLINT
        grad /= denom;

        ans += glm::dot(grad, n);
    }

    return ans * slice;
}

void
DisplayWidget::addPolygon(Vec3 const & p)
{
    assert(lines_.size() > 1);

    // @TODO:
    // Keep finished polygons separate in order to facilitate multiple
    // Kutta conditions

    // @TODO: Walk polygon to find closed loop
    (void)p;

    // Assume closed polygon with lines in order
    std::vector<Vec3> centers;
    for (Line3 const & l : lines_) {
        Vec3 const & p1 = l.first;
        Vec3 const & p2 = l.second;

        constexpr float half = 0.5F;
        centers.emplace_back(half * (p1 + p2));
    }

    // Anderson p. 387

    constexpr Mat3 const ccw_rotation(Vec3(0.0F, -1.0F, 0.0F),
                                      Vec3(1.0F, 0.0F, 0.0F),
                                      Vec3(0.0F, 0.0F, 1.0F));

    std::size_t const n_lines = lines_.size();
    Eigen::MatrixXd   a = Eigen::MatrixXd::Zero(n_lines, n_lines);
    Eigen::MatrixXd   b(n_lines, 1);
    // For all line midpoints
    for (std::size_t i = 0; i < n_lines - 1; ++i) {
        Line3 const & l = lines_[i];

        Vec3 const & p1 = l.first;
        Vec3 const & p2 = l.second;

        Vec3 const diff = p2 - p1;

        Vec3 const n = ccw_rotation * glm::normalize(diff);
        b(i) = glm::dot(n, Vec3(vinf_, 0.0F));

        for (std::size_t j = 0; j < n_lines; ++j) {
            auto & c = centers[j];

            if (i == j) {
                a(i, j) = 0.0F;
            } else {
                // Integrate over every line
                a(i, j) = integrate(p1, diff, n, c);
            }
        }
    }
    constexpr double pi = 3.14159265358979;
    constexpr double tau = 2.0 * pi;
    a /= tau;

    // Replace last panel strength with Kutta condition
    a(n_lines - 1, 0) = 1.0;
    a(n_lines - 1, n_lines - 1) = 1.0;
    b(n_lines - 1, 0) = 0;

    Eigen::MatrixXd x = a.fullPivHouseholderQr().solve(b);

    auto vorticity = x.col(0);

    std::cout << vorticity << '\n';

    std::vector<GLfloat> line_data;
    line_data.reserve(2 * n_lines);

    for (Line3 const & l : lines_) {
        Vec3 const & p1 = l.first;
        Vec3 const & p2 = l.second;

        line_data.push_back(p1.x);
        line_data.push_back(p1.y);

        line_data.push_back(p2.x);
        line_data.push_back(p2.y);
    }

    std::vector<GLfloat> strength_data(n_lines);
    for (std::size_t i = 0; i < n_lines; ++i) {
        strength_data[i] = vorticity(i);
    }

    {
        BindOperation prog{field_prog_};

        glUniform4fv(lines_location_, n_lines, line_data.data());
        glUniform1fv(strengths_location_, n_lines, strength_data.data());
        glUniform1i(n_lines_location_, n_lines);
    }

    has_polygon_ = true;
}

void
DisplayWidget::updatePoints()
{
    {
        BindOperation vbo{point_vbo_};

        if (!points_.empty()) {
            point_vbo_.write(
                static_cast<int>((points_.size() - 1) * sizeof(Vec3)),
                &points_.back(),
                sizeof(Vec3));
        }

        Vec3 const * prev = std::get_if<Vec3>(&prev_point_);
        if (prev != nullptr) {
            point_vbo_.write(static_cast<int>(points_.size() * sizeof(Vec3)),
                             prev,
                             sizeof(Vec3));
        }
    }
}

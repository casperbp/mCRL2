// Author(s): Rimco Boudewijns and Sjoerd Cranen
// Copyright: see the accompanying file COPYING or copy at
// https://github.com/mCRL2org/mCRL2/blob/master/COPYING
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MCRL2_LTSGRAPH_GLSCENE_H
#define MCRL2_LTSGRAPH_GLSCENE_H

#include "graph.h"
#include "camera.h"
#include "shaders.h"

#include <array>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPainter>

/// \brief The scene contains the graph that is shown and the camera from which the graph is viewed. It performs
///        all the necessary OpenGL calls to render this graph as if shown from the camera. It assumes
///        to be the only place where OpenGL calls are performed as it does not keep track or reset the internal OpenGL
///        state between calls to render.
/// \todo The select functionality should be removed from this class as it does not rely on OpenGL.
class GLScene : private QOpenGLFunctions_3_3_Core
{
public:
  static const int handleSize = 4;
  static const int arrowheadSize = 12;

  /// \brief An enumeration that identifies the types of objects that
  ///        can be selected. The order determines priority during selection.
  enum class SelectableObject
  {
    none,     ///< Nothing was selected.
    edge,     ///< An edge was selected.
    label,    ///< An edge label was selected.
    slabel,   ///< An state label was selected.
    handle,   ///< An edge handle was selected.
    node      ///< A node was selected.
  };

  /// \brief A structure that identifies a selectable object from m_graph.
  struct Selection
  {
    SelectableObject selectionType; ///< The type of object.
    std::size_t index;              ///< The index of the object in m_graph.

    /// \returns True whenever this is a valid selection.
    bool has_selection() { return selectionType != SelectableObject::none; }

    bool operator!=(const Selection& other)
    {
      return index != other.index || selectionType != other.selectionType;
    }
  };

  /// \brief Constructor.
  /// \param glwidget The widget where this scene is drawn
  /// \param g The graph that is to be visualised by this object.
  GLScene(QOpenGLWidget& glwidget, const Graph::Graph& g);

  /// \brief Initializes all state and data required for rendering.
  void initialize();

  /// \brief Updates the state of the scene.
  void update();

  /// \brief Builds static data based on the current graph.
  void rebuild();

  /// \brief Render the scene.
  void render(QPainter& painter);

  /// \brief Resize the OpenGL viewport.
  void resize(std::size_t width, std::size_t height);

  /// \brief Retrieve the object at viewport coordinate (x, y).
  /// \returns A record that represents the selected object.
  Selection select(int x, int y);

  /// Getters

  bool drawStateLabels() const { return m_drawstatelabels; }
  bool drawStateNumbers() const { return m_drawstatenumbers; }
  bool drawTransitionLabels() const { return m_drawtransitionlabels; }
  bool drawSelfLoops() const { return m_drawselfloops; }
  std::size_t nodeSize() const { return m_size_node; }
  std::size_t fontSize() const { return m_fontsize; }
  float nodeSizeScaled() const { return nodeSize() * m_device_pixel_ratio; }
  float fontSizeScaled() const { return fontSize() * m_device_pixel_ratio; }
  float handleSizeScaled() const { return handleSize * m_device_pixel_ratio; }
  float arrowheadSizeScaled() const { return arrowheadSize * m_device_pixel_ratio; }

  /// \brief Computes how long the world vector (length, 0, 0) at pos is when drawn on the screen
  float sizeOnScreen(const QVector3D& pos, float length) const;

  ArcballCameraView& camera() { return m_camera;  }
  const ArcballCameraView& camera() const { return m_camera;  }

  /**
  * \brief Calculates control points for the arc described by a origin, handle and destination node.
  * \param from Position of the starting point of the arc.
  * \param via Position of the handle associated with the arc.
  * \param to Position of the end point of the arc.
  * \param selfLoop True if the arc represents a self loop.
  * \returns Four points describing a cubic Bezier curve (start, control1, control2, end).
  */
  static constexpr std::array<QVector3D, 4> calculateArc(const QVector3D& from, const QVector3D& via,
    const QVector3D& to, bool selfLoop);

  /// Setters

  void setDrawTransitionLabels(bool drawLabels) { m_drawtransitionlabels = drawLabels; }
  void setDrawStateLabels(bool drawLabels) { m_drawstatelabels = drawLabels; }
  void setDrawStateNumbers(bool drawLabels) { m_drawstatenumbers = drawLabels; }
  void setDrawSelfLoops(bool drawLoops) { m_drawselfloops = drawLoops; }
  void setDrawInitialMarking(bool drawMark) { m_drawinitialmarking = drawMark; }
  void setDrawFog(bool enabled) { m_drawfog = enabled; }

  void setNodeSize(std::size_t size) { m_size_node = size; }
  void setFontSize(int size) { m_fontsize = size; m_font.setPixelSize(m_fontsize); }
  void setFogDistance(int value) { m_fogdensity = 1.0f / (value + 1); }
  void setDevicePixelRatio(float device_pixel_ratio) { m_device_pixel_ratio = device_pixel_ratio; }

private:
  /// \returns The color of an object receiving fogAmount amount of fog.
  QVector3D applyFog(const QVector3D& color, float fogAmount);

  /// \brief Renders text at a given world position, facing the camera and center aligned.
  void drawCenteredText3D(QPainter& painter, const QString& text, const QVector3D& position, const QVector3D& color);

  /// \brief Renders static text at a given world position, facing the camera and center aligned.
  void drawCenteredStaticText3D(QPainter& painter, const QStaticText& text, const QVector3D& position, const QVector3D& color);

  /// \returns Whether the given point (no radius) is visible based on the camera viewdistance and fog amount (if enabled).
  /// \param fog The amount of fog that a given point receives.
  bool isVisible(const QVector3D& position, float& fog);

  /// \brief Renders a single edge.
  /// \param i The index of the edge to render.
  void renderEdge(std::size_t i, const QMatrix4x4& viewProjMatrix);

  /// \brief Renders a single edge handle.
  /// \param i The index of the edge of the handle to render.
  void renderHandle(std::size_t i, const QMatrix4x4& viewProjMatrix);

  /// \brief Renders a single node.
  /// \param i The index of the node to render.
  /// \param transparent Allow nodes to be rendered with transparency, required for the two passes (opaque first, followed by transparent items).
  void renderNode(std::size_t i, const QMatrix4x4& viewProjMatrix, bool transparent);

  /// \brief Renders a single edge label.
  /// \param i The index of the edge of the label to render.
  void renderTransitionLabel(QPainter& painter, std::size_t i);

  /// \brief Renders a single state label.
  /// \param i The index of the state of the label to render.
  void renderStateLabel(QPainter& painter, std::size_t i);

  /// \brief Renders a single state number.
  /// \param i The index of the state number to render.
  void renderStateNumber(QPainter& painter, std::size_t i);

  /// \brief Select an object of type @type on (pixel) viewport coordinates x, y.
  /// \returns true if an object was selected.
  bool selectObject(Selection& s, int x, int y, SelectableObject type);

  /// \returns A rotation such that an object at the given position faces the camera.
  QQuaternion sphericalBillboard(const QVector3D& position) const;

  QOpenGLWidget& m_glwidget; /// The widget where this scene is drawn
  const Graph::Graph& m_graph;     /// The graph that is being visualised.

  ArcballCameraView m_camera;
  float m_device_pixel_ratio;
  QFont m_font;

  /// \brief The shader to draw uniformly filled three dimensional objects.
  GlobalShader m_global_shader;

  /// \brief The shader to draw arcs, uses control points to position the vertices.
  ArcShader m_arc_shader;

  bool m_drawtransitionlabels = true;   ///< Transition labels are only drawn if this field is true.
  bool m_drawstatelabels      = false;  ///< State labels are only drawn if this field is true.
  bool m_drawstatenumbers     = false;  ///< State numbers are only drawn if this field is true.
  bool m_drawselfloops        = true;   ///< Self loops are only drawn if this field is true.
  bool m_drawinitialmarking   = true; ///< The initial state is marked if this field is true.
  std::size_t m_size_node     = 20; ///< The size of a node (radius).
  int m_fontsize              = 16; ///< The size of the font.

  bool m_drawfog = true; ///< Enable drawing fog.
  float m_fogdensity = 0.0005f; ///< The density of the fog.

  /// The vertex layout and vertex buffer object for all objects with the 3 float per vertex layout.
  QOpenGLVertexArrayObject m_vertexarray;
  QOpenGLBuffer m_vertexbuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

  /// \brief The background color of the scene.
  QVector3D m_clearColor = QVector3D(1.0f, 1.0f, 1.0f);

  /// \brief For each label store a QStaticText object, which is more efficient for text that rarely changes its layout.
  std::vector<QStaticText> m_state_labels;
  std::vector<QStaticText> m_transition_labels;
};

constexpr std::array<QVector3D, 4> GLScene::calculateArc(const QVector3D& from, const QVector3D& via,
  const QVector3D& to, bool selfLoop)
{
  // Pick a point a bit further from the middle point between the nodes.
  // This is an affine combination of the points 'via' and '(from + to) / 2.0f'.
  const QVector3D base = via * 1.33333f - (from + to) / 6.0f;

  if (selfLoop)
  {
    // For self-loops, the control points need to lie apart, we'll spread
    // them in x-y direction.
    const QVector3D diff = QVector3D::crossProduct(base - from, QVector3D(0, 0, 1));
    const QVector3D n_diff = diff * ((via - from).length() / (diff.length() * 2.0f));
    return std::array<QVector3D, 4>{from, base + n_diff, base - n_diff, to};
  }
  else
  {
    // Standard case: use the same position for both points.
    //return std::array<QVector3D, 4>{from, base, base, to};
    // Method 2: project the quadratic bezier curve going through the handle onto a cubic bezier curve.
    const QVector3D control = via + (via - ((from + to) / 2.0f));
    return std::array<QVector3D, 4>{from,
      0.33333f * from + 0.66666f * control,
      0.33333f * to + 0.66666f * control,
      to};
  }
}

#endif // MCRL2_LTSGRAPH_GLSCENE_H

#pragma once
#include <Curve/Palette/CurveEditionSettings.hpp>
#include <wobjectdefs.h>
#include <Curve/Point/CurvePointModel.hpp>
#include <Curve/Point/CurvePointView.hpp>
#include <Curve/Segment/CurveSegmentModel.hpp>
#include <Curve/Segment/CurveSegmentView.hpp>
#include <QObject>
#include <QPoint>
#include <QRect>
#include <score/command/Dispatchers/CommandDispatcher.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/model/IdentifiedObjectMap.hpp>
#include <score/selection/SelectionDispatcher.hpp>
#include <score/widgets/GraphicsItem.hpp>
#include <score_plugin_curve_export.h>

class QActionGroup;
class QMenu;
namespace Curve
{
class SegmentList;
struct Style;
class View;
class Model;

class SCORE_PLUGIN_CURVE_EXPORT Presenter final : public QObject
{
  W_OBJECT(Presenter)
public:
  Presenter(
      const score::DocumentContext& lst,
      const Curve::Style&,
      const Model&,
      View*,
      QObject* parent);
  virtual ~Presenter();

  const auto& points() const
  {
    return m_points;
  }
  const auto& segments() const
  {
    return m_segments;
  }

  // Removes all the points & segments
  void clear();

  const Model& model() const
  {
    return m_model;
  }
  View& view() const
  {
    return *m_view;
  }

  void setRect(const QRectF& rect);

  void enableActions(bool);

  // Changes the colors
  void enable();
  void disable();

  Curve::EditionSettings& editionSettings()
  {
    return m_editionSettings;
  }

  void fillContextMenu(QMenu&, const QPoint&, const QPointF&);

  void removeSelection();
  QActionGroup& actions() const
  {
    return *m_actions;
  }

  // Used to allow moving outside [0; 1] when in the panel view.
  bool boundedMove() const
  {
    return m_boundedMove;
  }
  void setBoundedMove(bool b)
  {
    m_boundedMove = b;
  }

  QRectF rect() const
  {
    return m_localRect;
  }

public:
  void contextMenuRequested(const QPoint& arg_1, const QPointF& arg_2) W_SIGNAL(contextMenuRequested, arg_1, arg_2);

private:
  // Context menu actions
  void updateSegmentsType(const UuidKey<Curve::SegmentFactory>& segment);

  // Setup utilities
  void setPos(PointView&);
  void setPos(SegmentView&);
  void setupSignals();
  void setupView();
  void setupStateMachine();

  // Adding
  void addPoint(PointView*);
  void addSegment(SegmentView*);

  void addPoint_impl(PointView*);
  void addSegment_impl(SegmentView*);

  void setupPointConnections(PointView*);
  void setupSegmentConnections(SegmentView*);
  void modelReset();

  const SegmentList& m_curveSegments;
  Curve::EditionSettings m_editionSettings;
  QRectF m_localRect;

  const Model& m_model;
  graphics_item_ptr<View> m_view;

  IdContainer<PointView, PointModel> m_points;
  IdContainer<SegmentView, SegmentModel> m_segments;

  // Required dispatchers
  CommandDispatcher<> m_commandDispatcher;

  const Curve::Style& m_style;

  QMenu* m_contextMenu{};
  QActionGroup* m_actions{};

  bool m_enabled = true;
  bool m_boundedMove = true;
};
}

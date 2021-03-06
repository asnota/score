#pragma once
#include <QGraphicsItem>
#include <QStaticText>
#include <QString>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentViewConstants.hpp>

class QGraphicsSceneMouseEvent;

namespace Scenario
{
class IntervalView;
class IntervalHeader
    : public QObject
    , public QGraphicsItem
{
public:
  enum class State
  {
    Hidden,     // No rack, we show nothing
    RackHidden, // There is at least a hidden rack in the interval
    RackShown   // There is a rack currently shown
  };

  using QGraphicsItem::QGraphicsItem;

  static constexpr int static_type()
  {
    return QGraphicsItem::UserType + ItemType::IntervalHeader;
  }
  int type() const override
  {
    return static_type();
  }

  void setIntervalView(IntervalView* view)
  {
    m_view = view;
  }

  static constexpr double headerHeight()
  {
    return IntervalHeaderHeight;
  }

  void setWidth(double width);
  void setText(const QString& text);

  virtual void setState(State s)
  {
    if (s == m_state)
      return;

    if (m_state == State::Hidden)
      show();
    else if (s == State::Hidden)
      hide();

    m_state = s;
    update();
  }

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) final override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) final override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) final override;

protected:
  virtual void on_textChange()
  {
  }
  IntervalView* m_view{};
  State m_state{};
  double m_width{};
  QString m_text;
};
}

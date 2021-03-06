#pragma once
#include <ossia/editor/expression/expression.hpp>

#include <Engine/Executor/Component.hpp>
namespace ossia
{
class time_sync;
struct time_value;
}
namespace Scenario
{
class TimeSyncModel;
}

namespace Engine
{
namespace Execution
{

class SCORE_PLUGIN_ENGINE_EXPORT TimeSyncRawPtrComponent final
    : public Execution::Component
{
  COMMON_COMPONENT_METADATA("0e2d2259-8cb7-4b05-b43a-746c4f5bc80c")
public:
  static const constexpr bool is_unique = true;
  TimeSyncRawPtrComponent(
      const Scenario::TimeSyncModel& element,
      const Engine::Execution::Context& ctx,
      const Id<score::Component>& id,
      QObject* parent);

  void cleanup();

  //! To be called from the GUI thread
  ossia::expression_ptr makeTrigger() const;

  //! To be called from the API edition queue
  void onSetup(ossia::time_sync* ptr, ossia::expression_ptr exp);

  ossia::time_sync* OSSIATimeSync() const;
  const Scenario::TimeSyncModel& scoreTimeSync() const;

private:
  void updateTrigger();
  void on_GUITrigger();
  ossia::time_sync* m_ossia_node{};
  const Scenario::TimeSyncModel& m_score_node;
};
}
}

#pragma once
#include <ossia/dataflow/node_process.hpp>

#include <Engine/Executor/ProcessComponent.hpp>
#include <Media/Step/Model.hpp>
namespace Engine
{
namespace Execution
{

class StepComponent final
    : public ::Engine::Execution::
          ProcessComponent_T<Media::Step::Model, ossia::node_process>
{
  COMPONENT_METADATA("5b9c03cb-d062-40ee-b2a2-88279b088d4d")
public:
  StepComponent(
      Media::Step::Model& element,
      const ::Engine::Execution::Context& ctx,
      const Id<score::Component>& id,
      QObject* parent);

  void recompute();

  ~StepComponent();

private:
};

using StepComponentFactory
    = ::Engine::Execution::ProcessComponentFactory_T<StepComponent>;
}
}

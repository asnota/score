// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "DefaultClockManager.hpp"

#include <ossia/dataflow/execution_state.hpp>
#include <ossia/dataflow/graph/graph_interface.hpp>
#include <ossia/editor/state/state_element.hpp>

#include <Engine/Executor/BaseScenarioComponent.hpp>
#include <Engine/Executor/EventComponent.hpp>
#include <Engine/Executor/IntervalComponent.hpp>
#include <Engine/Executor/Settings/ExecutorModel.hpp>
#include <Engine/OSSIA2score.hpp>
#include <Engine/score2OSSIA.hpp>

namespace Engine
{
namespace Execution
{
DefaultClockManager::~DefaultClockManager() = default;
DefaultClockManager::DefaultClockManager(const Context& ctx)
    : ClockManager{ctx}
{
}

void DefaultClockManager::prepareExecution(
    const TimeVal& t, BaseScenarioElement& bs)
{
  IntervalComponentBase& comp = bs.baseInterval();
  if(context.settings.getValueCompilation())
  {
    comp.interval().duration.setPlayPercentage(0);

    // Send the first state
    const auto& oc = comp.OSSIAInterval();
    oc->get_start_event().tick(ossia::Zero, 0., ossia::Zero);
    context.plugin.execGraph->state(*context.plugin.execState);

    if (t != TimeVal::zero())
      oc->offset(context.time(t));

    context.plugin.execState->commit();
  }
}

void DefaultClockManager::play_impl(const TimeVal& t, BaseScenarioElement& bs)
{
  prepareExecution(t, bs);
  try
  {
    bs.baseInterval().OSSIAInterval()->start_and_tick();
    bs.baseInterval().executionStarted();
  }
  catch (const std::exception& e)
  {
    qDebug() << e.what();
  }
}

void DefaultClockManager::pause_impl(BaseScenarioElement& bs)
{
  bs.baseInterval().pause();
}

void DefaultClockManager::resume_impl(BaseScenarioElement& bs)
{
  bs.baseInterval().resume();
}

void DefaultClockManager::stop_impl(BaseScenarioElement& bs)
{
  bs.baseInterval().stop();
}

ControlClockFactory::~ControlClockFactory() = default;

ControlClock::ControlClock(const Engine::Execution::Context& ctx)
    : ClockManager{ctx}
    , m_default{ctx}
    , m_clock{*ctx.scenario.baseInterval().OSSIAInterval(), 1.}
{
  m_clock.set_granularity(std::chrono::microseconds(
      context.doc.app.settings<Settings::Model>().getRate() * 1000));

  // TODO this should be the case also with other clocks
  m_clock.set_exec_status_callback([=](ossia::clock::exec_status c) {
    if (c == ossia::clock::exec_status::STOPPED)
    {
      context.scenario.endEvent().OSSIAEvent()->tick(
          ossia::Zero, 0., ossia::Zero);
      context.plugin.execGraph->state(*context.plugin.execState);
      context.plugin.execState->commit();

      context.scenario.finished();
    }
  });
}

void ControlClock::play_impl(
    const TimeVal& t, Engine::Execution::BaseScenarioElement& bs)
{
  m_default.prepareExecution(t, bs);
  try
  {
    m_clock.start_and_tick();
    bs.baseInterval().executionStarted();
  }
  catch (const std::exception& e)
  {
    qDebug() << e.what();
  }
}

void ControlClock::pause_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_clock.pause();
  m_default.pause();
}

void ControlClock::resume_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_default.resume();
  m_clock.resume();
}

void ControlClock::stop_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_clock.stop();
  m_default.stop();
}

bool ControlClock::paused() const
{
  return m_clock.paused();
}

QString ControlClockFactory::prettyName() const
{
  return QObject::tr("Default");
}

std::unique_ptr<ClockManager>
ControlClockFactory::make(const Engine::Execution::Context& ctx)
{
  return std::make_unique<ControlClock>(ctx);
}

Engine::Execution::time_function
ControlClockFactory::makeTimeFunction(const score::DocumentContext& ctx) const
{
  return &score_to_ossia::defaultTime;
}

Engine::Execution::reverse_time_function
ControlClockFactory::makeReverseTimeFunction(
    const score::DocumentContext& ctx) const
{
  return &ossia_to_score::defaultTime;
}
}
}

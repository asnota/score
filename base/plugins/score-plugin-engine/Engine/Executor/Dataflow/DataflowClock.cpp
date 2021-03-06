#include "DataflowClock.hpp"

#include <ossia/audio/audio_parameter.hpp>
#include <ossia/audio/audio_protocol.hpp>
#include <ossia/dataflow/graph/graph_interface.hpp>

#include <Engine/Executor/IntervalComponent.hpp>
#include <Engine/Executor/Settings/ExecutorModel.hpp>
#include <Engine/Protocols/OSSIADevice.hpp>
#include <Engine/Protocols/Settings/Model.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentModel.hpp>
namespace Dataflow
{
Clock::Clock(const Engine::Execution::Context& ctx)
    : ClockManager{ctx}
    , m_default{ctx}
    , m_plug{context.doc.plugin<Engine::Execution::DocumentPlugin>()}
{
  auto& bs = context.scenario;
  if (!bs.active())
    return;
}

Clock::~Clock()
{
}

void Clock::play_impl(
    const TimeVal& t, Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;

  m_plug.execGraph->print(std::cerr);
  m_cur = &bs;
  m_default.play(t);

  resume_impl(bs);
}

void Clock::pause_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = true;
  m_plug.audioProto().set_tick([](auto&&...) {});
  m_default.pause();
}

void Clock::resume_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;
  m_default.resume();
  auto tick = m_plug.context().settings.getTick();
  auto commit = m_plug.context().settings.getCommit();

  ossia::tick_setup_options opt;
  if (tick == Engine::Execution::Settings::TickPolicies{}.Buffer)
    opt.tick = ossia::tick_setup_options::Buffer;
  else if (tick == Engine::Execution::Settings::TickPolicies{}.ScoreAccurate)
    opt.tick = ossia::tick_setup_options::ScoreAccurate;
  else if (tick == Engine::Execution::Settings::TickPolicies{}.Precise)
    opt.tick = ossia::tick_setup_options::Precise;

  if (commit == Engine::Execution::Settings::CommitPolicies{}.Default)
    opt.commit = ossia::tick_setup_options::Default;
  else if (commit == Engine::Execution::Settings::CommitPolicies{}.Ordered)
    opt.commit = ossia::tick_setup_options::Ordered;
  else if (commit == Engine::Execution::Settings::CommitPolicies{}.Priorized)
    opt.commit = ossia::tick_setup_options::Priorized;
  else if (commit == Engine::Execution::Settings::CommitPolicies{}.Merged)
    opt.commit = ossia::tick_setup_options::Merged;

  if (m_plug.context().settings.getBench())
  {
    auto tick = ossia::make_tick(
        opt, *m_plug.execState, *m_plug.execGraph,
        *m_cur->baseInterval().OSSIAInterval());

    m_plug.audioProto().set_tick([tick, plug = &m_plug](auto&&... args) {
      // Run some commands if they have been submitted.
      Engine::Execution::ExecutionCommand c;
      while (plug->context().executionQueue.try_dequeue(c))
      {
        c();
      }

      static int i = 0;
      if (i % 50 == 0)
      {
        ossia::bench_ptr()->measure = true;
        auto t0 = std::chrono::steady_clock::now();
        tick(args...);
        auto t1 = std::chrono::steady_clock::now();
        auto total
            = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0)
                  .count();

        plug->slot_bench(*ossia::bench_ptr(), total);
        for (auto& p : *ossia::bench_ptr())
        {
          p.second = {};
        }
      }
      else
      {
        ossia::bench_ptr()->measure = false;
        tick(args...);
      }

      i++;
    });
  }
  else
  {
    auto tick = ossia::make_tick(
        opt, *m_plug.execState, *m_plug.execGraph,
        *m_cur->baseInterval().OSSIAInterval());

    m_plug.audioProto().set_tick([tick, plug = &m_plug](auto&&... args) {
      // Run some commands if they have been submitted.
      Engine::Execution::ExecutionCommand c;
      while (plug->context().executionQueue.try_dequeue(c))
      {
        c();
      }

      tick(args...);
    });
  }
}

void Clock::stop_impl(Engine::Execution::BaseScenarioElement& bs)
{
  m_paused = false;
  m_plug.finished();

  auto& proto = m_plug.audioProto();
  proto.set_tick([](unsigned long, double) {});
  m_default.stop();
}

bool Clock::paused() const
{
  return m_paused;
}

std::unique_ptr<Engine::Execution::ClockManager>
ClockFactory::make(const Engine::Execution::Context& ctx)
{
  return std::make_unique<Clock>(ctx);
}

Engine::Execution::time_function
ClockFactory::makeTimeFunction(const score::DocumentContext& ctx) const
{
  auto rate = ctx.app.settings<Audio::Settings::Model>().getRate();
  return [=](const TimeVal& v) -> ossia::time_value {
    // Go from milliseconds to samples
    // 1000 ms = sr samples
    // x ms    = k samples
    return v.isInfinite()
               ? ossia::Infinite
               : ossia::time_value{int64_t(std::llround(rate * v.msec() / 1000.))};
  };
}

Engine::Execution::reverse_time_function
ClockFactory::makeReverseTimeFunction(const score::DocumentContext& ctx) const
{
  auto rate = ctx.app.settings<Audio::Settings::Model>().getRate();
  return [=](const ossia::time_value& v) -> TimeVal {
    return v.infinite() ? TimeVal{PositiveInfinity{}}
                        : TimeVal::fromMsecs(1000. * v.impl / rate);
  };
}

QString ClockFactory::prettyName() const
{
  return QObject::tr("Audio");
}
}

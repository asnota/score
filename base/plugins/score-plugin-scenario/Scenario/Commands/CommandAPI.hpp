#pragma once
#include <score/command/AggregateCommand.hpp>
#include <Scenario/Palette/ScenarioPoint.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/command/Dispatchers/MacroCommandDispatcher.hpp>
#include <memory>
#include <score/document/DocumentContext.hpp>
namespace Scenario::Command
{
class Macro
{
    RedoMacroCommandDispatcher<score::AggregateCommand> m;
  public:
    Macro(score::AggregateCommand* cmd, const score::DocumentContext& ctx);

    StateModel&
    createState(
        const Scenario::ProcessModel& scenar
        , const Id<EventModel>& ev
        , double y);

    std::tuple<TimeSyncModel&, EventModel&, StateModel&>
    createDot(
        const Scenario::ProcessModel& scenar
        , Scenario::Point pt);

    IntervalModel& createBox(
        const Scenario::ProcessModel& scenar
        , TimeVal start, TimeVal end, double y);

    IntervalModel& createIntervalAfter(
        const Scenario::ProcessModel& scenar
        , const Id<Scenario::StateModel>& state
        , Scenario::Point pt);

    Process::ProcessModel* createProcess(
        const Scenario::IntervalModel& interval
        , const UuidKey<Process::ProcessModel>& key
        , const QString& data);

    Process::ProcessModel* loadProcessInSlot(
        const Scenario::IntervalModel& interval
        , const QJsonObject& procdata);

    void createSlot(
        const Scenario::IntervalModel& interval);

    void addLayer(
        const Scenario::IntervalModel& interval
        , int slot_index
        , const Process::ProcessModel& proc);

    void addLayerToLastSlot(
        const Scenario::IntervalModel& interval
        , const Process::ProcessModel& proc);

    void addLayer(
        const Scenario::SlotPath& slotpath
        , const Process::ProcessModel& proc);

    void showRack(
        const Scenario::IntervalModel& interval);

    void resizeSlot(
        const Scenario::IntervalModel& interval
        , const SlotPath& slotPath
        , double newSize);

    void resizeSlot(
        const Scenario::IntervalModel& interval
        , SlotPath&& slotPath
        , double newSize);

    void duplicate(
        const Scenario::ProcessModel& scenario
        , const Scenario::IntervalModel& itv);

    void pasteElements(
        const Scenario::ProcessModel& scenario
        , const QJsonObject& objs
        , Scenario::Point pos);

    void mergeTimeSyncs(
        const Scenario::ProcessModel& scenario
        , const Id<TimeSyncModel>& a
        , const Id<TimeSyncModel>& b);

    void removeProcess(
        const Scenario::IntervalModel& interval
        , const Id<Process::ProcessModel>& proc);

    void removeElements(
        const Scenario::ProcessModel& scenario
        , const Selection& sel);

    void addMessages(
        const Scenario::StateModel& state
        , State::MessageList msgs);

    void submit(score::Command* cmd);
    void commit();
};
}

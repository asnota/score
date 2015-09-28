#pragma once
#include <ProcessInterface/TimeValue.hpp>
#include <iscore/tools/SettableIdentifier.hpp>

#include <Process/ScenarioModel.hpp>
#include <Document/Constraint/ConstraintModel.hpp>
#include <Document/Event/EventModel.hpp>
#include <Document/TimeNode/TimeNodeModel.hpp>

#include <iscore/document/DocumentInterface.hpp>
#include "Commands/Scenario/Deletions/ClearConstraint.hpp"
#include "Document/Constraint/Rack/RackModel.hpp"
#include "Document/Constraint/Rack/Slot/SlotModel.hpp"

#include "Document/Constraint/ViewModels/ConstraintViewModel.hpp"
#include "Process/Algorithms/StandardCreationPolicy.hpp"
#include "Process/Algorithms/VerticalMovePolicy.hpp"
#include <ProcessInterface/LayerModel.hpp>

#include "Tools/dataStructures.hpp"

using namespace iscore;
using namespace Scenario::Command;

/**
 * @brief The displacementPolicy class
 * This class allows to implement multiple displacement behaviors.
 */
class CommonDisplacementPolicy
{
public:
    /**
     * @brief computeDisplacement
     * This function (used by move commands) compute the displcements generated by moving one or more timenodes
     * @param draggedElements
     * Elements of the scenario which were moved by the user
     * @param deltaTime
     * the amount of time moved
     * @param elementsProperties
     * the elements affected by the moving process and their old and new parameters (dates and durations) will be stored here.
     * IMPORTANT : elementsProperties may not be empty (in case of an update), in such case old datse of existing elements must be kept as is
     */
    /*
    static
    void
    computeDisplacement(
            ScenarioModel& scenario,
            const QVector<Id<TimeNodeModel>>& draggedElements,
            const TimeValue& deltaTime,
            ElementsProperties& elementsProperties) = 0;*/

      /**
     * @brief updatePositions
     * @param elementsPropertiesToUpdate
     * @param useNewValues
     * if false, use old values of properties (undo)
     */
    template<typename ProcessScaleMethod>
    static
    void
    updatePositions(ScenarioModel& scenario, ProcessScaleMethod&& scaleMethod, ElementsProperties& elementsPropertiesToUpdate, bool useNewValues)
    {
        // update each affected timenodes
        for(auto& curTimenodePropertiesToUpdate_id : elementsPropertiesToUpdate.timenodes.keys())
        {
            auto& curTimenodeToUpdate = scenario.timeNode(curTimenodePropertiesToUpdate_id);
            auto& curTimenodePropertiesToUpdate = elementsPropertiesToUpdate.timenodes[curTimenodePropertiesToUpdate_id];

            if(useNewValues)//redo
            {
               curTimenodeToUpdate.setDate(curTimenodePropertiesToUpdate.newDate);
            }else//undo
            {
               curTimenodeToUpdate.setDate(curTimenodePropertiesToUpdate.oldDate);
            }

            // update related events
            for (const auto& event : curTimenodeToUpdate.events())
            {
                scenario.events.at(event).setDate(curTimenodeToUpdate.date());
            }
        }

        // update affected constraints
        for(auto& curConstraintPropertiesToUpdate_id : elementsPropertiesToUpdate.constraints.keys())
        {
            auto& curConstraintToUpdate = scenario.constraints.at(curConstraintPropertiesToUpdate_id);
            auto& curConstraintPropertiesToUpdate = elementsPropertiesToUpdate.constraints[curConstraintPropertiesToUpdate_id];


            if(useNewValues)// update durations
            {
                // compute default duration here
                const auto& startDate = scenario.event(scenario.state(curConstraintToUpdate.startState()).eventId()).date();
                const auto& endDate = scenario.event(scenario.state(curConstraintToUpdate.endState()).eventId()).date();

                TimeValue defaultDuration = endDate - startDate;

                // check if start date changed
                if (!(curConstraintToUpdate.startDate() - startDate).isZero())
                {
                    curConstraintToUpdate.setStartDate(startDate);
                }

                //change durations
                curConstraintToUpdate.duration.setDefaultDuration(defaultDuration);

                curConstraintToUpdate.duration.setMinDuration(curConstraintPropertiesToUpdate.newMin);
                curConstraintToUpdate.duration.setMaxDuration(curConstraintPropertiesToUpdate.newMax);
            }
            else// IF UNDO THEN RESTORE THE STATE OF THE CONSTRAINTS
            {
                // Now we have to restore the state of each constraint that might have been modified
                // during this command.
                auto& savedConstraintData = elementsPropertiesToUpdate.constraints[curConstraintPropertiesToUpdate_id].savedDisplay;

                // 1. Clear the constraint
                // TODO Don't use a command since it serializes a ton of unused stuff.
                ClearConstraint clear_cmd{Path<ConstraintModel>{savedConstraintData.first.first}};
                clear_cmd.redo();

                auto& constraint = savedConstraintData.first.first.find();
                // 2. Restore the rackes & processes.

                // TODO if possible refactor this with ReplaceConstraintContent and ConstraintModel::clone
                // Be careful however, the code differs in subtle ways
                {
                    ConstraintModel src_constraint{
                            Deserializer<DataStream>{savedConstraintData.first.second},
                            &constraint}; // Temporary parent

                    std::map<const Process*, Process*> processPairs;

                    // Clone the processes
                    for(const auto& sourceproc : src_constraint.processes)
                    {
                        auto newproc = sourceproc.clone(sourceproc.id(), &constraint);

                        processPairs.insert(std::make_pair(&sourceproc, newproc));
                        constraint.processes.add(newproc);
                    }

                    // Clone the rackes
                    for(const auto& sourcerack : src_constraint.racks)
                    {
                        // A note about what happens here :
                        // Since we want to duplicate our process view models using
                        // the target constraint's cloned shared processes (they might setup some specific data),
                        // we maintain a pair mapping each original process to their cloned counterpart.
                        // We can then use the correct cloned process to clone the process view model.
                        auto newrack = new RackModel{
                                sourcerack,
                                sourcerack.id(),
                                [&] (const SlotModel& source, SlotModel& target)
                                {
                                    for(const auto& lm : source.layers)
                                    {
                                        // We can safely reuse the same id since it's in a different slot.
                                        Process* proc = processPairs[&lm.processModel()];
                                        // TODO harmonize the order of parameters (source first, then new id)
                                        target.layers.add(proc->cloneLayer(lm.id(), lm, &target));
                                    }
                                },
                                &constraint};
                        constraint.racks.add(newrack);
                    }
                }

                // 3. Restore the correct rackes in the constraint view models
                if(constraint.processes.size() != 0)
                {
                    for(auto& viewmodel : constraint.viewModels())
                    {
                        viewmodel->showRack(curConstraintPropertiesToUpdate.savedDisplay.second[viewmodel->id()]);
                    }
                }
            }

            emit scenario.constraintMoved(curConstraintToUpdate);
        }
    }
};




namespace StandardDisplacementPolicy
{
    // pick out each timeNode that need to move when firstTimeNodeMovedId is moving
    void getRelatedTimeNodes(
            ScenarioModel& scenario,
            const Id<TimeNodeModel>& firstTimeNodeMovedId,
            std::vector<Id<TimeNodeModel> >& translatedTimeNodes);


    template<typename ProcessScaleMethod>
    void updatePositions(
            ScenarioModel& scenario,
            const QVector<Id<TimeNodeModel> >& translatedTimeNodes,
            const TimeValue& deltaTime,
            ProcessScaleMethod&& scaleMethod)
    {
        for (const auto& timeNode_id : translatedTimeNodes)
        {
            auto& timeNode = scenario.timeNode(timeNode_id);
            timeNode.setDate(timeNode.date() + deltaTime);
            for (const auto& event : timeNode.events())
            {
                scenario.event(event).setDate(timeNode.date());
            }
        }

        for(auto& constraint : scenario.constraints)
        {
            const auto& startDate = scenario.event(scenario.state(constraint.startState()).eventId()).date();
            const auto& endDate = scenario.event(scenario.state(constraint.endState()).eventId()).date();

            TimeValue newDuration = endDate - startDate;

            if (!(constraint.startDate() - startDate).isZero())
            {
                constraint.setStartDate(startDate);
            }

            if(!(constraint.duration.defaultDuration() - newDuration).isZero())
            {
                ConstraintDurations::Algorithms::changeAllDurations(constraint, newDuration);
                for(auto& process : constraint.processes)
                {
                    scaleMethod(process, newDuration);
                }
            }

            emit scenario.constraintMoved(constraint);
        }
    }
}

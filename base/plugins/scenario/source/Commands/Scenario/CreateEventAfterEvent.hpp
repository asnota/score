#pragma once
#include <core/presenter/command/SerializableCommand.hpp>
#include <tools/ObjectPath.hpp>
#include <QMap>
#include <tuple>

struct EventData;

#include <tests/helpers/ForwardDeclaration.hpp>
namespace Scenario
{
	namespace Command
	{
		/**
 * @brief The CreateEventAfterEventCommand class
 *
 * This Command creates a constraint and another event in a scenario,
 * starting from an event selected by the user.
 */
		class CreateEventAfterEvent : public iscore::SerializableCommand
		{
#include <tests/helpers/FriendDeclaration.hpp>
			public:
				CreateEventAfterEvent();
				CreateEventAfterEvent(ObjectPath&& scenarioPath, EventData data);
				CreateEventAfterEvent& operator=(CreateEventAfterEvent&&) = default;

				virtual void undo() override;
				virtual void redo() override;
				virtual int id() const override;
				virtual bool mergeWith(const QUndoCommand* other) override;

			protected:
				virtual void serializeImpl(QDataStream&) override;
				virtual void deserializeImpl(QDataStream&) override;

			private:
				ObjectPath m_path;

				int m_createdConstraintId{};
				int m_createdBoxId{};
				int m_createdEventId{};

				int m_firstEventId{};
				int m_time{};
				double m_heightPosition{};

				QMap<std::tuple<int,int,int>, int> m_createdConstraintViewModelIDs;
		};
	}
}
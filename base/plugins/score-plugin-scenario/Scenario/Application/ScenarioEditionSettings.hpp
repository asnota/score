#pragma once
#include <Process/ExpandMode.hpp>
#include <wobjectdefs.h>
#include <QObject>
#include <Scenario/Palette/Tool.hpp>
#include <score_plugin_scenario_export.h>
namespace Scenario
{
class SCORE_PLUGIN_SCENARIO_EXPORT EditionSettings final : public QObject
{
  W_OBJECT(EditionSettings)
  
  
  
  

  ExpandMode m_expandMode{ExpandMode::Scale};
  Scenario::Tool m_tool{Scenario::Tool::Select};
  Scenario::Tool m_previousTool{Scenario::Tool::Select};
  LockMode m_lockMode{};
  bool m_sequence{false};
  bool m_execution{false};

public:
  ExpandMode expandMode() const;
  Scenario::Tool tool() const;
  bool sequence() const;

  void setExpandMode(ExpandMode expandMode);
  void setTool(Scenario::Tool tool);
  void setSequence(bool sequence);
  void setExecution(bool ex);

  void setDefault();
  void restoreTool();

  LockMode lockMode() const;

public:
  void setLockMode(LockMode lockMode); W_SLOT(setLockMode);

public:
  void expandModeChanged(ExpandMode expandMode) W_SIGNAL(expandModeChanged, expandMode);
  void toolChanged(Scenario::Tool tool) W_SIGNAL(toolChanged, tool);
  void sequenceChanged(bool sequence) W_SIGNAL(sequenceChanged, sequence);

  void lockModeChanged(LockMode lockMode) W_SIGNAL(lockModeChanged, lockMode);

W_PROPERTY(bool, sequence READ sequence WRITE setSequence NOTIFY sequenceChanged)

W_PROPERTY(Scenario::Tool, tool READ tool WRITE setTool NOTIFY toolChanged)

W_PROPERTY(LockMode, lockMode READ lockMode WRITE setLockMode NOTIFY lockModeChanged)

W_PROPERTY(ExpandMode, expandMode READ expandMode WRITE setExpandMode NOTIFY expandModeChanged)
};
}

// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "score_plugin_automation.hpp"

#include <Automation/AutomationColors.hpp>
#include <Automation/AutomationModel.hpp>
#include <Automation/AutomationPresenter.hpp>
#include <Automation/AutomationProcessMetadata.hpp>
#include <Automation/AutomationView.hpp>
#include <Automation/Color/GradientAutomModel.hpp>
#include <Automation/Color/GradientAutomPresenter.hpp>
#include <Automation/Color/GradientAutomView.hpp>
#include <Automation/Commands/AutomationCommandFactory.hpp>
#include <Automation/Inspector/AutomationInspectorFactory.hpp>
#include <Automation/Inspector/AutomationStateInspectorFactory.hpp>
#include <Automation/Inspector/CurvePointInspectorFactory.hpp>
#include <Automation/Metronome/MetronomeColors.hpp>
#include <Automation/Metronome/MetronomeModel.hpp>
#include <Automation/Metronome/MetronomePresenter.hpp>
#include <Automation/Metronome/MetronomeView.hpp>
#include <Automation/Spline/SplineAutomModel.hpp>
#include <Automation/Spline/SplineAutomPresenter.hpp>
#include <Automation/Spline/SplineAutomView.hpp>
#include <Curve/Process/CurveProcessFactory.hpp>
#include <Inspector/InspectorWidgetFactoryInterface.hpp>
#include <Process/GenericProcessFactory.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/HeaderDelegate.hpp>
#include <Process/ProcessFactory.hpp>
#include <score/plugins/customfactory/FactorySetup.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#include <score/tools/std/HashMap.hpp>
#include <score_plugin_automation_commands_files.hpp>
#include <Process/Style/ScenarioStyle.hpp>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Automation::LayerPresenter)
W_OBJECT_IMPL(Metronome::LayerPresenter)
namespace Automation
{
template<typename Layer_T>
class MinMaxHeaderDelegate final :
        public Process::DefaultHeaderDelegate
{
  public:
    MinMaxHeaderDelegate(const Layer_T& layer)
      : Process::DefaultHeaderDelegate{layer}
    {
      const auto& model = layer.model();
      using model_t = std::remove_reference_t<decltype(model)>;

      con(model, &model_t::minChanged,
          this, [=] { updateName(); });
      con(model, &model_t::maxChanged,
          this, [=] { updateName(); });

      updateName();
    }

    void updateName() override
    {
      if (presenter)
      {
        const auto& style = ScenarioStyle::instance();
        using model_t = std::remove_reference_t<decltype(std::declval<Layer_T>().model())>;
        auto& model = static_cast<const model_t&>(presenter->model());

        QString txt = presenter->model().prettyName();
        txt += "  Min: ";
        txt += QString::number(model.min());
        txt += "  Max: ";
        txt += QString::number(model.max());
        m_line = Process::makeGlyphs(
            txt,
            m_sel ? style.IntervalHeaderTextPen : style.GrayTextPen);
        update();
      }
    }

};
using AutomationFactory = Process::ProcessFactory_T<Automation::ProcessModel>;
using AutomationLayerFactory = Curve::CurveLayerFactory_T<
    Automation::ProcessModel,
    Automation::LayerPresenter,
    Automation::LayerView,
    Automation::Colors,
    Automation::MinMaxHeaderDelegate<Automation::LayerPresenter>>;
}
namespace Gradient
{
using GradientFactory = Process::ProcessFactory_T<Gradient::ProcessModel>;
using GradientLayerFactory = Process::LayerFactory_T<
    Gradient::ProcessModel,
    Gradient::Presenter,
    Gradient::View>;
}

namespace Spline
{
using SplineFactory = Process::ProcessFactory_T<Spline::ProcessModel>;
using SplineLayerFactory = Process::
    LayerFactory_T<Spline::ProcessModel, Spline::Presenter, Spline::View>;
}

namespace Metronome
{
using MetronomeFactory = Process::ProcessFactory_T<Metronome::ProcessModel>;
using MetronomeLayerFactory = Curve::CurveLayerFactory_T<
    Metronome::ProcessModel,
    Metronome::LayerPresenter,
    Metronome::LayerView,
    Metronome::Colors,
    Automation::MinMaxHeaderDelegate<Metronome::LayerPresenter>>;
}

score_plugin_automation::score_plugin_automation() = default;
score_plugin_automation::~score_plugin_automation() = default;

std::vector<std::unique_ptr<score::InterfaceBase>>
score_plugin_automation::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext,
      FW<Process::ProcessModelFactory, Automation::AutomationFactory,
         Gradient::GradientFactory, Spline::SplineFactory,
         Metronome::MetronomeFactory>,
      FW<Process::LayerFactory, Automation::AutomationLayerFactory,
         Gradient::GradientLayerFactory, Spline::SplineLayerFactory,
         Metronome::MetronomeLayerFactory>,
      FW<Inspector::InspectorWidgetFactory, Automation::StateInspectorFactory,
         Automation::PointInspectorFactory, Automation::InspectorFactory,
         Gradient::InspectorFactory, Spline::InspectorFactory,
         Metronome::InspectorFactory>>(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap>
score_plugin_automation::make_commands()
{
  using namespace Automation;
  using namespace Gradient;
  using namespace Spline;
  using namespace Metronome;
  std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
      CommandFactoryName(), CommandGeneratorMap{}};

  using Types = TypeList<
#include <score_plugin_automation_commands.hpp>
      >;
  for_each_type<Types>(score::commands::FactoryInserter{cmds.second});

  return cmds;
}

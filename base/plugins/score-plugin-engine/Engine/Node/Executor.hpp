#pragma once
#include <ossia/dataflow/safe_nodes/executor.hpp>

#include <Engine/Executor/ProcessComponent.hpp>
#include <Engine/Node/Process.hpp>
#include <Engine/Node/TickPolicy.hpp>
#include <Engine/score2OSSIA.hpp>
#include <Explorer/DeviceList.hpp>
#include <Explorer/DocumentPlugin/DeviceDocumentPlugin.hpp>
#include <QTimer>

namespace Control
{

template <typename Info_T, typename Node_T, typename Element>
struct setup_Impl0
{
  Element& element;
  const Engine::Execution::Context& ctx;
  const std::shared_ptr<Node_T>& node_ptr;
  QObject* parent;

  template <typename Idx_T>
  struct con_validated
  {
    const Engine::Execution::Context& ctx;
    std::weak_ptr<Node_T> weak_node;
    void operator()(const ossia::value& val)
    {
      using namespace ossia::safe_nodes;
      constexpr auto idx = Idx_T::value;

      using control_type = typename std::tuple_element<
          idx, decltype(get_controls<Info_T>{}())>::type;
      using control_value_type = typename control_type::type;

      if (auto node = weak_node.lock())
      {
        constexpr const auto ctrl = std::get<idx>(get_controls<Info_T>{}());
        if (auto v = ctrl.fromValue(val))
          ctx.executionQueue.enqueue(control_updater<control_value_type>{
              std::get<idx>(node->controls), std::move(*v)});
      }
    }
  };

  template <typename Idx_T>
  struct con_unvalidated
  {
    const Engine::Execution::Context& ctx;
    std::weak_ptr<Node_T> weak_node;
    void operator()(const ossia::value& val)
    {
      using namespace ossia::safe_nodes;
      constexpr auto idx = Idx_T::value;

      using control_type = typename std::tuple_element<
          idx, decltype(get_controls<Info_T>{}())>::type;
      using control_value_type = typename control_type::type;

      if (auto node = weak_node.lock())
      {
        constexpr const auto ctrl = std::get<idx>(get_controls<Info_T>{}());
        ctx.executionQueue.enqueue(control_updater<control_value_type>{
            std::get<idx>(node->controls), ctrl.fromValue(val)});
      }
    }
  };

  template <typename T>
  void operator()(T)
  {
    using namespace ossia::safe_nodes;
    using Info = Info_T;
    constexpr int idx = T::value;

    constexpr const auto ctrl = std::get<idx>(get_controls<Info_T>{}());
    constexpr const auto control_start = info_functions<Info>::control_start;
    using control_type = typename std::tuple_element<
        idx, decltype(get_controls<Info_T>{}())>::type;
    auto inlet = static_cast<Process::ControlInlet*>(
        element.inlets()[control_start + idx]);

    auto& node = *node_ptr;
    std::weak_ptr<Node_T> weak_node = node_ptr;

    if constexpr (control_type::must_validate)
    {
      if (auto res = ctrl.fromValue(element.control(idx)))
        std::get<idx>(node.controls) = *res;

      QObject::connect(
          inlet, &Process::ControlInlet::valueChanged, parent,
          con_validated<T>{ctx, weak_node});
    }
    else
    {
      std::get<idx>(node.controls) = ctrl.fromValue(element.control(idx));

      QObject::connect(
          inlet, &Process::ControlInlet::valueChanged, parent,
          con_unvalidated<T>{ctx, weak_node});
    }
  }
};

template <typename Info, typename Element, typename Node_T>
struct setup_Impl1
{
  typename Node_T::controls_values_type& arr;
  Element& element;

  template <typename T>
  void operator()(T)
  {
    using namespace ossia::safe_nodes;
    constexpr const auto ctrl = std::get<T::value>(get_controls<Info>{}());

    element.setControl(T::value, ctrl.toValue(std::get<T::value>(arr)));
  }
};

template <typename Info, typename Node_T, typename Element_T>
void setup_node(
    const std::shared_ptr<Node_T>& node_ptr,
    Element_T& element,
    const Engine::Execution::Context& ctx,
    QObject* parent)
{
  using namespace ossia::safe_nodes;
  constexpr const auto control_count = info_functions<Info>::control_count;

  (void)parent;
  if constexpr (control_count > 0)
  {
    // Initialize all the controls in the node with the current value.
    //
    // And update the node when the UI changes
    ossia::for_each_in_range<control_count>(
        setup_Impl0<Info, Node_T, Element_T>{element, ctx, node_ptr, parent});

    // Update the value in the UI
    std::weak_ptr<Node_T> weak_node = node_ptr;
    con(ctx.doc.coarseUpdateTimer, &QTimer::timeout, parent,
        [weak_node, &element] {
          if (auto node = weak_node.lock())
          {
            // TODO disconnect the connection ? it will be disconnected shortly
            // after...
            typename Node_T::controls_values_type arr;
            bool ok = false;
            while (node->cqueue.try_dequeue(arr))
            {
              ok = true;
            }
            if (ok)
            {
              constexpr const auto control_count
                  = info_functions<Info>::control_count;

              ossia::for_each_in_range<control_count>(
                  setup_Impl1<Info, Element_T, Node_T>{arr, element});
            }
          }
        },
        Qt::QueuedConnection);
  }
}

template <typename Info>
class Executor final
    : public Engine::Execution::
          ProcessComponent_T<ControlProcess<Info>, ossia::node_process>
{
public:
  static Q_DECL_RELAXED_CONSTEXPR score::Component::Key static_key() noexcept
  {
    return Info::Metadata::uuid;
  }

  score::Component::Key key() const noexcept final override
  {
    return static_key();
  }

  bool key_match(score::Component::Key other) const noexcept final override
  {
    return static_key() == other
           || Engine::Execution::ProcessComponent::base_key_match(other);
  }

  Executor(
      ControlProcess<Info>& element,
      const ::Engine::Execution::Context& ctx,
      const Id<score::Component>& id,
      QObject* parent)
      : Engine::Execution::
            ProcessComponent_T<ControlProcess<Info>, ossia::node_process>{
                element, ctx, id, "Executor::ControlProcess<Info>", parent}
  {
    auto node = std::make_shared<ossia::safe_nodes::safe_node<Info>>();
    this->node = node;
    this->m_ossia_process = std::make_shared<ossia::node_process>(this->node);

    setup_node<Info>(node, element, ctx, this);
  }

  ~Executor()
  {
  }
};
}

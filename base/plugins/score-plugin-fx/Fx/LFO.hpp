#pragma once
#include <ossia/dataflow/execution_state.hpp>

#include <Engine/Node/PdNode.hpp>
#include <ossia/detail/math.hpp>
#include <algorithm>
#include <array>
#include <bitset>
#include <map>
#include <random>
#include <tuple>
#include <utility>
namespace Nodes
{

namespace LFO
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "LFO";
    static const constexpr auto objectKey = "LFO";
    static const constexpr auto category = "Control";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const constexpr auto uuid
        = make_uuid("0697b807-f588-49b5-926c-f97701edd0d8");

    static const constexpr auto value_outs
        = ossia::safe_nodes::value_outs<1>{value_out{"out"}};

    static const constexpr auto controls = std::make_tuple(
        Control::Widgets::LFOFreqChooser(),
        Control::FloatSlider{"Coarse intens.", 0., 1000., 0.},
        Control::FloatSlider{"Fine intens.", 0., 1., 1.},
        Control::FloatSlider{"Offset.", -1000., 1000., 0.},
        Control::FloatSlider{"Jitter", 0., 1., 0.},
        Control::FloatSlider{"Phase", -1., 1., 0.},
        Control::Widgets::WaveformChooser());
  };

  // Idea: save internal state for rewind... ? -> require Copyable
  struct State
  {
    int64_t phase{};
    std::mt19937 rd;
  };

  using control_policy = ossia::safe_nodes::precise_tick;

  static void
  run(float freq,
      float coarse,
      float fine,
      float offset,
      float jitter,
      float phase,
      const std::string& type,
      ossia::value_port& out,
      ossia::token_request tk,
      ossia::execution_state& st,
      State& s)
  {
    auto& waveform_map = Control::Widgets::waveformMap();

    if (auto it = waveform_map.find(type); it != waveform_map.end())
    {
      float new_val{};
      auto ph = s.phase;
      if (jitter > 0)
      {
        ph += std::normal_distribution<float>(0., 5000.)(s.rd) * jitter;
      }

      using namespace Control::Widgets;
      const auto phi = phase + (float(ossia::two_pi) * freq * ph) / st.sampleRate;

      switch (it->second)
      {
        case Sin:
          new_val = (coarse + fine) * std::sin(phi);
          out.add_raw_value(new_val + offset);
          break;
        case Triangle:
          new_val = (coarse + fine) * std::asin(std::sin(phi));
          out.add_raw_value(new_val + offset);
          break;
        case Saw:
          new_val = (coarse + fine) * std::atan(std::tan(phi));
          out.add_raw_value(new_val + offset);
          break;
        case Square:
          new_val = (coarse + fine) * ((std::sin(phi) > 0.f) ? 1.f : -1.f);
          out.add_raw_value(new_val + offset);
          break;
        case SampleAndHold:
        {
          auto start_phi = phase + (float(ossia::two_pi) * freq * s.phase) / st.sampleRate;
          auto end_phi = phase + (float(ossia::two_pi) * freq * (s.phase + tk.date - tk.prev_date)) / st.sampleRate;
          auto start_s = std::sin(start_phi);
          auto end_s = std::sin(end_phi);
          if((start_s > 0 && end_s <= 0) || (start_s <= 0 && end_s > 0))
          {
            new_val = std::uniform_real_distribution<float>(
                  -(coarse + fine), coarse + fine)(s.rd);
            out.add_raw_value(new_val + offset);
          }
          break;
        }
        case Noise1:
          new_val = std::uniform_real_distribution<float>(
              -(coarse + fine), coarse + fine)(s.rd);
          out.add_raw_value(new_val + offset);
          break;
        case Noise2:
          new_val = std::normal_distribution<float>(0, coarse + fine)(s.rd);
          out.add_raw_value(new_val + offset);
          break;
        case Noise3:
          new_val = std::cauchy_distribution<float>(0, coarse + fine)(s.rd);
          out.add_raw_value(new_val + offset);
          break;
      }
    }

    s.phase += (tk.date - tk.prev_date);
  }
};
}
}

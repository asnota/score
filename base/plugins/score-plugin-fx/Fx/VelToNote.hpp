#pragma once
#include <ossia/detail/math.hpp>

#include <Engine/Node/PdNode.hpp>
#include <Fx/Quantifier.hpp>
#include <random>
namespace Nodes
{
namespace PulseToNote
{
struct Node
{
  struct Metadata : Control::Meta_base
  {
    static const constexpr auto prettyName = "Pulse to Note";
    static const constexpr auto objectKey = "VelToNote";
    static const constexpr auto category = "Midi";
    static const constexpr auto tags = std::array<const char*, 0>{};
    static const constexpr auto uuid
        = make_uuid("2c6493c3-5449-4e52-ae04-9aee3be5fb6a");

    static const constexpr auto value_ins
        = ossia::safe_nodes::value_ins<1>{value_in{"in", true}};
    static const constexpr auto midi_outs
        = ossia::safe_nodes::midi_outs<1>{{"out"}};
    static const constexpr auto controls = std::make_tuple(
        Control::Widgets::QuantificationChooser(),
        Control::FloatSlider{"Tightness", 0.f, 1.f, 0.8f},
        Control::Widgets::DurationChooser(),
        Control::Widgets::MidiSpinbox("Default pitch"),
        Control::Widgets::MidiSpinbox("Default vel."),
        Control::Widgets::OctaveSlider("Pitch shift", -5, 5),
        Control::Widgets::OctaveSlider("Pitch random", 0, 2),
        Control::Widgets::OctaveSlider("Vel. random", 0, 2),
        Control::Widgets::MidiChannel("Channel"),
        Control::Widgets::TempoChooser());
  };

  using State = Quantifier::Node::State;
  using Note = Control::Note;

  using control_policy = ossia::safe_nodes::default_tick;
  struct val_visitor
  {
    State& st;
    uint8_t base_note{};
    uint8_t base_vel{};

    Note operator()()
    {
      return {base_note, base_vel};
    }
    Note operator()(ossia::impulse)
    {
      return {base_note, base_vel};
    }
    template <typename T>
    Note operator()(const T&)
    {
      return {base_note, base_vel};
    }
    Note operator()(float note)
    {
      return {(uint8_t)note, base_vel};
    }
    Note operator()(char note)
    {
      return {(uint8_t)note, base_vel};
    }
    Note operator()(int note)
    {
      return {(uint8_t)note, base_vel};
    }
    Note operator()(int note, int vel)
    {
      return {(uint8_t)note, (uint8_t)vel};
    }
    Note operator()(int note, float vel)
    {
      return {(uint8_t)note, (uint8_t)(vel * 127.f)};
    }
    Note operator()(const std::vector<ossia::value>& v)
    {
      switch (v.size())
      {
        case 0:
          return operator()();
        case 1:
          return operator()(ossia::convert<int>(v[0]));
        case 2:
        {
          int note = ossia::convert<int>(v[0]);
          switch (v[1].getType())
          {
            case ossia::val_type::FLOAT:
              return operator()(note, *v[1].v.target<float>());
            case ossia::val_type::INT:
              return operator()(note, *v[1].v.target<int>());
            default:
              return operator()(note, ossia::convert<int>(v[1]));
          }
        }
        default:
          return operator()(
              ossia::convert<int>(v[0]), ossia::convert<int>(v[1]));
      }
    }
    template <std::size_t N>
    Note operator()(const std::array<float, N>& v)
    {
      static_assert(N >= 2);
      return operator()(v[0], v[1]);
    }
  };

  static constexpr uint8_t midi_clamp(int num)
  {
    return (uint8_t)ossia::clamp(num, 0, 127);
  }
  static void
  run(const ossia::value_port& p1,
      const ossia::safe_nodes::timed_vec<float>& startq,
      const ossia::safe_nodes::timed_vec<float>& tightness,
      const ossia::safe_nodes::timed_vec<float>& dur,
      const ossia::safe_nodes::timed_vec<int>& basenote,
      const ossia::safe_nodes::timed_vec<int>& basevel,
      const ossia::safe_nodes::timed_vec<int>& shift_note,
      const ossia::safe_nodes::timed_vec<int>& note_random,
      const ossia::safe_nodes::timed_vec<int>& vel_random,
      const ossia::safe_nodes::timed_vec<int>& chan_vec,
      const ossia::safe_nodes::timed_vec<float>& tempo_vec,
      ossia::midi_port& p2,
      ossia::token_request tk,
      ossia::execution_state& st,
      State& self)
  {
    static std::mt19937 m;
    // TODO : when arrays like [ 1, 25, 12, 37, 10, 40 ] are received
    // send relevant chords

    // When a message is received, we have three cases :
    // 1. Just an impulse: use base note & base vel
    // 2. Just an int: it's the velocity, use base note
    // 3. A tuple [int, int]: it's note and velocity

    // Then once we have a pair [int, int] we randomize and we output a note
    // on.

    // At the end, scan running_notes: if any is going to end in this buffer,
    // end it too.

    auto start = startq.rbegin()->second;
    double precision = tightness.rbegin()->second;
    auto duration = dur.rbegin()->second;
    auto shiftnote = shift_note.rbegin()->second;
    auto base_note = midi_clamp(basenote.rbegin()->second);
    auto base_vel = midi_clamp(basevel.rbegin()->second);
    auto rand_note = note_random.rbegin()->second;
    auto rand_vel = vel_random.rbegin()->second;
    auto chan = chan_vec.rbegin()->second;
    auto tempo = tempo_vec.rbegin()->second;

    // how much time does a whole note last at this tempo given the current sr
    const auto whole_dur = 240.f / tempo; // in seconds
    const auto whole_samples = whole_dur * st.sampleRate;

    for (auto& in : p1.get_data())
    {
      auto note = in.value.apply(val_visitor{self, base_note, base_vel});

      if (rand_note != 0)
        note.pitch
            += std::uniform_int_distribution<int>(-rand_note, rand_note)(m);
      if (rand_vel != 0)
        note.vel += std::uniform_int_distribution<int>(-rand_vel, rand_vel)(m);

      note.pitch = ossia::clamp((int)note.pitch + shiftnote, 0, 127);
      note.vel = ossia::clamp((int)note.vel, 0, 127);

      if (note.vel != 0)
      {
        if (start == 0.f) // No quantification, start directly
        {
          auto no = rtmidi::message::note_on(chan, note.pitch, note.vel);
          no.timestamp = in.timestamp;

          p2.messages.push_back(no);
          if (duration > 0.f)
          {
            auto end = tk.date + (int64_t)no.timestamp
                       + (int64_t)(whole_samples * duration);
            self.running_notes.push_back({note, end});
          }
          else if (duration == 0.f)
          {
            // Stop at the next sample
            auto noff = rtmidi::message::note_off(chan, note.pitch, note.vel);
            noff.timestamp = no.timestamp;
            p2.messages.push_back(noff);
          }
          // else do nothing and just wait for a note off
        }
        else
        {
          // Find next time that matches the requested quantification
          const auto start_q = whole_samples * start;
          auto perf_date = int64_t(start_q * int64_t(1 + tk.date / start_q));
          int64_t actual_date = (1. - precision) * tk.date + precision * perf_date;
          ossia::time_value next_date{actual_date};
          self.to_start.push_back({note, next_date});
        }
      }
      else
      {
        // Just stop
        auto noff = rtmidi::message::note_off(chan, note.pitch, note.vel);
        noff.timestamp = in.timestamp;
        p2.messages.push_back(noff);
      }
    }

    for (auto it = self.to_start.begin(); it != self.to_start.end();)
    {
      auto& note = *it;
      if (note.date > tk.prev_date && note.date.impl < tk.date.impl)
      {
        auto no = rtmidi::message::note_on(chan, note.note.pitch, note.note.vel);
        no.timestamp = note.date - tk.prev_date;
        p2.messages.push_back(no);

        if (duration > 0.f)
        {
          auto end = note.date + (int64_t)(whole_samples * duration);
          self.running_notes.push_back({note.note, end});
        }
        else if (duration == 0.f)
        {
          // Stop at the next sample
          auto noff = rtmidi::message::note_off(chan, note.note.pitch, note.note.vel);
          noff.timestamp = no.timestamp;
          p2.messages.push_back(noff);
        }

        it = self.to_start.erase(it);
      }
      else
      {
        ++it;
      }
    }

    for (auto it = self.running_notes.begin(); it != self.running_notes.end();)
    {
      auto& note = *it;
      if (note.date > tk.prev_date && note.date.impl < tk.date.impl)
      {
        auto noff = rtmidi::message::note_off(chan, note.note.pitch, note.note.vel);
        noff.timestamp = note.date - tk.prev_date;
        p2.messages.push_back(noff);
        it = self.running_notes.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
};
}
}

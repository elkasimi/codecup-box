#pragma once

#include "AI.h"
#include "Position.h"
#include "RNG.h"
#include "TimeManagement.h"

namespace mcts_ai {

constexpr bool USE_DOT_COLOR_STATS{true};

struct DotColorStats {
  static constexpr int MAX{TOTAL_DOTS * MAX_COLORS};
  array<Stats, MAX> stats;

  static int code(int dot, Color color) {
    return dot + TOTAL_DOTS * (color - '1');
  }

  void update(int dot, Color color, Player player, double value) {
    auto v = player == PLAYER_1 ? value : -value;
    stats[code(dot, color)].update(v);
  }

  double evaluate(const Position& pos, const TileInfo* tile_info) const {
    double sum{0.0};

    for (int i{0}; const auto& [d1, d2] : tile_info->siblings) {
      auto color = pos.tile[i++];
      for (int dot : {d1, d2}) {
        sum += stats[code(dot, color)].value;
      }
    }
    double eval = sum / 12.0;
    return pos.player == PLAYER_1 ? eval : -eval;
  }

  void reset() {
    for (auto& s : stats) {
      s.value = 0.0;
      s.visits = 0;
    }
  }
};

DotColorStats dot_color_stats;

struct ActionInfo {
  explicit ActionInfo(const TileInfo* info) : tile_info(info) {}

  static constexpr double K0{10.0};
  static constexpr double K0xK0{K0 * K0};

  const TileInfo* tile_info;
  double value{0.0};
  double value_squares{0.0};
  double K{K0};
  double bias{0.0};
  int visits{0};

  void update(double v) {
    visits += 1;
    auto delta = v - value;
    value += delta / visits;
    value_squares += delta * (v - value);
    K = sqrt((value_squares + K0xK0) / visits);
  }
};

const vector<double> BONUS = []() {
  vector<double> res;
  for (int v = 0; v < 200'000; ++v) {
    res.push_back(sqrt(log(1 + v)));
  }
  return res;
}();

const vector<double> SQRT = []() {
  vector<double> res;
  for (int v = 0; v < 200'000; ++v) {
    res.push_back(sqrt(v));
  }
  return res;
}();

struct StateInfo {
  TileSet unexpanded_tiles;
  vector<ActionInfo> actions;
  double bonus{0.0};
  int visits{0};
  Player player;

  explicit StateInfo(const Position& pos)
      : unexpanded_tiles(pos.get_possible_tiles_set()), player(pos.player) {}

  double eval(const Position& pos, const ActionInfo* action_info) const {
    auto e = action_info->value +
             action_info->K * bonus / SQRT[1 + action_info->visits];
    if constexpr (USE_DOT_COLOR_STATS) {
      e += action_info->bias / (1 + action_info->visits);
    }
    return e;
  }

  ActionInfo* select_most_visited() {
    ActionInfo* most_visited{nullptr};
    int max_visits{numeric_limits<int>::lowest()};
    for (auto& action_info : actions) {
      if (max_visits < action_info.visits) {
        max_visits = action_info.visits;
        most_visited = &action_info;
      }
    }
    return most_visited;
  }

  ActionInfo* select(const Position& pos) {
    auto expanded_limit = static_cast<size_t>(SQRT[visits + 1]);
    if (expanded_limit > 64) expanded_limit = 64;
    while (actions.size() < expanded_limit && unexpanded_tiles.any()) {
      const TileInfo* selected{nullptr};
      auto best_value = numeric_limits<double>::lowest();
      unexpanded_tiles.for_each([&pos, &best_value, &selected](auto tile_info) {
        auto value = dot_color_stats.evaluate(pos, tile_info);
        if (best_value < value) {
          best_value = value;
          selected = tile_info;
        }
      });
      actions.emplace_back(selected);
      actions.back().bias = best_value;
      unexpanded_tiles.clear(selected->code);
    }

    ActionInfo* best_action{nullptr};
    double best_value{numeric_limits<double>::lowest()};
    for (auto& action_info : actions) {
      if (auto value = eval(pos, &action_info); best_value < value) {
        best_value = value;
        best_action = &action_info;
      }
    }
    return best_action;
  }

  void update(ActionInfo* action_info, double score) {
    ++visits;
    action_info->update(score);
    bonus = BONUS[visits];
  }

  bool consistent(const Position& pos) {
    return select_most_visited() == select(pos);
  }
};

struct StateStore {
  struct PositionInfoHash {
    size_t operator()(const Position::Info& info) const { return info.hash; }
  };

  struct PositionInfoEqual {
    bool operator()(const Position::Info& lhs,
                    const Position::Info& rhs) const {
      return lhs.tile_index == rhs.tile_index && lhs.player == rhs.player &&
             lhs.columns == rhs.columns;
    }
  };

  HashMap<Position::Info, StateInfo, PositionInfoHash, PositionInfoEqual> Q;

  pair<StateInfo*, bool> try_create_state(const Position& pos) {
    auto [it, inserted] = Q.try_emplace(pos.get_info(), pos);
    return {&it->second, inserted};
  }

  StateInfo* get(const Position& pos) {
    auto it = Q.find(pos.get_info());
    return it != Q.end() ? &it->second : nullptr;
  }

  void prepare_for(auto size) { Q.reserve(size); }

  void print_stats(ostream& out) const {
    map<size_t, size_t> m;
    size_t total{0};
    for (size_t i{0}; i < Q.bucket_count(); ++i) {
      auto size = Q.bucket_size(i);
      m[size]++;
      total++;
    }
    out << "{";
    for (const auto& [k, v] : m) {
      out << k << ":" << static_cast<double>(v) / static_cast<double>(total)
          << " ";
    }
    out << "total:" << total << "}" << endl;
    const ActionInfo* lowest_variance_action{nullptr};
    const ActionInfo* highest_variance_action{nullptr};
    for (const auto& [k, v] : Q) {
      for (const auto& action : v.actions) {
        if (!lowest_variance_action || lowest_variance_action->K > action.K) {
          lowest_variance_action = &action;
        }
        if (!highest_variance_action || highest_variance_action->K < action.K) {
          highest_variance_action = &action;
        }
      }
    }
    if (lowest_variance_action) {
      out << "lowest=(" << lowest_variance_action->K << ", "
          << lowest_variance_action->visits << ")" << endl;
    }
    if (highest_variance_action) {
      out << "highest=(" << highest_variance_action->K << ", "
          << highest_variance_action->visits << ")" << endl;
    }
  }
};

struct Warmup {
  Position pos;
  Player player;
  Color color;

  explicit Warmup(const Position& p, Color c)
      : pos(p), player(p.player), color(c) {}

  void run() {
    while (auto tile_info = pos.get_random_move()) {
      pos.do_move(tile_info);
      pos.play_chance_move();
    }
    auto score = pos.get_expected_score(color);
    for (int dot : ALL_DOTS) {
      if (auto dot_color = pos.colors[dot]; dot_color != Position::WHITE) {
        dot_color_stats.update(dot, dot_color, player, score);
      }
    }
  }
};

struct Simulation {
  StateStore& state_store;
  Position pos;
  Player player;
  vector<tuple<StateInfo*, ActionInfo*>> transitions{};
  Color color;

  static inline size_t max_level{0};

  static void reset_stats() {
    max_level = 0;
    if constexpr (USE_DOT_COLOR_STATS) {
      dot_color_stats.reset();
    }
  }

  Simulation(StateStore& state_store, const Position& p, Color color)
      : state_store(state_store), pos(p), player(p.player), color(color) {}

  void add(StateInfo* state_info, ActionInfo* action_info) {
    transitions.emplace_back(state_info, action_info);
  }

  void next(StateInfo* state_info) {
    auto action_info = state_info->select(pos);
    pos.do_move(action_info->tile_info);
    pos.play_chance_move();
    add(state_info, action_info);
  }

  void simulate_tree() {
    while (!pos.end_game()) {
      auto [state_info, created] = state_store.try_create_state(pos);
      next(state_info);
      if (created) {
        break;
      }
    }
  }

  void simulate_default() {
    while (auto tile_info = pos.get_random_move()) {
      pos.do_move(tile_info);
      pos.play_chance_move();
    }
  }

  void backup() const {
    auto score = pos.get_expected_score(color);
    for (const auto& [state_info, action_info] : transitions) {
      auto adjusted_score = state_info->player == player ? score : -score;
      state_info->update(action_info, adjusted_score);
    }
    if constexpr (USE_DOT_COLOR_STATS) {
      for (int dot : ALL_DOTS) {
        if (auto dot_color = pos.colors[dot]; dot_color != Position::WHITE) {
          dot_color_stats.update(dot, dot_color, player, score);
        }
      }
    }
  }

  void run() {
    simulate_tree();
    max_level = max(max_level, transitions.size());
    simulate_default();
    backup();
  }
};

double get_max_time(const Position& pos, const AiContext& ctx) {
#ifdef BOX_SUBMISSION
  constexpr double ratio{1.0};
#else
  constexpr double ratio{0.22};
#endif

  constexpr double MAX_TOTAL_TIME{30.0 * ratio};
  constexpr double TIME_MARGIN{0.5 * ratio};
  int r = max((31 - pos.turn) / 2, 2);
  double remaining_time = MAX_TOTAL_TIME - TIME_MARGIN - ctx.total_time;
  return remaining_time / static_cast<double>(r);
}

PlayerMove get_best_move(const Position& pos, AiContext& ctx) {
  Simulation::reset_stats();
  constexpr int MAX_ITERATIONS{100'000};

  StateStore state_store;
  state_store.prepare_for(MAX_ITERATIONS);
  auto color = ctx.color;
  auto& log = ctx.log;
  log << fixed << setprecision(2);
  auto start = get_time_point();
  const auto max_time = get_max_time(pos, ctx);
  log << "max-time=" << max_time << endl;
  for (int w = 0; w < 1000; ++w) {
    Warmup(pos, color).run();
  }
  auto wt = get_delta_time_since(start);
  log << "warmup took " << wt << " sec" << endl;
  int s = 0;
  pos.update_condidates();
  for (; s < MAX_ITERATIONS && get_delta_time_since(start) < max_time; ++s) {
    Simulation(state_store, pos, color).run();

    auto most_visited = state_store.get(pos)->select_most_visited();
    if (2 * most_visited->visits > MAX_ITERATIONS) {
      break;
    }
  }
  int extras{0};
  auto root = state_store.get(pos);
  for (; extras < 10'000 && get_delta_time_since(start) < max_time &&
         !root->consistent(pos);
       Simulation(state_store, pos, color).run(), ++s, ++extras) {
  }

  log << "extra=" << extras << endl;
  log << "c=" << pos.get_possible_tiles().size()
      << " ps=" << pos.get_expected_score(color) << " t=" << pos.turn << endl;

  auto most_visited = root->select_most_visited();
  log << "l=" << Simulation::max_level << " s=" << s
      << " v=" << most_visited->value << " n=" << most_visited->visits
      << " p=" << 100.0 * most_visited->visits / root->visits << "%" << endl;
  if constexpr (USE_DOT_COLOR_STATS) {
    log << "b=" << most_visited->bias << endl;
  }
  log << "expanded-count=" << root->actions.size() << endl;

  log << "k=" << most_visited->K << endl;
  auto dt = get_delta_time_since(start);
  ctx.total_time += dt;
  log << "impact = ";
  for (int i : pos.impact(most_visited->tile_info)) {
    log << i << " ";
  }
  log << endl;
  auto best_move = most_visited->tile_info->move();
  log << "best-move=" << best_move.show() << endl;
  state_store.Q.clear();
  log << string(12, '-') << endl;
  double speed = 0.001 * static_cast<double>(s) / dt;
  log << "dt=" << dt << " tt=" << ctx.total_time << " s=" << speed << " Ki/s"
      << endl;

  return best_move;
}
}  // namespace mcts_ai

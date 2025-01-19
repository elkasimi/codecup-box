#include "MctsAi.h"

int benchmark() {
  auto start = get_time_point();
  auto score = 0.0;
  Position::init_weigths('1');
  Position pos{"Hh123456h"};
  pos.update_condidates();
  for (int i = 0; i < 1'000'000; ++i) {
    auto p = pos;
    while (auto tile_info = p.get_random_move()) {
      p.play_chance_move();
      p.do_move(tile_info);
    }
    score += p.get_expected_score('1');
  }
  auto dt = get_delta_time_since(start);
  cout << "dt=" << dt << " speed=" << 1'000 / dt << " Ki/s" << endl;
  cout << score / 1'000'000 << endl;
  return 0;
}

int main() {
  cerr << "R player" << endl;
  cerr << "sizeof(Position)=" << sizeof(Position) << endl;
  cerr << "sizeof(Position::Info)=" << sizeof(Position::Info) << endl;
  cerr << "sizeof(StateInfo)=" << sizeof(mcts_ai::StateInfo) << endl;
  cerr << "sizeof(ActionInfo)=" << sizeof(mcts_ai::ActionInfo) << endl;
  cerr << "sizeof(DotColorStats)=" << sizeof(mcts_ai::DotColorStats) << endl;
  Color my_color;
  cin >> my_color;
  cerr << "my-color=" << my_color << endl;
  AiContext ctx{my_color, cerr};
  array<double, MAX_COLORS> total_delta_evals{{0, 0, 0, 0, 0, 0}};
  Position::init_weigths(my_color);
  string s;
  cin >> s;
  cerr << "starting-tile=" << s << endl;
  Position pos{s};
  while (cin >> s) {
    if (s == "Quit") {
      break;
    }
    if (s != "Start") {
      cerr << s << endl;
      const auto [chance_move, opponent_move] = parse_moves(s);
      pos.do_move(chance_move);
      auto delta_evals = pos.get_delta_evals(opponent_move);
      // array_log("last-delta-evals", delta_evals);
      for (int i = 0; i < MAX_COLORS; ++i) {
        total_delta_evals[i] += delta_evals[i];
      }
      // array_log("total-delta-evals", total_delta_evals);
      Position::update_weigths(total_delta_evals, my_color);
      pos.do_move(opponent_move);
    }

    ChanceMove chance_move;
    cin >> chance_move;
    cerr << chance_move << endl;
    pos.do_move(chance_move);

    const auto my_move = mcts_ai::get_best_move(pos, ctx);
    // auto my_delta_evals = pos.get_delta_evals(my_move);
    // array_log("my-delta-evals", my_delta_evals);
    pos.do_move(my_move);
    cout << my_move.show() << endl;
  }

  return 0;
}

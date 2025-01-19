#include "Position.h"

#include "RNG.h"

namespace {
const array<array<uint64_t, MAX_COLORS>, TOTAL_DOTS> zobrist_colors = []() {
  array<array<uint64_t, MAX_COLORS>, TOTAL_DOTS> res;
  for (int dot : ALL_DOTS) {
    for (int color : ALL_COLORS) {
      res[dot][color] = gen.random<uint64_t>();
    }
  }
  return res;
}();

const array<uint64_t, TILES_PERMUTATIONS_COUNT> zobrist_tiles = []() {
  array<uint64_t, TILES_PERMUTATIONS_COUNT> res;
  for (int p : iota_view(0, TILES_PERMUTATIONS_COUNT)) {
    res[p] = gen.random<uint64_t>();
  }
  return res;
}();

const uint64_t zobrist_player_1{gen.random<uint64_t>()};
const uint64_t zobrist_player_2{gen.random<uint64_t>()};
}  // namespace

Position::Position(const string& s) {
  colors.fill(WHITE);
  const auto& [chance_move, _] = parse_moves(s);
  do_move(chance_move);
  for (int i = 0; i < TILE_DOTS; ++i) {
    process_siblings(CENTER_TILE_INFO, i);
  }
}

void Position::update_tile_index(int index) {
  if (tile_index == index) {
    return;
  }

  if (tile_index != -1) {
    zobrist_hash ^= zobrist_tiles[tile_index];
  }

  tile = TILES_PERMUTATIONS[index].c_str();
  zobrist_hash ^= zobrist_tiles[index];
  tile_index = index;
}

void Position::play_chance_move() {
  auto index = gen.less_than(TILES_PERMUTATIONS_COUNT);
  update_tile_index(index);
}

bool Position::empty(int dot) const { return !filled.test(dot); }

bool Position::possible_move(const TileInfo* tile_info) const {
  if (auto overlap_count = tile_info->count_matches(filled)) {
    return (overlap_count <= MAX_OVERLAPS);
  } else {
    return tile_info->neighbour_to(filled);
  }
}

bool Position::possible_move(int dot, Orientation orientation) const {
  auto tile_info = orientation == VERTICAL ? &VERTICAL_TILES_INFO[dot]
                                           : &HORIZONTAL_TILES_INFO[dot];
  return possible_move(tile_info);
}

vector<const TileInfo*> Position::get_possible_tiles() const {
  vector<const TileInfo*> possible_tiles;
  possible_tiles.reserve(candidates.size());

  auto len = candidates.size();
  auto i = 0u;
  while (i < len) {
    auto tile_info = candidates[i];
    if (auto c = tile_info->count_matches(filled)) {
      if (c <= MAX_OVERLAPS) {
        possible_tiles.push_back(tile_info);
        ++i;
      } else {
        --len;
        candidates[i] = candidates[len];
      }
    } else if (tile_info->neighbour_to(filled)) {
      possible_tiles.push_back(tile_info);
      ++i;
    } else {
      ++i;
    }
  }

  candidates.resize(len);

  return possible_tiles;
}

TileSet Position::get_possible_tiles_set() const {
  TileSet res;

  auto len = candidates.size();
  auto i = 0u;
  while (i < len) {
    auto tile_info = candidates[i];
    if (auto c = tile_info->count_matches(filled)) {
      if (c <= MAX_OVERLAPS) {
        res.set(tile_info->code);
        ++i;
      } else {
        --len;
        candidates[i] = candidates[len];
      }
    } else if (tile_info->neighbour_to(filled)) {
      res.set(tile_info->code);
      ++i;
    } else {
      ++i;
    }
  }

  candidates.resize(len);

  return res;
}

bool Position::end_game() const {
  return ranges::none_of(
      candidates, [this](auto tile_info) { return possible_move(tile_info); });
}

int Position::bonus(int row, int col, int color) const {
  int score{0};
  for (auto v = columns[color][col].value; v > 0;) {
    // Find the least significant set bit
    int r = countr_zero(v);
    // Clear the least significant set bit
    v &= v - 1;

    if (row != r) {
      auto b = abs(row - r);
      if (col + b < COLS && columns[color][col + b].test(row, r)) {
        score += b;
      }
      if (col >= b && columns[color][col - b].test(row, r)) {
        score += b;
      }
    }
  }
  return score;
}

double Position::eval(int row, int col, int color) const {
  constexpr double P{0.142857};
  double e{0.0};
  for (int r = 0; r < ROWS; ++r) {
    if (r == row) continue;
    auto b = abs(row - r);
    if (col + b < COLS) {
      double m{1.0};
      if (!columns[color][col].test(r)) m *= P;
      if (!columns[color][col + b].test(row)) m *= P;
      if (!columns[color][col + b].test(r)) m *= P;
      e += (1.0 - P) * m * b;
    }
    if (col >= b) {
      double m{1.0};
      if (!columns[color][col].test(r)) m *= P;
      if (!columns[color][col - b].test(row)) m *= P;
      if (!columns[color][col - b].test(r)) m *= P;
      e += (1.0 - P) * m * b;
    }
  }
  return e;
}

array<double, MAX_COLORS> Position::get_delta_evals(
    const TileInfo* info) const {
  array<double, MAX_COLORS> evals;
  evals.fill(0.0);
  for (int i = 0; i < MAX_COLORS; ++i) {
    auto [d1, d2] = info->siblings[i];
    auto color = tile[i];
    for (int dot : {d1, d2}) {
      if (auto old_color = colors[dot]; old_color != color) {
        auto row = dot / COLS;
        auto col = dot % COLS;
        if (old_color != WHITE) {
          int old_color_index = old_color - '1';
          evals[old_color_index] -= eval(row, col, old_color_index);
        }
        int color_index = color - '1';
        evals[color_index] += eval(row, col, color_index);
      }
    }
  }
  return evals;
}

double Position::eval(const TileInfo* info, Color c) const {
  auto evals = get_delta_evals(info);
  auto index = c - '1';
  auto my_eval = evals[index];
  auto max_others_eval = numeric_limits<double>::lowest();
  for (int i = 0; i < MAX_COLORS; ++i) {
    if (i != index) {
      max_others_eval = max(max_others_eval, evals[i]);
    }
  }
  return my_eval - max_others_eval;
}

array<int, MAX_COLORS> Position::impact(const TileInfo* info) const {
  array<int, MAX_COLORS> res;
  res.fill(0);
  for (int i = 0; i < MAX_COLORS; ++i) {
    auto [d1, d2] = info->siblings[i];
    auto color = tile[i];
    for (int dot : {d1, d2}) {
      if (auto old_color = colors[dot]; old_color != color) {
        auto row = dot / COLS;
        auto col = dot % COLS;
        if (old_color != WHITE) {
          int old_color_index = old_color - '1';
          res[old_color_index] -= bonus(row, col, old_color_index);
        }
        int color_index = color - '1';
        res[color_index] += bonus(row, col, color_index);
      }
    }
  }
  return res;
}

void Position::update_color(int dot, Color color) {
  if (auto old_color = colors[dot]; old_color != color) {
    filled.set(dot);
    auto row = dot / COLS;
    auto col = dot % COLS;
    if (old_color != WHITE) {
      int old_color_index = old_color - '1';
      columns[old_color_index][col].unset(row);
      zobrist_hash ^= zobrist_colors[dot][old_color_index];
    }
    int color_index = color - '1';
    columns[color_index][col].set(row);
    colors[dot] = color;
    zobrist_hash ^= zobrist_colors[dot][color_index];
  }
}

void Position::process_siblings(const TileInfo* tile_info, int index) {
  const auto& [d1, d2] = tile_info->siblings[index];
  auto color = tile[index];
  update_color(d1, color);
  update_color(d2, color);
}

void Position::increment_turn() {
  ++turn;
  if (player == PLAYER_1) {
    player = PLAYER_2;
  } else if (player == PLAYER_2) {
    player = PLAYER_1;
  } else {
    assert(false);
  }
}

void Position::do_move(const TileInfo* tile_info) {
  for (int i = 0; i < TILE_DOTS; ++i) {
    process_siblings(tile_info, i);
  }
  increment_turn();
}

array<int, MAX_COLORS> Position::impact(const PlayerMove& move) const {
  const auto tile_info = move.orientation == VERTICAL
                             ? &VERTICAL_TILES_INFO[move.dot]
                             : &HORIZONTAL_TILES_INFO[move.dot];

  return impact(tile_info);
}

array<double, MAX_COLORS> Position::get_delta_evals(
    const PlayerMove& move) const {
  const auto tile_info = move.orientation == VERTICAL
                             ? &VERTICAL_TILES_INFO[move.dot]
                             : &HORIZONTAL_TILES_INFO[move.dot];

  return get_delta_evals(tile_info);
}

void Position::do_move(const PlayerMove& move) {
  const auto tile_info = move.orientation == VERTICAL
                             ? &VERTICAL_TILES_INFO[move.dot]
                             : &HORIZONTAL_TILES_INFO[move.dot];

  do_move(tile_info);
}

void Position::do_move(const ChanceMove& chance_move) {
  auto index = find_tile_index(chance_move);
  update_tile_index(index);
}

int Position::get_score(int col, int color) const {
  int score{0};
  for (auto v = columns[color][col].value; v > 0;) {
    // Find the least significant set bit
    int row = countr_zero(v);
    // Clear the least significant set bit
    v &= v - 1;

    for (uint16_t temp = v; temp > 0; temp &= temp - 1) {
      int b = countr_zero(temp) - row;
      if (col + b >= COLS) break;
      if (columns[color][col + b].test(row, row + b)) {
        score += b;
      }
    }
  }
  return score;
}

int Position::get_score(int color) const {
  int score{0};
  for (int col = 0; col < COLS; ++col) {
    score += get_score(col, color);
  }
  return score;
}

array<int, MAX_COLORS> Position::get_scores() const {
  array<int, MAX_COLORS> scores;
  for (int color : ALL_COLORS) {
    scores[color] = get_score(color);
  }
  return scores;
}

int Position::get_pessimist_score(Color color) const {
  auto scores = get_scores();
  auto my_color_idx = color - '1';
  auto s1 = scores[my_color_idx];
  auto s2 = 0;
  for (int c : ALL_COLORS) {
    if (c != my_color_idx) {
      s2 = max(s2, scores[c]);
    }
  }
  return (s1 - s2);
}

double Position::get_expected_score(Color color) const {
  if (opponent_color_index != -1) {
    auto my_color_index = color - '1';
    auto expected = get_score(my_color_index) - get_score(opponent_color_index);
    return expected;
  }

  auto scores = get_scores();
  auto expected = 0.0;
  for (int i = 0; i < MAX_COLORS; ++i) {
    expected += weights[i] * scores[i];
  }
  return expected;
}

const vector<tuple<int, int, int>> EVAL_DATA = []() {
  vector<tuple<int, int, int>> res;
  for (int col = 0; col < COLS; ++col) {
    for (int row = 0; row < ROWS; ++row) {
      for (int b = 1; b < 16 && row + b < ROWS && col + b < COLS; ++b) {
        res.emplace_back(col, row, b);
      }
    }
  }
  return res;
}();

double Position::evaluate(Color my_color) const {
  array<double, MAX_COLORS> evals;
  evals.fill(0.0);
  for (int color : ALL_COLORS) {
    for (const auto& [col, row, b] : EVAL_DATA) {
      constexpr double BASE{0.142857};
      double m{1.0};
      if (!columns[color][col].test(row)) m *= BASE;
      if (!columns[color][col].test(row + b)) m *= BASE;
      if (!columns[color][col + b].test(row)) m *= BASE;
      if (!columns[color][col + b].test(row + b)) m *= BASE;
      evals[color] += m * b;
    }
  }

  auto my_color_idx = my_color - '1';
  auto my_eval = evals[my_color_idx];
  auto best_others_eval{0.0};
  for (int c : ALL_COLORS) {
    if (c != my_color_idx) {
      best_others_eval = max(best_others_eval, evals[c]);
    }
  }

  return (my_eval - best_others_eval);
}

string Position::show() const {
  ostringstream out;
  out << "tile=" << tile << endl << endl;
  for (int col = 0; int dot : ALL_DOTS) {
    out << colors[dot];
    out << "|";
    ++col;
    if (col % COLS == 0) out << endl;
  }

  return out.str();
}

void Position::remove_candidate(int c) const {
  candidates[c] = candidates.back();
  candidates.pop_back();
}

const TileInfo* Position::get_random_move() {
  while (!candidates.empty()) {
    auto candidates_size = static_cast<int>(candidates.size());
    auto r = gen.less_than(candidates_size);
    auto info = candidates[r];
    if (auto c = info->count_matches(filled)) {
      remove_candidate(r);
      if (c <= MAX_OVERLAPS) {
        return info;
      }
    } else if (info->neighbour_to(filled)) {
      remove_candidate(r);
      return info;
    }
  }
  return nullptr;
}

uint64_t Position::get_hash() const {
  return player == PLAYER_1 ? (zobrist_hash ^ zobrist_player_1)
                            : (zobrist_hash ^ zobrist_player_2);
}

uint64_t Position::compute_hash() const {
  uint64_t hash{0};
  for (int dot : ALL_DOTS) {
    if (auto color = colors[dot]; color != WHITE) {
      hash ^= zobrist_colors[dot][color - '1'];
    }
  }

  if (tile_index != -1) {
    hash ^= zobrist_tiles[tile_index];
  }
  if (player == PLAYER_1) {
    hash ^= zobrist_player_1;
  } else if (player == PLAYER_2) {
    hash ^= zobrist_player_2;
  }
  return hash;
}

void Position::update_weigths(const array<double, MAX_COLORS>& impact,
                              Color my_color) {
  constexpr double BASE{10.0};
  constexpr double T{0.2};
  double min_eval = numeric_limits<double>::max();
  auto my_color_index = my_color - '1';
  for (int i = 0; i < MAX_COLORS; ++i) {
    if (i != my_color_index) {
      min_eval = min(min_eval, impact[i]);
    }
  }
  double sum_evals{0.0};
  for (int i = 0; i < MAX_COLORS; ++i) {
    if (i != my_color_index) {
      sum_evals += pow(BASE, T * (impact[i] - min_eval));
    }
  }
  for (int i = 0; i < MAX_COLORS; ++i) {
    weights[i] = i == my_color_index
                     ? 1.0
                     : -pow(BASE, T * (impact[i] - min_eval)) / sum_evals;
  }

  constexpr double WEIGHT_THRESHOLD{-0.67};
  opponent_color_index = -1;
  for (int i = 0; i < MAX_COLORS; ++i) {
    if (weights[i] <= WEIGHT_THRESHOLD) {
      opponent_color_index = i;
      break;
    }
  }

  if (opponent_color_index != -1) {
    cerr << "opponent_color_index=" << opponent_color_index << endl;
  }
  array_log("weights", weights);
}

void Position::init_weigths(Color my_color) {
  weights.fill(-0.2);
  weights[my_color - '1'] = 1.0;
}

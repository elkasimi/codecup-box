#pragma once

#include "Box.h"

struct TileSet {
  static constexpr int SIZE = ALL_TILES_COUNT;
  static constexpr int CHUNK_SIZE = 64;
  static constexpr int NUM_CHUNKS = (SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;

  array<uint64_t, NUM_CHUNKS> data{};
  int count{0};

  TileSet() = default;

  // Set a bit at position `pos`
  void set(int pos) {
    // assert(pos < SIZE && "Index out of bounds");
    data[pos / CHUNK_SIZE] |= (1ULL << (pos % CHUNK_SIZE));
    ++count;
  }

  // Clear a bit at position `pos`
  void clear(int pos) {
    // assert(pos < SIZE && "Index out of bounds");
    data[pos / CHUNK_SIZE] &= ~(1ULL << (pos % CHUNK_SIZE));
    --count;
  }

  // Check if a bit at position `pos` is set
  bool test(int pos) const {
    // assert(pos < SIZE && "Index out of bounds");
    return data[pos / CHUNK_SIZE] & (1ULL << (pos % CHUNK_SIZE));
  }

  // Check if any bit is set
  bool any() const { return count > 0; }

  // Iterate over set bits and call a function `func` with the bit index
  template <typename Func>
  void for_each(Func func) const {
    for (int chunk = 0; chunk < NUM_CHUNKS; ++chunk) {
      uint64_t current = data[chunk];
      while (current) {
        // Find the index of the least significant set bit
        int bit = countr_zero(current);  // GCC/Clang intrinsic
        auto tile_info = ALL_TILES_INFO[chunk * CHUNK_SIZE + bit];
        func(tile_info);
        current &= (current - 1);  // Clear the least significant bit
      }
    }
  }

  // Print the bitset (for debugging)
  ostream& operator<<(ostream& out) const {
    for (int i = 0; i < SIZE; ++i) {
      out << (test(i) ? '1' : '0');
      if ((i + 1) % CHUNK_SIZE == 0) out << ' ';
    }
    return out << '\n';
  }
};

struct Position {
  static constexpr int MAX_OVERLAPS{4};
  static constexpr Color WHITE{'0'};

  inline static array<double, MAX_COLORS> weights;
  inline static int opponent_color_index{-1};

  array<Color, TOTAL_DOTS> colors;
  Bitboard filled;
  struct Column {
    uint16_t value{0};

    void set(int r) { value |= (1 << r); }
    void unset(int r) { value &= ~(1 << r); }
    bool test(int r) const { return (value & (1 << r)) != 0; }
    bool test(int r0, int r1) const {
      return (value & (1 << r0)) && (value & (1 << r1));
    }
    bool operator==(const Column& col) const = default;
  };

  array<array<Column, COLS>, MAX_COLORS> columns;
  const char* tile{nullptr};
  uint64_t zobrist_hash{0};
  int tile_index{-1};
  int turn{0};
  Player player{PLAYER_1};

  explicit Position(const string& s);

  void play_chance_move();

  bool empty(int dot) const;
  bool possible_move(const TileInfo* tile_info) const;
  bool possible_move(int dot, Orientation orientation) const;
  void update_color(int dot, Color color);
  int bonus(int row, int col, int color) const;
  double eval(int row, int col, int color) const;
  array<double, MAX_COLORS> get_delta_evals(const TileInfo* info) const;
  array<double, MAX_COLORS> get_delta_evals(const PlayerMove& move) const;
  double eval(const TileInfo* info, Color c) const;
  array<int, MAX_COLORS> impact(const TileInfo* info) const;
  array<int, MAX_COLORS> impact(const PlayerMove& move) const;
  void process_siblings(const TileInfo* tile_info, int tile_index);
  void increment_turn();
  void update_tile_index(int index);
  int get_score(int col, int color) const;
  int get_score(int color) const;
  array<int, MAX_COLORS> get_scores() const;
  void do_move(const TileInfo* tile_info);
  void do_move(const PlayerMove& move);
  void do_move(const ChanceMove& move);
  vector<const TileInfo*> get_possible_tiles() const;

  TileSet get_possible_tiles_set() const;
  bool end_game() const;

  int get_pessimist_score(Color color) const;
  double get_expected_score(Color color) const;
  double evaluate(Color color) const;
  string show() const;

  mutable vector<const TileInfo*> candidates{ALL_TILES_INFO};

  const TileInfo* get_random_move();

  void remove_candidate(int c) const;

  struct Info {
    array<array<Column, COLS>, MAX_COLORS> columns;
    uint64_t hash;
    int32_t tile_index;
    int32_t player;
  };

  uint64_t get_hash() const;
  uint64_t compute_hash() const;

  Info get_info() const { return {columns, get_hash(), tile_index, player}; }

  void update_condidates() const {
    auto len = candidates.size();
    auto i = 0u;
    while (i < len) {
      auto tile_info = candidates[i];
      if (tile_info->count_matches(filled) > MAX_OVERLAPS) {
        --len;
        candidates[i] = candidates[len];
      } else {
        ++i;
      }
    }
    candidates.resize(len);
  }

  static void update_weigths(const array<double, MAX_COLORS>& impact,
                             Color my_color);
  static void init_weigths(Color my_color);
};

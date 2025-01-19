#pragma once

#include "STD.h"

constexpr int COLS{20};
constexpr int ROWS{16};
constexpr int TILE_DOTS{6};
constexpr int TOTAL_DOTS{ROWS * COLS};
constexpr int MAX_COLORS{6};

const auto ALL_DOTS = iota_view(0, TOTAL_DOTS);
const auto ALL_COLORS = iota_view(0, MAX_COLORS);

using Color = char;

using Orientation = char;
constexpr Orientation VERTICAL{'v'};
constexpr Orientation HORIZONTAL{'h'};

using Tile = string;

#if defined(__APPLE__)
using Bitboard = bitset<TOTAL_DOTS>;
#else
class Bitboard {
 public:
  inline static constexpr int SIZE{3};

  Bitboard() { reset(); }

  // Reset all bits to 0
  void reset() {
    for (auto& d : data) {
      d = _mm_setzero_si128();
    }
  }

  // Bitwise OR-assignment operator
  Bitboard& operator|=(const Bitboard& other) {
    for (size_t i = 0; i < SIZE; ++i) {
      data[i] = _mm_or_si128(data[i], other.data[i]);
    }
    return *this;
  }

  // Bitwise NOT operator
  Bitboard operator~() const {
    Bitboard result;
    __m128i all_ones = _mm_set1_epi32(-1);
    for (size_t i = 0; i < SIZE; ++i) {
      result.data[i] = _mm_xor_si128(data[i], all_ones);
    }
    return result;
  }

  // Equality operator
  bool operator==(const Bitboard& other) const {
    for (size_t i = 0; i < SIZE; ++i) {
      __m128i cmp = _mm_cmpeq_epi64(data[i], other.data[i]);
      if (_mm_movemask_epi8(cmp) != 0xFFFF) {
        return false;
      }
    }
    return true;
  }

  // Bitwise AND operator
  Bitboard operator&(const Bitboard& other) const {
    Bitboard result;
    for (size_t i = 0; i < SIZE; ++i) {
      result.data[i] = _mm_and_si128(data[i], other.data[i]);
    }
    return result;
  }

  int count_matches(const Bitboard& other) const {
    int count{0};
    for (size_t i = 0; i < SIZE; ++i) {
      __m128i result = _mm_and_si128(data[i], other.data[i]);

      // Extract the 128-bit result into two 64-bit integers
      uint64_t lower = _mm_extract_epi64(result, 0);
      uint64_t upper = _mm_extract_epi64(result, 1);

      count += __builtin_popcountll(lower) + __builtin_popcountll(upper);
    }
    return count;
  }

  bool any_matches(const Bitboard& other) const {
    for (size_t i = 0; i < SIZE; ++i) {
      __m128i result = _mm_and_si128(data[i], other.data[i]);
      if (!_mm_testz_si128(result, result)) return true;
    }
    return false;
  }

  // Set a bit at a specific position
  void set(size_t pos) {
    apply_mask(pos, [](auto a, auto b) { return _mm_or_si128(a, b); });
  }

  // Clear a specific bit at a given position
  void reset(size_t pos) {
    apply_mask(pos, [](auto a, auto b) { return _mm_andnot_si128(b, a); });
  }

  // Toggle a bit at a specific position
  void toggle(size_t pos) {
    apply_mask(pos, [](auto a, auto b) { return _mm_xor_si128(a, b); });
  }

  // Check if a specific bit is set
  bool test(size_t pos) const {
    __m128i mask = create_mask(pos);
    __m128i result = _mm_and_si128(data[pos / 128], mask);
    return !_mm_testz_si128(result, result);
  }

  // Check if any bit is set
  bool any() const {
    for (const auto& d : data) {
      if (!_mm_testz_si128(d, d)) {
        return true;
      }
    }
    return false;
  }

  // Check if all bits are zero
  bool none() const { return !any(); }

  // Count the number of set bits
  int count() const {
    int total_count = 0;
    for (const auto& d : data) {
      uint64_t lower = _mm_cvtsi128_si64(d);
      uint64_t upper = _mm_extract_epi64(d, 1);
      total_count += popcount(lower) + popcount(upper);
    }
    return total_count;
  }

  // Print the bitset for debugging
  void print() const {
    for (size_t i = 0; i < 320; ++i) {
      cout << test(i);
      if (i % 32 == 31) cout << " ";
    }
    cout << '\n';
  }

 private:
  alignas(16) __m128i data[3];

  // Helper function to create a mask for a specific bit position
  __m128i create_mask(size_t pos) const {
    size_t bit_offset = pos % 128;
    if (bit_offset < 64) {
      return _mm_set_epi64x(0, 1ULL << bit_offset);
    } else {
      return _mm_set_epi64x(1ULL << (bit_offset - 64), 0);
    }
  }

  // Helper function to apply a mask operation at a specific index
  template <typename Operation>
  void apply_mask(size_t pos, Operation op) {
    __m128i mask = create_mask(pos);
    data[pos / 128] = op(data[pos / 128], mask);
  }
};
#endif

// Variadic template function to set multiple bits in a Bitboard
template <typename... Bits>
constexpr Bitboard mk_bitboard(Bits... bits) {
  Bitboard bitboard;
  // Set each bit using a fold expression
  (..., bitboard.set(bits));
  return bitboard;
}

inline int get_dot(int r, int c) { return r * COLS + c; }

inline string show_dot(int dot) {
  ostringstream out;
  int r = dot / COLS;
  int c = dot % COLS;
  out << static_cast<char>(r + 'A') << static_cast<char>(c + 'a');
  return out.str();
}

inline vector<string> get_dots(const Bitboard& bitboard) {
  vector<string> res{};
  for (int dot : ALL_DOTS) {
    if (bitboard.test(dot)) {
      res.push_back(show_dot(dot));
    }
  }
  return res;
}

struct PlayerMove {
  int dot{-1};
  Orientation orientation{'?'};

  string show() const {
    ostringstream out;
    out << show_dot(dot) << orientation;
    return out.str();
  }

  int code() const {
    return orientation == VERTICAL ? (2 * dot + 1) : (2 * dot);
  }

  static int code(int dot, Orientation o) {
    return o == VERTICAL ? (2 * dot + 1) : (2 * dot);
  }
};

using ChanceMove = Tile;

inline int parse_dot(const string& s) {
  return (s[0] - 'A') * COLS + s[1] - 'a';
}

inline pair<ChanceMove, PlayerMove> parse_moves(const string& s) {
  constexpr size_t PLAYER_MOVE_LENGTH{3};
  constexpr size_t CHANCE_MOVE_LENGTH{6};
  assert(s.size() == CHANCE_MOVE_LENGTH + PLAYER_MOVE_LENGTH);
  auto chance_move = s.substr(2, 6);
  auto orientation = s[8];
  auto dot = parse_dot(s);
  PlayerMove player_move{dot, orientation};
  return {chance_move, player_move};
}

using Player = char;
constexpr Player PLAYER_1 = '1';
constexpr Player PLAYER_2 = '2';

struct TileInfo {
  array<pair<int, int>, TILE_DOTS> siblings;
  Bitboard bitboard;
  Bitboard neighbors_bitboard;
  mutable int code;
  int dot;
  Orientation orientation;

  bool valid() const { return bitboard.any(); }
  void clear() {
    bitboard.reset();
    neighbors_bitboard.reset();
  }
  PlayerMove move() const { return {dot, orientation}; }
  auto count_matches(const Bitboard& b) const {
#ifdef __APPLE__
    return (bitboard & b).count();
#else
    return bitboard.count_matches(b);
#endif
  }
  auto count_matches(const TileInfo& info) const {
    return count_matches(info.bitboard);
  }
  auto none_matches(const Bitboard& b) const { return (bitboard & b).none(); }
  auto neighbour_to(const Bitboard& b) const {
#ifdef __APPLE__
    return (neighbors_bitboard & b).any();
#else
    return neighbors_bitboard.any_matches(b);
#endif
  }
  auto neighbour_to(const TileInfo& info) const {
#ifdef __APPLE__
    return (neighbors_bitboard & info.bitboard).any();
#else
    return neighbors_bitboard.any_matches(info.bitboard);
#endif
  }
  auto top() const {
    array<int, TILE_DOTS> res;
    for (int i = 0; i < TILE_DOTS; ++i) {
      res[i] = siblings[i].first;
    }
    return res;
  }

  auto bottom() const {
    array<int, TILE_DOTS> res;
    for (int i = 0; i < TILE_DOTS; ++i) {
      res[i] = siblings[TILE_DOTS - i - 1].second;
    }
    return res;
  }
};

struct Stats {
  double value{0.0};
  int visits{0};

  void update(double v) {
    ++visits;
    auto delta = v - value;
    value += delta / visits;
  }

  ostream& operator<<(ostream& out) const {
    return out << "(" << value << ", " << visits << ")";
  }
};

extern const vector<TileInfo> VERTICAL_TILES_INFO;
extern const vector<TileInfo> HORIZONTAL_TILES_INFO;
extern const vector<const TileInfo*> ALL_TILES_INFO;
constexpr int ALL_TILES_COUNT{434};
extern const TileInfo* const CENTER_TILE_INFO;
constexpr int TILES_PERMUTATIONS_COUNT{6 * 5 * 4 * 3 * 2 * 1};
extern const vector<Tile> TILES_PERMUTATIONS;

inline int find_tile_index(const Tile& tile) {
  if (auto it = ranges::lower_bound(TILES_PERMUTATIONS, tile);
      it != TILES_PERMUTATIONS.end() && *it == tile) {
    return static_cast<int>(distance(TILES_PERMUTATIONS.begin(), it));
  }
  return -1;
}

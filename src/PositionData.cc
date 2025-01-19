#include "Position.h"

bool valid_dot(int r, int c) {
  return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

vector<int> get_neighbors(int r, int c) {
  vector<int> res;
  if (valid_dot(r + 1, c)) res.push_back(get_dot(r + 1, c));
  if (valid_dot(r - 1, c)) res.push_back(get_dot(r - 1, c));
  if (valid_dot(r, c + 1)) res.push_back(get_dot(r, c + 1));
  if (valid_dot(r, c - 1)) res.push_back(get_dot(r, c - 1));
  return res;
}

vector<int> get_neighbors(int dot) {
  int r = dot / COLS;
  int c = dot % COLS;
  return get_neighbors(r, c);
}

Bitboard generate_tile_neighbors_bitboard(const TileInfo& info) {
  Bitboard b{};

  for (auto [d1, d2] : info.siblings) {
    for (int dot : {d1, d2}) {
      for (int n : get_neighbors(dot)) {
        b.set(n);
      }
    }
  }
  b = b & (~info.bitboard);
  return b;
}

vector<TileInfo> generate_vertical_tiles_info() {
  vector<TileInfo> res;
  for (int dot : ALL_DOTS) {
    int r = dot / COLS;
    int c = dot % COLS;

    TileInfo info;
    // fill dot & orientation
    info.code = 2 * get_dot(r, c) + 1;
    info.dot = get_dot(r, c);
    info.orientation = VERTICAL;

    array<int, TILE_DOTS> top;
    array<int, TILE_DOTS> bottom;
    // fill top
    for (int i = 0; i < TILE_DOTS; ++i) {
      if (!valid_dot(r + i, c + 1)) {
        info.clear();
        break;
      }
      int di = get_dot(r + i, c + 1);
      top[i] = di;
      info.bitboard.set(di);
    }
    if (!info.valid()) {
      res.push_back(info);
      continue;
    }
    // fill bottom
    for (int i = 0; i < TILE_DOTS; ++i) {
      if (!valid_dot(r + i, c)) {
        info.clear();
        break;
      }
      int di = get_dot(r + i, c);
      bottom[i] = di;
      info.bitboard.set(di);
    }

    if (!info.valid()) {
      res.push_back(info);
      continue;
    }

    // fill siblings
    for (int i = 0; i < TILE_DOTS; ++i) {
      info.siblings[i].first = top[i];
      info.siblings[i].second = bottom[TILE_DOTS - i - 1];
    }

    // fill neighbors
    info.neighbors_bitboard = generate_tile_neighbors_bitboard(info);

    res.push_back(info);
  }

  return res;
}

vector<TileInfo> generate_horizontal_tiles_info() {
  vector<TileInfo> res;
  for (int dot : ALL_DOTS) {
    int r = dot / COLS;
    int c = dot % COLS;
    TileInfo info;
    // fill dot & orientation
    info.code = 2 * get_dot(r, c);
    info.dot = get_dot(r, c);
    info.orientation = HORIZONTAL;

    array<int, TILE_DOTS> top;
    array<int, TILE_DOTS> bottom;
    // fill top
    for (int i = 0; i < TILE_DOTS; ++i) {
      if (!valid_dot(r, c + i)) {
        info.clear();
        break;
      }
      int di = get_dot(r, c + i);
      top[i] = di;
      info.bitboard.set(di);
    }

    if (!info.valid()) {
      res.push_back(info);
      continue;
    }

    // fill bottom
    for (int i = 0; i < TILE_DOTS; ++i) {
      if (!valid_dot(r + 1, c + i)) {
        info.clear();
        break;
      }
      int di = get_dot(r + 1, c + i);
      bottom[i] = di;
      info.bitboard.set(di);
    }
    if (!info.valid()) {
      res.push_back(info);
      continue;
    }

    // fill siblings
    for (int i = 0; i < TILE_DOTS; ++i) {
      info.siblings[i].first = top[i];
      info.siblings[i].second = bottom[TILE_DOTS - i - 1];
    }

    // fill neighbors
    info.neighbors_bitboard = generate_tile_neighbors_bitboard(info);

    res.push_back(info);
  }
  return res;
}

// generate all tiles permutations
vector<Tile> generate_all_tiles_permutations() {
  Tile tile{"123456"};
  vector<Tile> res;
  do {
    res.push_back(tile);
  } while (ranges::next_permutation(tile).found);
  return res;
}

const vector<TileInfo> VERTICAL_TILES_INFO = generate_vertical_tiles_info();
const vector<TileInfo> HORIZONTAL_TILES_INFO = generate_horizontal_tiles_info();
const vector<Tile> TILES_PERMUTATIONS = generate_all_tiles_permutations();
const vector<const TileInfo*> ALL_TILES_INFO = []() {
  vector<const TileInfo*> res;
  int code = 0;
  for (const auto& info : VERTICAL_TILES_INFO) {
    if (info.valid()) {
      info.code = code++;
      res.push_back(&info);
    }
  }
  for (const auto& info : HORIZONTAL_TILES_INFO) {
    if (info.valid()) {
      info.code = code++;
      res.push_back(&info);
    }
  }
  return res;
}();

const TileInfo* const CENTER_TILE_INFO =
    &HORIZONTAL_TILES_INFO[parse_dot("Hh")];

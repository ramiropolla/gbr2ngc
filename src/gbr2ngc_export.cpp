/*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This program was written while working at Bright Works Computer Consulting
* and allowed to be GPL'd under express permission of the current president
* John Guttridge
* Dated May 20th 2013
*/

#include <inttypes.h>

#include "gbr2ngc.hpp"

static double unit_mm2in(double v) {
  return v/25.4;
}

static double unit_in2mm(double v) {
  return v*25.4;
}

static double unit_identity(double v) {
  return v;
}

// Export a rapid (G0) movement to file.
// `axes` is a string containing the letters of the axes to move.
// Up to three axis coordinates can be provided.
// Ex: rapid(f, "xz", 3, -5) will move X3 and Y-5
//
void rapid(FILE* file, const char* axes, double one, double two, double three) {
  int i;
  double coords[] = {one, two, three};

  if (gSeekRateSet && gCurRate != gSeekRate) {
    fprintf(file, "%c%" GCODE_LENGTH_PRECISION "f\n", char_F, (double)gSeekRate);
    gCurRate = gSeekRate;
  }

  fprintf(file, "%c00", char_G);
  for (i = 0; axes[i] != '\0'; i++) {
    char axis = (gUppercase ? toupper(axes[i]) : tolower(axes[i]));
    if (gHumanReadable)
      fputc(' ', file);
    fprintf(file, "%c%." GCODE_LENGTH_PRECISION "f", axis, coords[i]);
  }

  fprintf(file, "\n");
}

// G1 equiv. of rapid()
//
void cut(FILE* file, const char* axes, double one, double two, double three) {
  int i;
  double coords[] = {one, two, three};

  if (gFeedRateSet && gCurRate != gFeedRate) {
    fprintf(file, "%c%" GCODE_LENGTH_PRECISION "f\n", char_F, (double)gFeedRate);
    gCurRate = gFeedRate;
  }

  fprintf(file, "%c01", char_G);
  for (i = 0; axes[i] != '\0'; i++) {
    char axis = (gUppercase ? toupper(axes[i]) : tolower(axes[i]));
    if (gHumanReadable)
      fputc(' ', file);
    fprintf(file, "%c%." GCODE_LENGTH_PRECISION "f", axis, coords[i]);
  }

  fprintf(file, "\n");
}

typedef int city_t;

static void reorder_paths(std::vector<city_t> *_dst, const Paths &src)
{
  std::vector<city_t> &dst = *_dst;

  // drop empty paths
  const city_t src_size = src.size();
  std::vector<city_t> result_map;
  result_map.reserve(src_size);
  for ( city_t i = 0; i < src_size; i++ )
  {
    const Path &path = src[i];
    if ( !path.empty() )
      result_map.push_back(i);
  }

  // populate costs
  const city_t n_total = result_map.size();
  city_t costs_size = n_total + 1; // +1 for origin
  int64_t *costs = (int64_t *) malloc(sizeof(int64_t) * costs_size * costs_size);
  const IntPoint origin(gOffsetX, gOffsetY);
  for ( city_t i = 0; i < costs_size; i++ )
  {
    const IntPoint *from;
    if ( i == n_total )
    {
      from = &origin;
    }
    else
    {
      city_t i_idx = result_map[i];
      if ( gDrill )
        from = &src[i_idx].back();
      else
        from = &src[i_idx].front();
    }
    for ( city_t j = 0; j < costs_size; j++ )
    {
      const IntPoint *to;
      if ( j == n_total )
        to = &origin;
      else
        to = &src[result_map[j]].front();
      // Chebyshev distance
      int64_t distance_x = std::abs(to->X - from->X);
      int64_t distance_y = std::abs(to->Y - from->Y);
      int64_t distance = std::max(distance_x, distance_y);
      costs[i * costs_size + j] = distance;
    }
  }

  // calculate initial cost
  int64_t total_cost = 0;
  city_t prev_i = n_total; // we know the first element is origin
  for ( city_t i = 0; i < n_total; i++ )
  {
    total_cost += costs[prev_i * costs_size + i];
    prev_i = i;
  }
  total_cost += costs[prev_i * costs_size + n_total];
  printf("initial_cost %" PRId64 "\n", total_cost);

  // nearest neighbor
  std::vector<city_t> result_nn(n_total);
  bool *used = (bool *) malloc(sizeof(bool) * n_total);
  for ( city_t i = 0; i < n_total; i++ )
    used[i] = false;
  total_cost = 0;
  prev_i = n_total; // we know the first element is origin
  for ( city_t i = 0; i < n_total; i++ )
  {
    city_t best_j = n_total;
    int64_t best_cost;
    for ( city_t j = 0; j < n_total; j++ )
    {
      if ( i == j || used[j] )
        continue;
      int64_t cost = costs[prev_i * costs_size + j];
      if ( (best_j == n_total) || (cost < best_cost) )
      {
        best_cost = cost;
        best_j = j;
      }
    }
    result_nn[i] = best_j;
    used[best_j] = true;
    total_cost += costs[prev_i * costs_size + best_j];
    prev_i = best_j;
  }
  total_cost += costs[prev_i * costs_size + n_total];
  printf("nn_cost %" PRId64 "\n", total_cost);
  free(used);

  // 2-opt
//   std::vector<city_t> result_2opt(n_total);
  std::vector<city_t> result_2opt = result_nn;
#if 1
_2opt:
#if 0
  for ( city_t x : result_2opt ) { printf("%3zu,", (size_t) x); } printf("\n");
#endif
  for ( city_t i = -1; i < (n_total - 2); i++ )
  {
    city_t i0_idx = (i == -1) ? n_total : result_2opt[i];
    city_t i1_idx = result_2opt[i + 1];
    for ( city_t j = i + 2; j < n_total; j++ )
    {
      city_t j0_idx = result_2opt[j];
      city_t j1_idx = (j == (n_total - 1)) ? n_total : result_2opt[j + 1];
      int64_t delta = costs[i0_idx * costs_size + j0_idx] + costs[i1_idx * costs_size + j1_idx]
                    - costs[i0_idx * costs_size + i1_idx] - costs[j0_idx * costs_size + j1_idx];
#if 0
      printf("+cost[%zu][%zu]: %" PRId64 "\n", (size_t) i0_idx, (size_t) j0_idx, costs[i0_idx * costs_size + j0_idx]);
      printf("+cost[%zu][%zu]: %" PRId64 "\n", (size_t) i1_idx, (size_t) j1_idx, costs[i1_idx * costs_size + j1_idx]);
      printf("-cost[%zu][%zu]: %" PRId64 "\n", (size_t) i0_idx, (size_t) i1_idx, costs[i0_idx * costs_size + i1_idx]);
      printf("-cost[%zu][%zu]: %" PRId64 "\n", (size_t) j0_idx, (size_t) j1_idx, costs[j0_idx * costs_size + j1_idx]);
      printf("delta[%zu][%zu]: %" PRId64 "\n", (size_t) i, (size_t) j, delta);
#endif
      if ( delta < 0 )
      {
        std::reverse(result_2opt.begin() + i + 1, result_2opt.begin() + j + 1);
        total_cost += delta;
        goto _2opt;
      }
    }
  }
  printf("2opt_cost %" PRId64 "\n", total_cost);
  for ( city_t x : result_2opt ) { printf("%3zu,", (size_t) x); } printf("\n");
#endif

  // 3-opt
//   std::vector<city_t> result_3opt(n_total);
  std::vector<city_t> result_3opt = result_2opt;
#if 1
_3opt:
#if 0
  for ( city_t x : result_3opt ) { printf("%3zu,", (size_t) x); } printf("\n");
#endif
  // 0...i...j...k...m...0
  for ( city_t i = -1; i < (n_total - 3); i++ )
  {
    city_t i0_idx = (i == -1) ? n_total : result_3opt[i];
    city_t i1_idx = result_3opt[i + 1];
    for ( city_t j = i + 1; j < (n_total - 2); j++ )
    {
      city_t j0_idx = result_3opt[j];
      city_t j1_idx = result_3opt[j + 1];
      for ( city_t k = j + 1; k < (n_total - 1); k++ )
      {
        city_t k0_idx = result_3opt[k];
        city_t k1_idx = result_3opt[k + 1];
        for ( city_t m = k + 1; m < n_total; m++ )
        {
          city_t m0_idx = result_3opt[m];
          city_t m1_idx = (m == (n_total - 1)) ? n_total : result_3opt[m + 1];
          for ( int n = 1; n < 8; n++ )
          {
            int64_t delta = 0;
#if 1
            if ( n & 0x01 )
              delta += costs[k0_idx * costs_size + m0_idx] + costs[k1_idx * costs_size + m1_idx] - costs[k0_idx * costs_size + k1_idx] - costs[m0_idx * costs_size + m1_idx];
            if ( n & 0x02 )
              delta += costs[j0_idx * costs_size + k0_idx] + costs[j1_idx * costs_size + k1_idx] - costs[j0_idx * costs_size + j1_idx] - costs[k0_idx * costs_size + k1_idx];
            if ( n & 0x04 )
              delta += costs[i0_idx * costs_size + j0_idx] + costs[i1_idx * costs_size + j1_idx] - costs[i0_idx * costs_size + i1_idx] - costs[j0_idx * costs_size + j1_idx];
#else
            switch ( n )
            {
            case 1: // 001
              delta = costs[k0_idx * costs_size + m0_idx] + costs[k1_idx * costs_size + m1_idx]
                    - costs[k0_idx * costs_size + k1_idx] - costs[m0_idx * costs_size + m1_idx];
              break;
            case 2: // 010
              delta = costs[j0_idx * costs_size + k0_idx] + costs[j1_idx * costs_size + k1_idx]
                    - costs[j0_idx * costs_size + j1_idx] - costs[k0_idx * costs_size + k1_idx];
              break;
            case 3: // 011
              delta = costs[j0_idx * costs_size + m0_idx] + costs[j1_idx * costs_size + m1_idx]
                    - costs[j0_idx * costs_size + j1_idx] - costs[m0_idx * costs_size + m1_idx];
              break;
            case 4: // 100
              delta = costs[i0_idx * costs_size + j0_idx] + costs[i1_idx * costs_size + j1_idx]
                    - costs[i0_idx * costs_size + i1_idx] - costs[j0_idx * costs_size + j1_idx];
              break;
            case 5: // 101
              delta = costs[i0_idx * costs_size + j0_idx] + costs[i1_idx * costs_size + j1_idx]
                    + costs[k0_idx * costs_size + m0_idx] + costs[k1_idx * costs_size + m1_idx]
                    - costs[i0_idx * costs_size + i1_idx] - costs[j0_idx * costs_size + j1_idx]
                    - costs[k0_idx * costs_size + k1_idx] - costs[m0_idx * costs_size + m1_idx];
              break;
            case 6: // 110
              delta = costs[i0_idx * costs_size + k0_idx] + costs[i1_idx * costs_size + k1_idx]
                    - costs[i0_idx * costs_size + i1_idx] - costs[k0_idx * costs_size + k1_idx];
              break;
            case 7: // 111
              delta = costs[i0_idx * costs_size + m0_idx] + costs[i1_idx * costs_size + m1_idx]
                    - costs[i0_idx * costs_size + i1_idx] - costs[m0_idx * costs_size + m1_idx];
              break;
            }
#endif
            if ( delta >= 0 )
              continue;
            if ( n & 0x01 )
              std::reverse(result_3opt.begin() + k + 1, result_3opt.begin() + m + 1);
            if ( n & 0x02 )
              std::reverse(result_3opt.begin() + j + 1, result_3opt.begin() + k + 1);
            if ( n & 0x04 )
              std::reverse(result_3opt.begin() + i + 1, result_3opt.begin() + j + 1);
            total_cost += delta;
            goto _3opt;
          }
        }
      }
    }
  }
  printf("3opt_cost %" PRId64 "\n", total_cost);
  for ( city_t x : result_3opt ) { printf("%3zu,", (size_t) x); } printf("\n");
#endif

  free(costs);

  dst.resize(n_total);
  for ( city_t i = 0; i < n_total; i++ )
    dst[i] = result_map[result_3opt[i]];
}

int export_paths_to_gcode_unit( FILE *ofp, const Paths &paths, int src_units_0in_1mm, int dst_units_0in_1mm, double ds)
{
  std::vector<city_t> reordered_paths;
  double (*f)(double);

  f = unit_identity;
  if (src_units_0in_1mm != dst_units_0in_1mm) {
    if ((src_units_0in_1mm == 1) && (dst_units_0in_1mm == 0)) {
      f = unit_mm2in;
    } else {
      f = unit_in2mm;
    }
  }

  if (gGCodeHeader)   { fprintf(ofp, "%s\n", gGCodeHeader); }

  if (gHumanReadable && gShowComments) { fprintf(ofp, "\n"); }
  if (gShowComments)  { fprintf(ofp, "\n( feed %i seek %i zsafe %f zcut %f )\n", gFeedRate, gSeekRate, gZSafe, gZCut ); }

  reorder_paths(&reordered_paths, paths);

  for ( city_t i : reordered_paths )
  {
    const Path &path = paths[i];
    bool first = true;

    if (gHumanReadable && gShowComments) { fprintf(ofp, "\n\n"); }
    if (gShowComments)  { fprintf(ofp, "( path %zu )\n", (size_t) i); }

    if (gDrill)
    {
      for (const IntPoint &pt : path)
      {
        double x = f(ctod(offsetX(pt)));
        double y = f(ctod(offsetY(pt)));
        if (first) {
          rapid(ofp, "xy", x, y);
          cut(ofp, "z", gZCut);
          first = false;
        } else {
          cut(ofp, "xy", x, y);
        }
      }

      // raise drill bit
      cut(ofp, "z", 0);
      rapid(ofp, "z", gZSafe);
    }
    else
    {
      double start_x = f(ctod( offsetX(path[0]) ));
      double start_y = f(ctod( offsetY(path[0]) ));

      for (const IntPoint &pt : path)
      {
        double x = f(ctod(offsetX(pt)));
        double y = f(ctod(offsetY(pt)));
        if (first) {
          rapid(ofp, "xy", x, y);
          cut(ofp, "z", gZCut);
          first = false;
        } else {
          cut(ofp, "xy", x, y);
        }
      }

      // go back to start
      cut(ofp, "xy", start_x, start_y);
      rapid(ofp, "z", gZSafe);
    }
  }

  if (gHumanReadable && gShowComments) { fprintf(ofp, "\n\n"); }
  if (gGCodeFooter)   { fprintf(ofp, "%s\n", gGCodeFooter); }

  return 0;
}

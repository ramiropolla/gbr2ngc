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

int export_paths_to_gcode_unit( FILE *ofp, const Paths &paths, int src_units_0in_1mm, int dst_units_0in_1mm, double ds)
{
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

  size_t i = 0;
  for (const Path &path : paths)
  {
    bool first = true;

    if (gHumanReadable && gShowComments) { fprintf(ofp, "\n\n"); }
    if (gShowComments)  { fprintf(ofp, "( path %zu )\n", i++); }

    if (path.empty())
      continue;

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

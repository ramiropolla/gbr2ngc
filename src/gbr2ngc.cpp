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

#include <unistd.h>
#include <inttypes.h>

// gbr2ngc will look here by default for a config file
//
#define DEFAULT_CONFIG_FILENAME "./gbr2ngc.ini"

// Users should specify boolean options as "yes" or "no"
// (without quotes).
//
// e.g.
//  - verbose = yes|no
//
#define CONFIG_FILE_YES "yes"
#define CONFIG_FILE_NO "no"

// There are only 26 letters in the alphabet; 52 including uppercase. As the
// program grows in complexity and cabability, the letter choices available
// for command-line arguments will get slim. One solution is to assign numbers
// instead of letters for less-commonly used or advanced options.
//
#define ARG_GCODE_HEADER '2'
#define ARG_GCODE_FOOTER '3'

static struct option gLongOption[] = {
  {"radius",            required_argument, nullptr, 'r'},
  {"fillradius",        required_argument, nullptr, 'F'},

  {"input",             required_argument, nullptr, 'i'},
  {"output",            required_argument, nullptr, 'o'},
  {"config-file",       required_argument, nullptr, 'c'},

  {"feed",              required_argument, nullptr, 'f'},
  {"seek",              required_argument, nullptr, 's'},
  {"spindle-speed",     required_argument, nullptr, 'S'},

  {"zsafe",             required_argument, nullptr, 'z'},
  {"zcut",              required_argument, nullptr, 'Z'},

  {"find-extremes",     no_argument,       nullptr, 'O'},
  {"offset-x",          required_argument, nullptr, 'x'},
  {"offset-y",          required_argument, nullptr, 'y'},
  {"scale-x",           required_argument, nullptr, 'X'},
  {"scale-y",           required_argument, nullptr, 'Y'},

  {"gcode-header",      required_argument, nullptr, ARG_GCODE_HEADER},
  {"gcode-footer",      required_argument, nullptr, ARG_GCODE_FOOTER},

  {"segment-length",    required_argument, nullptr, 'l'},

  {"metric",            no_argument,       nullptr, 'M'},
  {"inches",            no_argument,       nullptr, 'I'},

  {"no-comment",        no_argument,       nullptr, 'C'},
  {"uppercase",         no_argument,       nullptr, 'U'},
  {"machine-readable",  no_argument,       nullptr, 'R'},

  {"horizontal",        no_argument,       nullptr, 'H'},
  {"vertical",          no_argument,       nullptr, 'V'},
  {"zengarden",         no_argument,       nullptr, 'G'},

  {"invertfill",        no_argument,       &gInvertFlag, 1},
  {"simple-infill",     no_argument,       &gSimpleInfill, 1},
  {"no-outline",        no_argument,       &gDrawOutline, 0},
  {"drill",             no_argument,       nullptr, 'd'},

  {"verbose",           no_argument,       nullptr, 'v'},
  {"version",           no_argument,       nullptr, 'N'},
  {"help",              no_argument,       nullptr, 'h'},

  {nullptr, no_argument, nullptr, 0}
};

static char gOptionDescription[][1024] =
{
  "radius (default 0) (units in inches)",
  "radius to be used for fill pattern (default to radius above)",

  "input file",
  "output file (default stdout)",
  "configuration file (default ./gbr2ngc.ini)",

  "feed rate (default 10)",
  "seek rate (add 'g0 f<rate>' to header if set)",
  "spindle speed (default 1000)",

  "z safe height (default 0.1 inches)",
  "z cut height (default -0.05 inches)",

  "find x and y extremes and exit",
  "offset for x coordinates in gerber file",
  "offset for y coordinates in gerber file",
  "scale for x coordinates in gerber file",
  "scale for y coordinates in gerber file",

  "prepend custom G-code to the beginning of the program",
  "append custom G-code to the end of the program",

  "minimum segment length",

  "output units in metric",
  "output units in inches (default)",

  "do not show comments",
  "uppercase",
  "machine readable (uppercase, no spaces in gcode)",

  "route out blank areas with a horizontal scan line technique",
  "route out blank areas with a vertical scan line technique",
  "route out blank areas with a 'zen garden' technique",

  "invert the fill pattern (experimental)",
  "infill copper polygons with pattern (currently only -H and -V supported)",
  "do not route out outline when doing infill",
  "drill holes/slots",

  "verbose",
  "display version information",
  "help (this screen)",

  "n/a"
};

static option lookup_option_by_name(const char* name)
{
  for (int i = 0; gLongOption[i].name; i++) {
    const option opt = gLongOption[i];
    if (strcmp(opt.name, name) == 0) {
      return opt;
    }
  }

  const option no_option = {0, 0, 0, 0};

  return no_option;
}

static void show_version(FILE *fp)
{
  fprintf(fp, "version %s\n", GBL2NGC_VERSION);
}

static void show_help(FILE *fp)
{
  int i, j, len;

  fprintf(fp, "\ngbr2ngc: A gerber to gcode converter\n");
  show_version(fp);
  fprintf(fp, "\n");
  fprintf(fp, "  usage: gbr2ngc [<options>] [<input_Gerber>] [-o <output_GCode_file>]\n");
  fprintf(fp, "\n");

  for (i=0; gLongOption[i].name; i++) {
    len = strlen(gLongOption[i].name);

    if (gLongOption[i].flag != 0) {
      fprintf(fp, "  --%s", gLongOption[i].name);
      len -= 4;
    } else {
      if (gLongOption[i].val != 0) {
        fprintf(fp, "  -%c, --%s", gLongOption[i].val, gLongOption[i].name);
      }
      else {
        fprintf(fp, "  --%s", gLongOption[i].name);
      }
    }

    if (gLongOption[i].has_arg) {
      fprintf(fp, " %s", gLongOption[i].name);

      len *= 2;
      if (gLongOption[i].val != 0) { len += 3; }
      else { len -= 1; }
    }
    else {
      len = len + 2;
    }
    for (j=0; j<(32-len); j++) fprintf(fp, " ");

    fprintf(fp, "%s\n", gOptionDescription[i]);
  }

  fprintf(fp, "\n");
}

// Use as a go-between for the internal globals and optarg,
// which could either come from a config file or be NULL if
// the option was given as a command line switch.
// `default_` is the value that is returned if the option is
// set to CONFIG_FILE_YES or given as a CLI switch.
//
static bool bool_option(const char* optarg, bool default_ = true)
{
  if (optarg == NULL) {
    return default_;
  }

  else if (strcmp(optarg, CONFIG_FILE_YES) == 0) {
    return default_;
  }

  else if (strcmp(optarg, CONFIG_FILE_NO) == 0) {
    return !default_;
  }

  // Treat all other values for optarg as NO
  return !default_;
}

static bool set_option(const char option_char, const char* optarg)
{
  switch(option_char) {
    case 'C':
      gShowComments = bool_option(optarg, false);
      break;
    case 'U':
      gUppercase = bool_option(optarg);
      break;
    case 'R':
      gHumanReadable = bool_option(optarg, false);
      break;
    case 'r':
      gRadius = atof(optarg);
      break;
    case 'F':
      gFillRadius = atof(optarg);
      break;
    case '$':
      gFillRadius = atof(optarg);
      break;
    case 's':
      gSeekRateSet = true;
      gSeekRate = atoi(optarg);
      break;
    case 'z':
      gZSafe = atof(optarg);
      break;
    case 'Z':
      gZCut = atof(optarg);
      break;
    case 'O':
      gFindExtremes = bool_option(optarg);
      break;
    case 'x':
      gOffsetX = atoll(optarg);
      break;
    case 'y':
      gOffsetY = atoll(optarg);
      break;
    case 'X':
      gScaleX = atof(optarg);
      break;
    case 'Y':
      gScaleY = atof(optarg);
      break;
    case 'f':
      gFeedRate = atoi(optarg);
      gFeedRateSet = true;
      break;
    case 'S':
      gSpindleSpeed = atoi(optarg);
      gSpindleSpeedSet = true;
      break;

    case ARG_GCODE_HEADER:
      gGCodeHeader = strdup(optarg);
      break;
    case ARG_GCODE_FOOTER:
      gGCodeFooter = strdup(optarg);
      break;

    case 'D':
      gDebug=1;
      break;

    case 'l':
      gMinSegmentLength = atof(optarg);
      break;

    case 'I':
      gMetricUnits = bool_option(optarg, false);
      gUnitsDefault = 0;
      break;
    case 'M':
      gMetricUnits = bool_option(optarg);
      gUnitsDefault = 0;
      break;

    case 'H':
      gScanLineHorizontal = bool_option(optarg);
      break;
    case 'V':
      gScanLineVertical = bool_option(optarg);
      break;
    case 'G':
      gScanLineZenGarden = bool_option(optarg);
      break;

    case 'd':
      gDrill = bool_option(optarg);
      break;

    case 'v':
      gVerboseFlag = bool_option(optarg);
      break;
    default:
      return false;
      break;
  }

  return true;
}

static void process_config_file_options()
{
  if (!gConfigFilename) {
    gConfigFilename = strdup(DEFAULT_CONFIG_FILENAME);
  }

  if (! (gCfgStream = fopen(gConfigFilename, "r"))) {
    if (strcmp(gConfigFilename, DEFAULT_CONFIG_FILENAME) == 0) {
      return;
    }

    fprintf(stderr, "Can't load configuration: ");
    perror(gOutputFilename);
    exit(1);
  }

  char option_name[64];
  char line[256] = {'\0'};

  while (fgets(line, sizeof(line), gCfgStream)) {

    if (strcmp(line, "\n") == 0) {
      // printf("skipping blank line");
    }

    else if (line[0] == ';') {
      // printf("skipping comment");
    }

    else {
      memset(option_name, 0, sizeof(option_name));

      const char* name_end = strpbrk(line, " \t=");
      const char* value_start = name_end + strspn(name_end, " \t=");
      strncpy(option_name, line, name_end - line);

      const char option_char = lookup_option_by_name(option_name).val;
      set_option(option_char, value_start);
    }
  }

  fclose(gCfgStream);
}

static void process_command_line_options(int argc, char **argv)
{
  extern char *optarg;
  extern int optind, opterr;
  int option_index;

  char ch;

  gFillRadius = -1.0;

  // Check if a custom path for the configuration file was given. If so,
  // those options need to be loaded before applying the command-line
  // options.

  // disable error messages for extra arguments
  //
  opterr = 0;

  while ((ch = getopt_long(argc, argv, "-c:", gLongOption, &option_index)) >= 0) {
    if (ch == 'c') {
      gConfigFilename = strdup(optarg);
      break;
    }
  }

  optind = 0;

  process_config_file_options();

  // Now the cli args and be applied. 'c:' is still included in the
  // argstring so that it won't be defaulted as a bad option.


  // this time extra arguments SHOULD raise errors
  //
  opterr = 1;

  while ((ch = getopt_long(argc, argv, "i:o:c:r:s:z:Z:f:IMHVGvNhCRF:Pl:D", gLongOption, &option_index)) >= 0) {
    switch(ch) {
      case 0:
        // long option
        //
        break;
      case 'N':
        show_version(stdout);
        exit(0);
        break;

      case 'h':
        show_help(stdout);
        exit(0);
        break;

      case 'D':
        gDebug=1;
        break;

      case 'i':
        gInputFilename = strdup(optarg);
        break;
      case 'o':
        gOutputFilename = strdup(optarg);
        break;
      case 'c':
        // Do nothing, but don't go to default!
        break;

      default:
        if (!set_option(ch, optarg)) {
          if (optarg!=NULL) {
            fprintf(stderr, "bad option: -%c %s\n", ch, optarg);
          }
          else {
            fprintf(stderr, "bad option: -%c\n", ch);
          }
          show_help(stderr);
          exit(1);
        }
        break;
    }

  }

  if (!gInputFilename) {
    if (optind < argc) {
      gInputFilename = strdup(argv[optind]);
    }
    else {
      fprintf(stderr, "ERROR: Must provide input file\n");
      show_help(stderr);
      exit(1);
    }
  }

  if (gOutputFilename) {
    if (!(gOutStream = fopen(gOutputFilename, "w"))) {
      perror(gOutputFilename);
      exit(1);
    }
  }

  if (gFillRadius <= 0.0) {
    gFillRadius = gRadius;
  }


  if ( ((gScanLineHorizontal + gScanLineVertical + gScanLineZenGarden)>0) &&
       ((gRadius < eps) && (gFillRadius < eps)) ) {
    fprintf(stderr, "ERROR: Radius (-r) or fill radius (-F) must be specified for fill options (-H, -V or -G)\n");
    show_help(stderr);
    exit(1);
  }

  if ((gSimpleInfill>0) && (gRadius >= eps)) {
    fprintf(stderr, "ERROR: Cannot specify offset radius (-r) for simple infills (-H or -V)\n");
    show_help(stderr);
    exit(1);
  }

  if ((gSimpleInfill>0) && (gScanLineZenGarden>0)) {
    fprintf(stderr, "ERROR: Currently simple infills do not support the zen garden fill pattern, please use -H or -V\n");
    show_help(stderr);
    exit(1);
  }

  if (gVerboseFlag) {
    fprintf(gOutStream, "( radius %f )\n", gRadius);
  }

  if (!gHumanReadable) {
    gShowComments = false;
    gUppercase = true;
  }

  if (gUppercase) {
    char_F = 'F';
    char_G = 'G';
    char_M = 'M';
    char_P = 'P';
    char_S = 'S';
  }
}

static void cleanup(void)
{
  if (gOutStream != stdout) { fclose(gOutStream); }
  if (gOutputFilename)      { free(gOutputFilename); }
  if (gInputFilename)       { free(gInputFilename); }
  if (gConfigFilename)      { free(gConfigFilename); }

  if (gGCodeHeader) { free(gGCodeHeader); }
  if (gGCodeFooter) { free(gGCodeFooter); }
}

static void construct_polygon_offset(Paths *_soln, const Paths &src)
{
  Paths &soln = *_soln;
  ClipperOffset co;

  co.MiterLimit = 3.0;

  co.AddPaths( src, jtMiter, etClosedPolygon);
  co.Execute( soln, g_scalefactor * gRadius );
}

static void find_min_max(IntPoint *minp, IntPoint *maxp, const Paths &src)
{
  int i, j, n, m;

  minp->X = 0;
  minp->Y = 0;
  maxp->X = 0;
  maxp->Y = 0;

  n = src.size();
  for (i=0; i<n; i++) {
    m = src[i].size();
    if ( m == 0 ) continue;
    if (i==0) {
      minp->X = src[i][0].X;
      minp->Y = src[i][0].Y;
      maxp->X = src[i][0].X;
      maxp->Y = src[i][0].Y;
    }

    for (j=0; j<m; j++) {
      if (minp->X > src[i][j].X) minp->X = src[i][j].X;
      if (minp->Y > src[i][j].Y) minp->Y = src[i][j].Y;
      if (maxp->X < src[i][j].X) maxp->X = src[i][j].X;
      if (maxp->Y < src[i][j].Y) maxp->Y = src[i][j].Y;
    }
  }

  minp->X--;
  minp->Y--;
  maxp->X++;
  maxp->Y++;
}


static void do_zen_r(Paths *_paths, const IntPoint &minp, const IntPoint &maxp)
{
  Paths &paths = *_paths;
  static int recur_count=0;
  int i, n, m;
  unsigned int k;
  ClipperOffset co;
  Paths soln;
  Paths tpath;

  IntPoint minpathp, maxpathp;

  // We assume at least an outer boundary.  If only
  // the outer boundary is left, we've finished
  //
  if (paths.size() <= 1) {
    return;
  }

  co.MiterLimit = 3.0;
  recur_count++;
  if(recur_count==400) {
    return;
  }

  if (paths.size() == 0) {
    return;
  }

  co.AddPaths( paths, jtMiter, etClosedPolygon);
  co.Execute( soln, 2.0 * g_scalefactor * gFillRadius );

  do_zen_r(&soln, minp, maxp);

  n = soln.size();
  for (i=0; i<n; i++) {
    m = soln[i].size();
    if (m <= 2 ) { continue; }


    for (k=0; k<soln[i].size(); k++) {
      if ( (soln[i][k].X < minp.X) ||
           (soln[i][k].Y < minp.Y) ||
           (soln[i][k].X > maxp.X) ||
           (soln[i][k].Y > maxp.Y) )
        break;
    }
    if (k<soln[i].size()) { continue; }

    paths.push_back(soln[i]);
  }

}



// extends outwards.  Need to do a final intersect with final (rectangle) polygon
//
static void do_zen(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  static int recur_count=0;
  ClipperOffset co;
  Paths soln;
  Paths tpath;

  IntPoint minp, maxp;

  co.MiterLimit = 3.0;

  recur_count++;
  if(recur_count==400) { return; }

  if (src.size() == 0) { return; }

  find_min_max(&minp, &maxp, src);

  co.AddPaths( src, jtMiter, etClosedPolygon);
  co.Execute( soln, 2.0 * g_scalefactor * gFillRadius );

  do_zen_r(&soln, minp, maxp);

  dst.insert( dst.end(), soln.begin(), soln.end() );
}

static void do_horizontal(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  Paths line_collection;
  cInt h;
  cInt cury;
  IntPoint minp, maxp;

  h = 2.0 * g_scalefactor * gFillRadius;
  h++;

  find_min_max(&minp, &maxp, src);

  cury = minp.Y;

  while ( cury < maxp.Y ) {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( minp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury + h ) );
    line.push_back( IntPoint( minp.X, cury + h ) );

    cury += 2*h;

    clip.AddPath( line, ptSubject, true );
    clip.AddPaths( src, ptClip, true );
    clip.Execute( ctDifference, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );
}

static void do_horizontal_infill(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  Paths line_collection;
  cInt h;
  cInt cury;
  IntPoint minp, maxp;


  h = 2.0 * g_scalefactor * gFillRadius;
  h++;

  find_min_max(&minp, &maxp, src);

  cury = minp.Y;

  while ( cury < maxp.Y ) {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( minp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury + h ) );
    line.push_back( IntPoint( minp.X, cury + h ) );

    cury += 2*h;

    clip.AddPaths( src, ptSubject, true );
    clip.AddPath( line, ptClip, true );
    clip.Execute( ctIntersection, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );

  if (gDrawOutline) {
    dst.insert( dst.end(), src.begin(), src.end());
  }
}


static void do_vertical(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  Paths line_collection;
  cInt w;
  cInt curx;
  IntPoint minp, maxp;

  w = 2.0 * g_scalefactor * gFillRadius ;
  w++;

  find_min_max(&minp, &maxp, src);

  curx = minp.X;
  while ( curx < maxp.X ) {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( curx, minp.Y ) );
    line.push_back( IntPoint( curx, maxp.Y ) );
    line.push_back( IntPoint( curx + w, maxp.Y ) );
    line.push_back( IntPoint( curx + w, minp.Y ) );

    curx += 2*w;

    clip.AddPath( line, ptSubject, true );
    clip.AddPaths( src, ptClip, true );
    clip.Execute( ctDifference, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );
}

static void do_vertical_infill(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  Paths line_collection;
  cInt w;
  cInt curx;
  IntPoint minp, maxp;

  w = 2.0 * g_scalefactor * gFillRadius ;
  w++;

  find_min_max(&minp, &maxp, src);

  curx = minp.X;
  while ( curx < maxp.X ) {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( curx, minp.Y ) );
    line.push_back( IntPoint( curx, maxp.Y ) );
    line.push_back( IntPoint( curx + w, maxp.Y ) );
    line.push_back( IntPoint( curx + w, minp.Y ) );

    curx += 2*w;

    clip.AddPaths( src, ptSubject, true );
    clip.AddPath( line, ptClip, true );
    clip.Execute( ctIntersection, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );

  if (gDrawOutline) {
    dst.insert( dst.end(), src.begin(), src.end());
  }
}

static void invert(Paths *_dst, const Paths &src)
{
  Paths &dst = *_dst;
  unsigned int i;
  ClipperOffset co;
  Clipper clip_stencil;

  Paths stencil;
  Paths oot;
  Path p;

  // Construct outline stencil
  //
  for (i=0; i<src.size(); i++) {
    p = src[i];
    if (Area(p) < 0.0) { std::reverse(p.begin(), p.end()); }
    clip_stencil.AddPath(p, ptSubject, true);
  }
  clip_stencil.Execute(ctUnion, stencil, pftNonZero, pftNonZero);

  for (i=0; i<stencil.size(); i++) {
    dst.push_back(stencil[i]);
  }

  // Reverse all paths for the inversion
  //
  for (i=0; i<src.size(); i++) {
    p = src[i];
    std::reverse(p.begin(), p.end());
    dst.push_back(p);
  }
}

static void setup_aperture_blocks_r(gerber_state_t *gs, int level)
{
  for ( gerber_item_ll_t *item = gs->item_head; item; item = item->next )
  {
    if (item->type == GERBER_AB)
    {
      gApertureBlock[item->d_name] = item->aperture_block;
      setup_aperture_blocks_r(item->aperture_block, level+1);
    }
  }
}

static void setup_aperture_blocks(gerber_state_t *gs)
{
  setup_aperture_blocks_r(gs, 0);
}

int main(int argc, char **argv)
{
  int k, ret;
  gerber_state_t gs;

  Paths pgn_union;

  std::vector< double > xyz;

  //----

  process_command_line_options(argc, argv);

  // Initalize and load gerber file
  //
  gerber_state_init(&gs);

  k = gerber_state_load_file(&gs, gInputFilename);
  if (k < 0) {
    perror(gInputFilename);
    exit(errno);
  }

  if (gDebug) {
    dump_information(&gs, 0);
    exit(1);
  }


  // Construct library of atomic shapes and create polygons
  realize_apertures(&gs);

  // aperture blocks need a lookup, so set that up
  setup_aperture_blocks(&gs);

  // If units haven't been specified on the command line,
  // inherit units from the Gerber file.
  if (gUnitsDefault) {
    gMetricUnits = gs.units_metric;
  }

  if (gMinSegmentLength <= 0.0) {
    gMinSegmentLength = ( gMetricUnits ? gMinSegmentLengthMM : gMinSegmentLengthInch );
  }

  if (gDrill)
    join_drill_set(&gs, &pgn_union);
  else
    join_polygon_set(&gs, &pgn_union);

  if (gFindExtremes) {
    int64_t min_x = INT64_MAX;
    int64_t max_x = INT64_MIN;
    int64_t min_y = INT64_MAX;
    int64_t max_y = INT64_MIN;
    for ( const Path &path : pgn_union )
    {
      for ( const IntPoint &intpoint : path )
      {
        if ( min_x > intpoint.X ) min_x = intpoint.X;
        if ( max_x < intpoint.X ) max_x = intpoint.X;
        if ( min_y > intpoint.Y ) min_y = intpoint.Y;
        if ( max_y < intpoint.Y ) max_y = intpoint.Y;
      }
    }
    printf("%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 "\n", min_x, max_x, min_y, max_y);
    goto the_end;
  }

  if (gShowComments) {
    fprintf( gOutStream, "( segment length %f )\n", gMinSegmentLength );
  }

  if (gShowComments) {
    fprintf( gOutStream, "( union path size %lu )\n", pgn_union.size());
  }


  {
    if (gShowComments) {
      fprintf(gOutStream, "\n");
      fprintf(gOutStream, "( start )\n");
    }
    // G20 - Inch Units
    // G21 - Millimeter Units
    fprintf(gOutStream, "%c%s\n", char_G, (gMetricUnits ? "21" : "20"));
    // G90 - Absolute Positioning
    fprintf(gOutStream, "%c90\n", char_G);
    // G94 - Units per Minute Feed Mode
    fprintf(gOutStream, "%c94\n", char_G);
    // Set Feed Rate
    fprintf(gOutStream, "%c%" GCODE_LENGTH_PRECISION "f\n", char_F, (double)gFeedRate);
    gCurRate = gFeedRate;
    // Move Z up to zsafe
    rapid(gOutStream, "z", gZSafe);
    // M03 - Spindle Clockwise
    fprintf(gOutStream, "%c03", char_M);
    if (gSpindleSpeedSet) {
      // Spindle Speed
      fprintf(gOutStream, " %c%d", char_S, gSpindleSpeed);
    }
    fprintf(gOutStream, "\n");
    // G4 - Dwell
    fprintf(gOutStream, "%c4 %c1\n", char_G, char_P);
  }

  if ((gSimpleInfill>0) && (gFillRadius > eps)) {
    Paths fin_polygons;

    if      ( gScanLineVertical )   { do_vertical_infill(&fin_polygons, pgn_union); }
    else if ( gScanLineHorizontal ) { do_horizontal_infill(&fin_polygons, pgn_union); }
    else { //error
      fprintf(stderr, "unsupported command (for simple-infill)\n");
      exit(1);
    }

    export_paths_to_gcode_unit(gOutStream, fin_polygons, gs.units_metric, gMetricUnits);
  }

  // Offsetting is enabled if the tool radius is specified
  //
  else if ((gRadius > eps) || (gFillRadius > eps)) {
    Paths offset_polygons;

    construct_polygon_offset(&offset_polygons, pgn_union);

    if (gInvertFlag) {
      Paths inverted_polygons;

      if (gShowComments) { fprintf( gOutStream, "( inverted selection radius %f, fill radius %f )\n", gRadius, gFillRadius ); }

      invert(&inverted_polygons, offset_polygons);
      if      ( gScanLineZenGarden )  { do_zen(&offset_polygons, inverted_polygons); }
      else if ( gScanLineVertical )   { do_vertical(&offset_polygons, inverted_polygons); }
      else if ( gScanLineHorizontal ) { do_horizontal(&offset_polygons, inverted_polygons); }

    } else {

      if      ( gScanLineZenGarden )  { do_zen(&offset_polygons, offset_polygons); }
      else if ( gScanLineVertical )   { do_vertical(&offset_polygons, offset_polygons); }
      else if ( gScanLineHorizontal ) { do_horizontal(&offset_polygons, offset_polygons); }

    }

    export_paths_to_gcode_unit(gOutStream, offset_polygons, gs.units_metric, gMetricUnits);
  }
  else {
    ret = export_paths_to_gcode_unit(gOutStream, pgn_union, gs.units_metric, gMetricUnits);
    if (ret < 0) { fprintf(stderr, "got %i\n", ret); }
  }

  {
    if (gShowComments) {
      fprintf(gOutStream, "( end )\n");
    }
    // Move X/Y back to origin
    rapid(gOutStream, "xy", 0, 0);
    // M05 - Spindle Off
    fprintf(gOutStream, "%c05\n", char_M);
  }

the_end:
  cleanup();
  gerber_state_clear( &gs );

  exit(0);
}

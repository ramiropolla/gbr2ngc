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

#ifndef GBL2NGC_HPP
#define GBL2NGC_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <getopt.h>

#include <math.h>

#define GBL2NGC_VERSION "0.8.7"

extern "C" {
  #include "gerber_interpreter.h"
  #include "tesexpr.h"
}


#include <list>
#include <algorithm>
#include <map>

#include <string>
#include <vector>

#include <cmath>

#include "clipper.hpp"
using namespace ClipperLib;

// How many digits after the decimal point?
// Yes, it should be a string :)
#define GCODE_LENGTH_PRECISION "6"

//#define g_scalefactor 1000000.0
#define g_scalefactor 1000000000.0
//#define dtoc(x,y) IntPoint( (cInt)((double)(g_scalefactor*((double)(x)) + 0.5)), (cInt)((double)(g_scalefactor*((double)(y)) + 0.5)) )
#define dtoc(x,y) IntPoint( (cInt)(g_scalefactor*(x)), (cInt)(g_scalefactor*(y)) )
#define ctod(x) (((double)(x))/g_scalefactor)

//----- Class definitions

typedef std::vector< DoublePoint > PathDouble;
typedef std::vector< PathDouble > PathsDouble;

class Aperture_realization {
  public:
    Aperture_realization() { };
    ~Aperture_realization() { };

    int m_name;
    int m_type;

    double m_hole_d;

    std::string m_macro_name;
    std::vector< double > m_macro_param;

    Paths m_macro_path;
    std::vector< int > m_macro_exposure;

    // The paths that are queued to be rendered, additive or subtractive
    // depending on exposure.
    //
    Paths m_path;

    // exposures indicate whether the path in m_path is additive or subtractive
    // 1 - additive
    // 0 - subtractive
    //
    std::vector< int > m_exposure;
};

typedef std::map<int, Aperture_realization> ApertureNameMap;
typedef std::pair<int, Aperture_realization> ApertureNameMapPair;

typedef std::map<int, gerber_state_t *> ApertureBlockMap;

//------------- GLOBALS

extern int gDebug;

extern int gVerboseFlag;
extern int gMetricUnits;
extern int gUnitsDefault;

extern char *gInputFilename;
extern char *gOutputFilename;
extern char *gConfigFilename;
extern char *gGCodeHeader;
extern char *gGCodeFooter;

extern int gFeedRate;
extern bool gFeedRateSet;
extern int gSeekRate;
extern bool gSeekRateSet;
extern int gCurRate;
extern int gSpindleSpeed;
extern bool gSpindleSpeedSet;

extern bool gShowComments;
extern bool gUppercase;
extern bool gHumanReadable;

extern char char_F;
extern char char_G;
extern char char_M;
extern char char_P;
extern char char_S;

extern int gScanLineVertical;
extern int gScanLineHorizontal;
extern int gScanLineZenGarden;

extern double gZSafe;
extern double gZCut;

extern bool gFindExtremes;
extern int64_t gOffsetX;
extern int64_t gOffsetY;
extern double gScaleX;
extern double gScaleY;

extern FILE *gOutStream;
extern FILE *gInpStream;
extern FILE *gCfgStream;

extern double eps;
extern double gRadius;
extern double gFillRadius;

extern int gInvertFlag;
extern int gSimpleInfill;
extern int gDrawOutline;
extern bool gDrill;

extern int gMinSegment;
extern double gMinSegmentLengthInch;
extern double gMinSegmentLengthMM;
extern double gMinSegmentLength;

extern ApertureNameMap gAperture;

extern ApertureBlockMap gApertureBlock;

extern struct timeval gProfileStart;
extern struct timeval gProfileEnd;

//----- aperture functions

int realize_apertures(gerber_state_t *gs);


//----- construct functions

typedef std::vector< Paths > PathSet;

void print_polygon_set(gerber_state_t *gs);
int join_polygon_set(gerber_state_t *gs, Paths *result);
int join_drill_set(gerber_state_t *gs, Paths *result);

//----- export functions

inline double offsetX(const IntPoint &ip)
{
  return gScaleX * (ip.X - gOffsetX);
}
inline double offsetY(const IntPoint &ip)
{
  return gScaleY * (ip.Y - gOffsetY);
}

void rapid(FILE* file, const char* axes, double one, double two=0, double three=0);
void cut(FILE* file, const char* axes, double one, double two=0, double three=0);
int export_paths_to_gcode( FILE *ofp, Paths &paths);
int export_paths_to_gcode_unit( FILE *ofp, const Paths &paths, int src_unit_0mm_1in, int dst_unit_0mm_1in, double ds=0.0);

//int _expose_bit(int local_exposure, int global_exposure = 1);
int _expose_bit(int local_exposure, int global_exposure = 1);
//int _expose_bit(int,int);

#endif

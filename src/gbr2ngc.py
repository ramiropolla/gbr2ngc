from optparse import OptionParser
import subprocess
import json

parser = OptionParser()
parser.add_option("-B", "--back", dest="BCu", help="gerber file for B.Cu", metavar="FILE")
parser.add_option("-F", "--front", dest="FCu", help="gerber file for F.Cu", metavar="FILE")
parser.add_option("-P", "--pth", dest="PTH", help="gerber file for PTH", metavar="FILE")
parser.add_option("-o", "--out", dest="out", help="output directory", metavar="DIR")

# parse options
(options, args) = parser.parse_args()

# options consistency check
if not options.out:
  parser.error("output directory required")
if not (options.BCu or options.FCu):
  parser.error("BCu or FCu required")

GBR2NGC = "./gbr2ngc"
GBR2NGC_FLAGS = "--spindle-speed 9000 --zsafe=0.03937 --inches --no-comment --uppercase"

# get extremes
args = GBR2NGC_FLAGS.split() + [ "--find-extremes" ]
if options.BCu:
  result = subprocess.run([ GBR2NGC ] + args + [ options.BCu ], stdout=subprocess.PIPE)
else:
  result = subprocess.run([ GBR2NGC ] + args + [ options.FCu ], stdout=subprocess.PIPE)
extremes = json.loads(result.stdout)
offset_x = extremes["min_x"]
offset_y = extremes["min_y"]
GBR2NGC_FLAGS += f" --offset-x={offset_x} --offset-y={offset_y}"

if options.BCu:
  args = GBR2NGC_FLAGS.split() + "--feed=1 --scale-x=-1 --zcut=-0.04 --cutout=0.1".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.BCu ] + [ "-o", f"{options.out}/cutout_back_04.gcode" ], stdout=subprocess.PIPE)
  args = GBR2NGC_FLAGS.split() + "--feed=1 --scale-x=-1 --zcut=-0.08 --cutout=0.1".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.BCu ] + [ "-o", f"{options.out}/cutout_back_08.gcode" ], stdout=subprocess.PIPE)
else:
  args = GBR2NGC_FLAGS.split() + "--feed=1 --scale-x=-1 --scale-y=-1 --zcut=-0.04 --cutout=0.1".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.FCu ] + [ "-o", f"{options.out}/cutout_front_04.gcode" ], stdout=subprocess.PIPE)
  args = GBR2NGC_FLAGS.split() + "--feed=1 --scale-x=-1 --scale-y=-1 --zcut=-0.08 --cutout=0.1".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.FCu ] + [ "-o", f"{options.out}/cutout_front_08.gcode" ], stdout=subprocess.PIPE)

# back
if options.BCu:
  args = GBR2NGC_FLAGS.split() + "--feed=3 --scale-x=-1 --zcut=-0.02".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.BCu ] + [ "-o", f"{options.out}/back.gcode" ], stdout=subprocess.PIPE)

# front
if options.FCu:
  args = GBR2NGC_FLAGS.split() + "--feed=3 --scale-x=-1 --scale-y=-1 --zcut=-0.02".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.FCu ] + [ "-o", f"{options.out}/front.gcode" ], stdout=subprocess.PIPE)

# drill
if options.PTH:
  # get drill sizes
  args = GBR2NGC_FLAGS.split() + "--feed=3 --slot-rate=0.5 --scale-x=-1 --zcut=-0.1 --find-drill-sizes".split()
  result = subprocess.run([ GBR2NGC ] + args + [ options.PTH ], stdout=subprocess.PIPE)
  drill_sizes = json.loads(result.stdout)
  for drill in drill_sizes:
    args = GBR2NGC_FLAGS.split() + f"--feed=3 --slot-rate=0.5 --scale-x=-1 --zcut=-0.1 --drill={drill}".split()
    fname = f"{options.out}/{drill_sizes[drill]['type']}_{drill}_{round(drill_sizes[drill]['diameter'] * 10):02}.gcode"
    run_args = [ GBR2NGC ] + args + [ options.PTH ] + [ "-o", fname ]
    # print(' '.join(run_args))
    result = subprocess.run(run_args, stdout=subprocess.PIPE)

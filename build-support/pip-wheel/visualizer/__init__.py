# This is a stub module that loads the actual visualizer
# from ns3.visualizer
import sys

try:
    import ns3.visualizer
except ModuleNotFoundError as e:
    print("Install the ns3 package with pip install ns3.", file=sys.stderr)
    exit(-1)

from ns3.visualizer import start, register_plugin, set_bounds, add_initialization_hook

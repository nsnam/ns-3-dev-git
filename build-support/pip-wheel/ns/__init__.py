# This is a stub module that loads the actual ns-3
# bindings from ns3.ns
import sys

try:
    import ns3.ns

    sys.modules["ns"] = ns3.ns
except ModuleNotFoundError as e:
    print("Install the ns3 package with pip install ns3.", file=sys.stderr)
    exit(-1)

# This is a stub module that loads the actual visualizer
# from ns3.visualizer
import importlib
import importlib.abc
import importlib.util
import sys

try:
    import ns3.visualizer
except ModuleNotFoundError as e:
    print("Install the ns3 package with pip install ns3.", file=sys.stderr)
    exit(-1)


class _SubmoduleAlias(importlib.abc.MetaPathFinder, importlib.abc.Loader):
    """Import visualizer.X as the very same module object as ns3.visualizer.X.

    Loading the files again under the visualizer.X name would create a second
    copy of each module, with its own visualizer state.
    """

    def find_spec(self, name, path, target=None):
        if name.startswith("visualizer."):
            return importlib.util.spec_from_loader(name, self)
        return None

    def create_module(self, spec):
        return importlib.import_module("ns3." + spec.name)

    def exec_module(self, module):
        pass


sys.meta_path.insert(0, _SubmoduleAlias())
sys.modules["visualizer"] = ns3.visualizer

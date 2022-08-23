import builtins
import os.path
import sys
import re


def find_ns3_lock():
    # Get the absolute path to this file
    path_to_this_init_file = os.path.dirname(os.path.abspath(__file__))
    path_to_lock = path_to_this_init_file
    lock_file = (".lock-ns3_%s_build" % sys.platform)

    # Move upwards until we reach the directory with the ns3 script
    prev_path = None
    while "ns3" not in os.listdir(path_to_lock):
        prev_path = path_to_lock
        path_to_lock = os.path.dirname(path_to_lock)
        if prev_path == path_to_lock:
            break

    # We should be now at the directory that contains a lock if the project is configured
    if lock_file in os.listdir(path_to_lock):
        path_to_lock += os.sep + lock_file
    else:
        path_to_lock = None
    return path_to_lock


def load_modules():
    lock_file = find_ns3_lock()

    if lock_file:
        # Load NS3_ENABLED_MODULES from the lock file inside the build directory
        values = {}

        # If we find a lock file, load the ns-3 modules from it
        # Should be the case when running from the source directory
        exec(open(lock_file).read(), {}, values)
        suffix = "-" + values["BUILD_PROFILE"] if values["BUILD_PROFILE"] != "release" else ""
        modules = [module.replace("ns3-", "") for module in values["NS3_ENABLED_MODULES"]]
        prefix = values["out_dir"]
        libraries = {x.split(".")[0]: x for x in os.listdir(os.path.join(prefix, "lib"))}
        version = values["VERSION"]
    else:
        # Otherwise, search for ns-3 libraries
        # Should be the case when ns-3 is installed as a package
        import glob
        env_sep = ";" if sys.platform == "win32" else ":"

        # Search in default directories PATH and LD_LIBRARY_PATH
        library_search_paths = os.getenv("PATH", "").split(env_sep)
        library_search_paths += os.getenv("LD_LIBRARY_PATH", "").split(env_sep)
        if "" in library_search_paths:
            library_search_paths.remove("")
        del env_sep

        # And the current working directory too
        library_search_paths += [os.path.abspath(os.getcwd())]

        # And finally the directories containing this file and its parent directory
        library_search_paths += [os.path.abspath(os.path.dirname(__file__))]
        library_search_paths += [os.path.dirname(library_search_paths[-1])]
        library_search_paths += [os.path.dirname(library_search_paths[-1])]

        # Search for the core library in the search paths
        libraries = []
        for search_path in library_search_paths:
            libraries += glob.glob("%s/**/libns3*" % search_path, recursive=True)
        del search_path, library_search_paths

        if not libraries:
            raise Exception("ns-3 libraries were not found.")

        # The prefix is the directory with the lib directory
        # libns3-dev-core.so/../../
        prefix = os.path.dirname(os.path.dirname(libraries[0]))

        # Extract version and build suffix (if it exists)
        def filter_module_name(library):
            library = os.path.basename(library)
            # Drop extension and libns3.version prefix
            components = ".".join(library.split(".")[:-1]).split("-")[1:]

            # Drop build profile suffix and test libraries
            if components[0] == "dev":
                components.pop(0)
            if components[-1] in ["debug", "default", "optimized", "release", "relwithdebinfo"]:
                components.pop(-1)
            if components[-1] == "test":
                components.pop(-1)
            return "-".join(components)

        # Filter out module names
        modules = set([filter_module_name(library) for library in libraries])

    # Try to import Cppyy and warn the user in case it is not found
    try:
        import cppyy
    except ModuleNotFoundError:
        print("Cppyy is required by the ns-3 python bindings.")
        print("You can install it with the following command: pip install cppyy")
        exit(-1)

    # Enable full logs for debugging
    # cppyy.set_debug(True)

    # Register Ptr<> as a smart pointer
    import libcppyy
    libcppyy.AddSmartPtrType('Ptr')

    # Import ns-3 libraries
    prefix = os.path.abspath(prefix)
    cppyy.add_library_path("%s/lib" % prefix)
    cppyy.add_include_path("%s/include" % prefix)

    for module in modules:
        cppyy.include("ns3/%s-module.h" % module)

    if lock_file:
        # When we have the lock file, we assemble the correct library name
        for module in modules:
            library_name = "libns{version}-{module}{suffix}".format(
                version=version,
                module=module,
                suffix=suffix
            )
            if library_name not in libraries:
                raise Exception("Missing library %s\n" % library_name,
                                "Build all modules with './ns3 build'"
                                )
            cppyy.load_library(libraries[library_name])
    else:
        # When we don't have the lock, we just load the known libraries
        for library in libraries:
            cppyy.load_library(os.path.basename(library))

    # We expose cppyy to consumers of this module as ns.cppyy
    setattr(cppyy.gbl.ns3, "cppyy", cppyy)

    # To maintain compatibility with pybindgen scripts,
    # we set an attribute per module that just redirects to the upper object
    for module in modules:
        setattr(cppyy.gbl.ns3, module.replace("-", "_"), cppyy.gbl.ns3)

    # Set up a few tricks
    cppyy.cppdef("""
        using namespace ns3;
        bool Time_ge(Time& a, Time& b){ return a >= b;}
        bool Time_eq(Time& a, Time& b){ return a == b;}
        bool Time_ne(Time& a, Time& b){ return a != b;}
        bool Time_le(Time& a, Time& b){ return a <= b;}
        bool Time_gt(Time& a, Time& b){ return a > b;}
        bool Time_lt(Time& a, Time& b){ return a < b;}
    """)
    cppyy.gbl.ns3.Time.__ge__ = cppyy.gbl.Time_ge
    cppyy.gbl.ns3.Time.__eq__ = cppyy.gbl.Time_eq
    cppyy.gbl.ns3.Time.__ne__ = cppyy.gbl.Time_ne
    cppyy.gbl.ns3.Time.__le__ = cppyy.gbl.Time_le
    cppyy.gbl.ns3.Time.__gt__ = cppyy.gbl.Time_gt
    cppyy.gbl.ns3.Time.__lt__ = cppyy.gbl.Time_lt

    # Node::~Node isn't supposed to destroy the object,
    # since it gets destroyed at the end of the simulation
    # we need to hold the reference until it gets destroyed by C++
    #
    # Search for NodeList::Add (this)
    cppyy.gbl.ns3.__nodes_pending_deletion = []

    def Node_del(self: cppyy.gbl.ns3.Node) -> None:
        cppyy.gbl.ns3.__nodes_pending_deletion.append(self)
        return None

    cppyy.gbl.ns3.Node.__del__ = Node_del

    # Define ns.cppyy.gbl.addressFromIpv4Address and others
    cppyy.cppdef("""using namespace ns3;
                    Address addressFromIpv4Address(Ipv4Address ip){ return Address(ip); };
                    Address addressFromInetSocketAddress(InetSocketAddress addr){ return Address(addr); };
                    Address addressFromPacketSocketAddress(PacketSocketAddress addr){ return Address(addr); };
                    """)
    # Expose addressFromIpv4Address as a member of the ns3 namespace (equivalent to ns)
    setattr(cppyy.gbl.ns3, "addressFromIpv4Address", cppyy.gbl.addressFromIpv4Address)
    setattr(cppyy.gbl.ns3, "addressFromInetSocketAddress", cppyy.gbl.addressFromInetSocketAddress)
    setattr(cppyy.gbl.ns3, "addressFromPacketSocketAddress", cppyy.gbl.addressFromPacketSocketAddress)
    cppyy.cppdef(
        """using namespace ns3; CommandLine& getCommandLine(std::string filename){ static CommandLine g_cmd = CommandLine(filename); return g_cmd; };""")
    setattr(cppyy.gbl.ns3, "getCommandLine", cppyy.gbl.getCommandLine)
    cppyy.cppdef(
        """using namespace ns3; template Callback<bool, std::string> ns3::MakeNullCallback<bool, std::string>(void);""")
    cppyy.cppdef(
        """using namespace ns3; Callback<bool, std::string> null_callback(){ return MakeNullCallback<bool, std::string>(); };""")
    setattr(cppyy.gbl.ns3, "null_callback", cppyy.gbl.null_callback)

    cppyy.cppdef("""
        using namespace ns3;
        std::tuple<bool, TypeId> LookupByNameFailSafe(std::string name)
        {
            TypeId id;
            bool ok = TypeId::LookupByNameFailSafe(name, &id);
            return std::make_tuple(ok, id);
        }
    """)
    setattr(cppyy.gbl.ns3, "LookupByNameFailSafe", cppyy.gbl.LookupByNameFailSafe)

    def CreateObject(className):
        try:
            try:
                func = "CreateObject%s" % re.sub('[<|>]', '_', className)
                return getattr(cppyy.gbl, func)()
            except AttributeError:
                pass
            try:
                func = "Create%s" % re.sub('[<|>]', '_', className)
                return getattr(cppyy.gbl, func)()
            except AttributeError:
                pass
            raise AttributeError
        except AttributeError:
            try:
                func = "CreateObject%s" % re.sub('[<|>]', '_', className)
                cppyy.cppdef("""
                            using namespace ns3;
                            Ptr<%s> %s(){
                                Ptr<%s> object = CreateObject<%s>();
                                return object;
                            }
                            """ % (className, func, className, className)
                             )
            except Exception as e:
                try:
                    func = "Create%s" % re.sub('[<|>]', '_', className)
                    cppyy.cppdef("""
                                using namespace ns3;
                                %s %s(){
                                    %s object = %s();
                                    return object;
                                }
                                """ % (className, func, className, className)
                                 )
                except Exception as e:
                    exit(-1)
        return getattr(cppyy.gbl, func)()

    setattr(cppyy.gbl.ns3, "CreateObject", CreateObject)

    def GetObject(parentObject, aggregatedObject):
        # Objects have __cpp_name__ attributes, so parentObject
        # should not have it while aggregatedObject can
        if hasattr(parentObject, "__cpp_name__"):
            raise Exception("Class was passed instead of an instance in parentObject")

        aggregatedIsClass = hasattr(aggregatedObject, "__cpp_name__")
        aggregatedIsString = type(aggregatedObject) == str
        aggregatedIsInstance = not aggregatedIsClass and not aggregatedIsString

        if aggregatedIsClass:
            aggregatedType = aggregatedObject.__cpp_name__
        if aggregatedIsInstance:
            aggregatedType = aggregatedObject.__class__.__cpp_name__
        if aggregatedIsString:
            aggregatedType = aggregatedObject

        cppyy.cppdef(
            """using namespace ns3; template <> Ptr<%s> getAggregatedObject<%s>(Ptr<Object> parentPtr, %s param)
               {
                    return parentPtr->GetObject<%s>();
               }
            """ % (aggregatedType, aggregatedType, aggregatedType, aggregatedType)
        )
        return cppyy.gbl.getAggregatedObject(parentObject, aggregatedObject if aggregatedIsClass else aggregatedObject.__class__)

    setattr(cppyy.gbl.ns3, "GetObject", GetObject)
    return cppyy.gbl.ns3


# Load all modules and make them available via a built-in
ns = load_modules()  # can be imported via 'from ns import ns'
builtins.__dict__['ns'] = ns  # or be made widely available with 'from ns import *'

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
    while "ns3" not in os.listdir(path_to_lock):
        path_to_lock = os.path.dirname(path_to_lock)

    # We should be now at the directory that contains a lock if the project is configured
    if lock_file in os.listdir(path_to_lock):
        path_to_lock += os.sep + lock_file
    else:
        raise Exception("ns-3 lock file was not found.\n"
                        "Are you sure %s is inside your ns-3 directory?" % path_to_this_init_file)
    return path_to_lock


def load_modules():
    # Load NS3_ENABLED_MODULES from the lock file inside the build directory
    values = {}

    exec(open(find_ns3_lock()).read(), {}, values)
    suffix = "-" + values["BUILD_PROFILE"] if values["BUILD_PROFILE"] != "release" else ""
    required_modules = [module.replace("ns3-", "") for module in values["NS3_ENABLED_MODULES"]]
    ns3_output_directory = values["out_dir"]
    libraries = {x.split(".")[0]: x for x in os.listdir(os.path.join(ns3_output_directory, "lib"))}

    import cppyy

    # Enable full logs for debugging
    # cppyy.set_debug(True)

    # Register Ptr<> as a smart pointer
    import libcppyy
    libcppyy.AddSmartPtrType('Ptr')

    # Import ns-3 libraries
    cppyy.add_library_path("%s/lib" % ns3_output_directory)
    cppyy.add_include_path("%s/include" % ns3_output_directory)

    for module in required_modules:
        cppyy.include("ns3/%s-module.h" % module)

    for module in required_modules:
        library_name = "libns{version}-{module}{suffix}".format(
            version=values["VERSION"],
            module=module,
            suffix=suffix
        )
        if library_name not in libraries:
            raise Exception("Missing library %s\n" % library_name,
                            "Build all modules with './ns3 build'"
                            )
        cppyy.load_library(libraries[library_name])

    # We expose cppyy to consumers of this module as ns.cppyy
    setattr(cppyy.gbl.ns3, "cppyy", cppyy)

    # To maintain compatibility with pybindgen scripts,
    # we set an attribute per module that just redirects to the upper object
    for module in required_modules:
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

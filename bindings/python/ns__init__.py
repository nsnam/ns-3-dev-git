import builtins
import glob
import os.path
import re
import sys
import sysconfig
from functools import lru_cache

DEFAULT_INCLUDE_DIR = sysconfig.get_config_var("INCLUDEDIR")
DEFAULT_LIB_DIR = sysconfig.get_config_var("LIBDIR")


def find_ns3_lock() -> str:
    # Get the absolute path to this file
    path_to_this_init_file = os.path.dirname(os.path.abspath(__file__))
    path_to_lock = path_to_this_init_file
    lock_file = ".lock-ns3_%s_build" % sys.platform

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
        path_to_lock = ""
    return path_to_lock


SYSTEM_LIBRARY_DIRECTORIES = (
    DEFAULT_LIB_DIR,
    os.path.dirname(DEFAULT_LIB_DIR),
    "/usr/lib64",
    "/usr/lib",
)
DYNAMIC_LIBRARY_EXTENSIONS = {
    "linux": "so",
    "win32": "dll",
    "darwin": "dylib",
}
LIBRARY_EXTENSION = DYNAMIC_LIBRARY_EXTENSIONS[sys.platform]


def trim_library_path(library_path: str) -> str:
    trimmed_library_path = os.path.basename(library_path)

    # Remove lib prefix if it exists and extensions
    trimmed_library_path = trimmed_library_path.split(".")[0]
    if trimmed_library_path[0:3] == "lib":
        trimmed_library_path = trimmed_library_path[3:]
    return trimmed_library_path


@lru_cache(maxsize=None)
def _search_libraries() -> dict:
    # Otherwise, search for ns-3 libraries
    # Should be the case when ns-3 is installed as a package
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

    # Filter unique search paths and those that are not part of system directories
    library_search_paths = list(
        filter(lambda x: x not in SYSTEM_LIBRARY_DIRECTORIES, set(library_search_paths))
    )

    # Exclude injected windows paths in case of WSL
    # BTW, why Microsoft? Who had this brilliant idea?
    library_search_paths = list(filter(lambda x: "/mnt/c/" not in x, library_search_paths))

    # Search for the core library in the search paths
    libraries = []
    for search_path in library_search_paths:
        if os.path.exists(search_path):
            libraries += glob.glob(
                "%s/**/*.%s*" % (search_path, LIBRARY_EXTENSION),
                recursive=not os.path.exists(os.path.join(search_path, "ns3")),
            )

    # Search system library directories (too slow for recursive search)
    for search_path in SYSTEM_LIBRARY_DIRECTORIES:
        if os.path.exists(search_path):
            libraries += glob.glob(
                "%s/**/*.%s*" % (search_path, LIBRARY_EXTENSION), recursive=False
            )
            libraries += glob.glob("%s/*.%s*" % (search_path, LIBRARY_EXTENSION), recursive=False)

    del search_path, library_search_paths

    library_map = {}
    # Organize libraries into a map
    for library in libraries:
        library_infix = trim_library_path(library)

        # Then check if a key already exists
        if library_infix not in library_map:
            library_map[library_infix] = set()

        # Append the directory
        library_map[library_infix].add(os.path.realpath(library))

    # Replace sets with lists
    for key, values in library_map.items():
        library_map[key] = list(values)
    return library_map


def search_libraries(library_name: str) -> list:
    libraries_map = _search_libraries()
    trimmed_library_name = trim_library_path(library_name)
    matched_names = list(filter(lambda x: trimmed_library_name in x, libraries_map.keys()))
    matched_libraries = []

    if matched_names:
        for match in matched_names:
            matched_libraries += libraries_map[match]
    return matched_libraries


LIBRARY_AND_DEFINES = {
    "libgsl": ["HAVE_GSL"],
    "libxml2": ["HAVE_LIBXML2"],
    "libsqlite3": ["HAVE_SQLITE3"],
    "openflow": ["NS3_OPENFLOW", "ENABLE_OPENFLOW"],
    "click": ["NS3_CLICK"],
}


def add_library_defines(library_name: str):
    has_defines = list(filter(lambda x: x in library_name, LIBRARY_AND_DEFINES.keys()))
    defines = ""
    if len(has_defines):
        for define in LIBRARY_AND_DEFINES[has_defines[0]]:
            defines += f"""
                    #ifndef {define}
                    #define {define} 1
                    #endif
                """
    return defines


def extract_linked_libraries(library_name: str, prefix: str) -> tuple:
    lib = ""
    for variant in ["lib", "lib64"]:
        library_path = f"{prefix}/{variant}/{library_name}"
        if os.path.exists(library_path):
            lib = variant
            break
    linked_libs = []
    # First discover which 3rd-party libraries are used by the current module
    try:
        with open(os.path.abspath(library_path), "rb") as f:
            linked_libs = re.findall(
                b"\x00(lib.*?.%b)" % LIBRARY_EXTENSION.encode("utf-8"), f.read()
            )
    except Exception as e:
        print(f"Failed to extract libraries used by {library_path} with exception:{e}")
        exit(-1)
    return library_path, lib, list(map(lambda x: x.decode("utf-8"), linked_libs))


def extract_library_include_dirs(library_name: str, prefix: str) -> tuple:
    library_path, lib, linked_libs = extract_linked_libraries(library_name, prefix)

    linked_libs_include_dirs = set()
    defines = add_library_defines(library_name)

    # Now find these libraries and add a few include paths for them
    for linked_library in linked_libs:
        # Skip ns-3 modules
        if "libns3" in linked_library:
            continue

        # Search for the absolute path of the library
        linked_library_path = search_libraries(linked_library)

        # Raise error in case the library can't be found
        if len(linked_library_path) == 0:
            raise Exception(
                f"Failed to find {linked_library}. Make sure its library directory is in LD_LIBRARY_PATH."
            )

        # Get path with the shortest length
        linked_library_path = sorted(linked_library_path, key=lambda x: len(x))[0]

        # If library is part of the ns-3 build, continue without any new includes
        if prefix in linked_library_path:
            continue

        # Add defines based in linked libraries found
        defines += add_library_defines(linked_library)

        # If it is part of the system directories, try to find it
        system_include_dir = os.path.dirname(linked_library_path).replace(lib, "include")
        if os.path.exists(system_include_dir):
            linked_libs_include_dirs.add(system_include_dir)

            # If system_include_dir/library_name exists, we add it too
            linked_library_name = linked_library.replace("lib", "").replace(
                "." + LIBRARY_EXTENSION, ""
            )
            if os.path.exists(os.path.join(system_include_dir, linked_library_name)):
                linked_libs_include_dirs.add(os.path.join(system_include_dir, linked_library_name))

        # In case it isn't, include new include directories based on the path
        def add_parent_dir_recursively(x: str, y: int) -> None:
            if y <= 0:
                return
            parent_dir = os.path.dirname(x)
            linked_libs_include_dirs.add(parent_dir)
            add_parent_dir_recursively(parent_dir, y - 1)

        add_parent_dir_recursively(linked_library_path, 2)

        for lib_path in list(linked_libs_include_dirs):
            inc_path = os.path.join(lib_path, "include")
            if os.path.exists(inc_path):
                linked_libs_include_dirs.add(inc_path)
    return list(linked_libs_include_dirs), defines


def find_ns3_from_lock_file(lock_file: str) -> (str, list, str):
    # Load NS3_ENABLED_MODULES from the lock file inside the build directory
    values = {}

    # If we find a lock file, load the ns-3 modules from it
    # Should be the case when running from the source directory
    exec(open(lock_file).read(), {}, values)
    suffix = "-" + values["BUILD_PROFILE"] if values["BUILD_PROFILE"] != "release" else ""
    modules = list(
        map(
            lambda x: x.replace("ns3-", ""),
            values["NS3_ENABLED_MODULES"] + values["NS3_ENABLED_CONTRIBUTED_MODULES"],
        )
    )

    prefix = values["out_dir"]
    path_to_lib = None
    for variant in ["lib", "lib64"]:
        path_candidate = os.path.join(prefix, variant)
        if os.path.isdir(path_candidate):
            path_to_lib = path_candidate
            break
    if path_to_lib is None:
        raise Exception(f"Directory {prefix} does not contain subdirectory lib/ (nor lib64/).")
    libraries = {os.path.splitext(os.path.basename(x))[0]: x for x in os.listdir(path_to_lib)}

    version = values["VERSION"]

    # Filter out test libraries and incorrect versions
    def filter_in_matching_ns3_libraries(
        libraries_to_filter: dict,
        modules_to_filter: list,
        version: str,
        suffix: str,
    ) -> dict:
        suffix = [suffix[1:]] if len(suffix) > 1 else []
        filtered_in_modules = []
        for module in modules_to_filter:
            filtered_in_modules += list(
                filter(
                    lambda x: "-".join([version, module, *suffix]) in x, libraries_to_filter.keys()
                )
            )
        for library in list(libraries_to_filter.keys()):
            if library not in filtered_in_modules:
                libraries_to_filter.pop(library)
        return libraries_to_filter

    libraries = filter_in_matching_ns3_libraries(libraries, modules, version, suffix)

    # When we have the lock file, we assemble the correct library names
    libraries_to_load = []
    for module in modules:
        library_name = f"libns{version}-{module}{suffix}"
        if library_name not in libraries:
            raise Exception(
                f"Missing library {library_name}\n", "Build all modules with './ns3 build'"
            )
        libraries_to_load.append(libraries[library_name])
    return prefix, libraries_to_load, version


# Extract version and build suffix (if it exists)
def filter_module_name(library: str) -> str:
    library = os.path.splitext(os.path.basename(library))[0]
    components = library.split("-")

    # Remove version-related prefixes
    if "libns3" in components[0]:
        components.pop(0)
    if "dev" == components[0]:
        components.pop(0)
    if "rc" in components[0]:
        components.pop(0)

    # Drop build profile suffix and test libraries
    if components[-1] in [
        "debug",
        "default",
        "optimized",
        "release",
        "relwithdebinfo",
        "minsizerel",
    ]:
        components.pop(-1)
    return "-".join(components)


def extract_version(library: str, module: str) -> str:
    library = os.path.basename(library)
    return re.findall(r"libns([\d.|rc|\-dev]+)-", library)[0]


def get_newest_version(versions: list) -> str:
    versions = list(sorted(versions))
    if "dev" in versions[0]:
        return versions[0]

    # Check if there is a release of a possible candidate
    try:
        pos = versions.index(os.path.splitext(versions[-1])[0])
    except ValueError:
        pos = None

    # Remove release candidates
    if pos is not None:
        return versions[pos]
    else:
        return versions[-1]


def find_ns3_from_search() -> (str, list, str):
    libraries = search_libraries("ns3")

    if not libraries:
        raise Exception("ns-3 libraries were not found.")

    # If there is a version with a hash by the end, we have a pip-installed library
    pip_libraries = list(filter(lambda x: "python" in x, libraries))
    if pip_libraries:
        # We drop non-pip libraries
        libraries = pip_libraries

    # The prefix is the directory with the lib directory
    # libns3-dev-core.so/../../
    prefix = os.path.dirname(os.path.dirname(libraries[0]))

    # Remove test libraries
    libraries = list(filter(lambda x: "test" not in x, libraries))

    # Filter out module names
    modules = set([filter_module_name(library) for library in libraries])

    def filter_in_newest_ns3_libraries(libraries_to_filter: list, modules_to_filter: list) -> tuple:
        newest_version_found = ""
        # Filter out older ns-3 libraries
        for module in list(modules_to_filter):
            # Filter duplicates of modules, while excluding test libraries
            conflicting_libraries = list(
                filter(lambda x: module == filter_module_name(x), libraries_to_filter)
            )

            # Extract versions from conflicting libraries
            conflicting_libraries_versions = list(
                map(lambda x: extract_version(x, module), conflicting_libraries)
            )

            # Get the newest version found for that library
            newest_version = get_newest_version(conflicting_libraries_versions)

            # Check if the version found is the global newest version
            if not newest_version_found:
                newest_version_found = newest_version
            else:
                newest_version_found = get_newest_version([newest_version, newest_version_found])
                if newest_version != newest_version_found:
                    raise Exception(
                        f"Incompatible versions of the ns-3 module '{module}' were found: {newest_version} != {newest_version_found}."
                    )

            for conflicting_library in list(conflicting_libraries):
                if "-".join([newest_version, module]) not in conflicting_library:
                    libraries.remove(conflicting_library)
                    conflicting_libraries.remove(conflicting_library)

            if len(conflicting_libraries) > 1:
                raise Exception(
                    f"There are multiple build profiles for module '{module}'.\nDelete one to continue: {', '.join(conflicting_libraries)}"
                )

        return libraries_to_filter, newest_version_found

    # Get library base names
    libraries, version = filter_in_newest_ns3_libraries(libraries, list(modules))
    return prefix, libraries, version


def load_modules():
    lock_file = find_ns3_lock()
    libraries_to_load = []

    # Search for prefix to ns-3 build, modules and respective libraries plus version
    ret = find_ns3_from_search() if not lock_file else find_ns3_from_lock_file(lock_file)

    # Unpack returned values
    prefix, libraries, version = ret
    prefix = os.path.abspath(prefix)

    # Sort libraries according to their dependencies
    def sort_to_dependencies(libraries: list, prefix: str) -> list:
        module_dependencies = {}
        libraries = list(map(lambda x: os.path.basename(x), libraries))
        for ns3_library in libraries:
            _, _, linked_libraries = extract_linked_libraries(ns3_library, prefix)
            linked_libraries = list(
                filter(lambda x: "libns3" in x and ns3_library not in x, linked_libraries)
            )
            linked_libraries = list(map(lambda x: os.path.basename(x), linked_libraries))
            module_dependencies[os.path.basename(ns3_library)] = linked_libraries

        def modules_that_can_be_loaded(module_dependencies, pending_modules, current_modules):
            modules = []
            for pending_module in pending_modules:
                can_be_loaded = True
                for dependency in module_dependencies[pending_module]:
                    if dependency not in current_modules:
                        can_be_loaded = False
                        break
                if not can_be_loaded:
                    continue
                modules.append(pending_module)
            return modules

        def dependency_order(
            module_dependencies, pending_modules, current_modules, step_number=0, steps={}
        ):
            if len(pending_modules) == 0:
                return steps
            if step_number not in steps:
                steps[step_number] = []
            for module in modules_that_can_be_loaded(
                module_dependencies, pending_modules, current_modules
            ):
                steps[step_number].append(module)
                pending_modules.remove(module)
                current_modules.append(module)
            return dependency_order(
                module_dependencies, pending_modules, current_modules, step_number + 1, steps
            )

        sorted_libraries = []
        for step in dependency_order(
            module_dependencies, list(module_dependencies.keys()), [], 0
        ).values():
            sorted_libraries.extend(step)
        return sorted_libraries

    libraries_to_load = sort_to_dependencies(libraries, prefix)

    # Extract library base names
    libraries_to_load = [os.path.basename(x) for x in libraries_to_load]

    # Sort modules based on libraries
    modules = list(map(lambda x: filter_module_name(x), libraries_to_load))

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

    libcppyy.AddSmartPtrType("Ptr")

    # Import ns-3 libraries
    for variant in ["lib", "lib64"]:
        path_to_lib = f"{prefix}/{variant}"
        if not os.path.exists(path_to_lib):
            continue
        cppyy.add_library_path(path_to_lib)
    del variant, path_to_lib
    cppyy.add_include_path(f"{prefix}/include")

    known_include_dirs = set()
    # We then need to include all include directories for dependencies
    for library in libraries_to_load:
        linked_lib_include_dirs, defines = extract_library_include_dirs(library, prefix)
        cppyy.cppexec(defines)
        for linked_lib_include_dir in linked_lib_include_dirs:
            if linked_lib_include_dir not in known_include_dirs:
                known_include_dirs.add(linked_lib_include_dir)
                if os.path.isdir(linked_lib_include_dir):
                    cppyy.add_include_path(linked_lib_include_dir)

    for module in modules:
        cppyy.include(f"ns3/{module}-module.h")

    # After including all headers, we finally load the modules
    for library in libraries_to_load:
        cppyy.load_library(library)

    # We expose cppyy to consumers of this module as ns.cppyy
    setattr(cppyy.gbl.ns3, "cppyy", cppyy)

    # Set up a few tricks
    cppyy.cppdef(
        """
        using namespace ns3;
        bool Time_ge(Time& a, Time& b){ return a >= b;}
        bool Time_eq(Time& a, Time& b){ return a == b;}
        bool Time_ne(Time& a, Time& b){ return a != b;}
        bool Time_le(Time& a, Time& b){ return a <= b;}
        bool Time_gt(Time& a, Time& b){ return a > b;}
        bool Time_lt(Time& a, Time& b){ return a < b;}
    """
    )
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

    cppyy.cppdef(
        """
        using namespace ns3;
        std::tuple<bool, TypeId> LookupByNameFailSafe(std::string name)
        {
            TypeId id;
            bool ok = TypeId::LookupByNameFailSafe(name, &id);
            return std::make_tuple(ok, id);
        }
    """
    )
    setattr(cppyy.gbl.ns3, "LookupByNameFailSafe", cppyy.gbl.LookupByNameFailSafe)

    return cppyy.gbl.ns3


# Load all modules and make them available via a built-in
ns = load_modules()  # can be imported via 'from ns import ns'
builtins.__dict__["ns"] = ns  # or be made widely available with 'from ns import *'

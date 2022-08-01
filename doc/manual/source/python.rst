.. include:: replace.txt
.. highlight:: python

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)


Using Python to Run |ns3|
-------------------------

Python bindings allow the C++ code in |ns3| to be called from Python.

This chapter shows you how to create a Python script that can run |ns3| and also the process of creating Python bindings for a C++ |ns3| module.

Introduction
************

Python bindings provide support for importing |ns3| model libraries as Python
modules.  Coverage of most of the |ns3| C++ API is provided.  The intent
has been to allow the programmer to write complete simulation scripts in
Python, to allow integration of |ns3| with other Python tools and workflows.
The intent is not to provide a different language choice to author new
|ns3| models implemented in Python.

Python bindings for |ns3| use a tool called Cppyy (https://cppyy.readthedocs.io/en/latest/)
to create a Python module from the C++ libraries built by CMake. The Python bindings that Cppyy
uses are built at runtime, by importing the C++ libraries and headers for each ns-3 module.
This means that even if the C++ API changes, the Python bindings will adapt to them
without requiring any preprocessing or scanning.

If a user is not interested in Python, no action is needed; the Python bindings
are only built on-demand by Cppyy.

As of ns-3.37, the previous Python bindings framework based on Pybindgen has been
removed due to a lack of active maintainers. The Cppyy frameword that replaced
it may also be removed from future ns-3 releases if new maintainers are not found.

An Example Python Script that Runs |ns3|
****************************************

Here is some example code that is written in Python and that runs |ns3|, which is written in C++.  This Python example can be found in ``examples/tutorial/first.py``:

::

  from ns import ns

  ns.core.LogComponentEnable("UdpEchoClientApplication", ns.core.LOG_LEVEL_INFO)
  ns.core.LogComponentEnable("UdpEchoServerApplication", ns.core.LOG_LEVEL_INFO)

  nodes = ns.network.NodeContainer()
  nodes.Create(2)

  pointToPoint = ns.point_to_point.PointToPointHelper()
  pointToPoint.SetDeviceAttribute("DataRate", ns.core.StringValue("5Mbps"))
  pointToPoint.SetChannelAttribute("Delay", ns.core.StringValue("2ms"))

  devices = pointToPoint.Install(nodes)

  stack = ns.internet.InternetStackHelper()
  stack.Install(nodes)

  address = ns.internet.Ipv4AddressHelper()
  address.SetBase(ns.network.Ipv4Address("10.1.1.0"),
                  ns.network.Ipv4Mask("255.255.255.0"))

  interfaces = address.Assign(devices)

  echoServer = ns.applications.UdpEchoServerHelper(9)

  serverApps = echoServer.Install(nodes.Get(1))
  serverApps.Start(ns.core.Seconds(1.0))
  serverApps.Stop(ns.core.Seconds(10.0))

  address = ns.addressFromIpv4Address(interfaces.GetAddress(1))
  echoClient = ns.applications.UdpEchoClientHelper(address, 9)
  echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(1))
  echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(1.0)))
  echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

  clientApps = echoClient.Install(nodes.Get(0))
  clientApps.Start(ns.core.Seconds(2.0))
  clientApps.Stop(ns.core.Seconds(10.0))

  ns.core.Simulator.Run()
  ns.core.Simulator.Destroy()



Running Python Scripts
**********************

First, we need to enable the build of Python bindings:

.. sourcecode:: bash

  $ ./ns3 configure

Other options such as ``--enable-examples`` may be passed to the above command.
ns3 contains some options that automatically update the python path to find the ns3 module.
To run example programs, there are two ways to use ns3 to take care of this.  One is to run a ns3 shell; e.g.:

.. sourcecode:: bash

  $ ./ns3 shell
  $ python3 examples/wireless/mixed-wireless.py

and the other is to use the 'run' option to ns3:

.. sourcecode:: bash

  $ ./ns3 run examples/wireless/mixed-wireless.py

Use the ``--no-build`` option to run the program without invoking a project rebuild.
This option may be useful to improve execution time when running the same program
repeatedly but with different arguments, such as from scripts.

.. sourcecode:: bash

  $ ./ns3 run --no-build examples/wireless/mixed-wireless.py

To run a python script under the C debugger:

.. sourcecode:: bash

  $ ./ns3 shell
  $ gdb --args python3 examples/wireless/mixed-wireless.py

To run your own Python script that calls |ns3| and that has this path, ``/path/to/your/example/my-script.py``, do the following:

.. sourcecode:: bash

  $ ./ns3 shell
  $ python3 /path/to/your/example/my-script.py

Caveats
*******

Some of the limitations of the Cppyy-based byindings are listed here.

Incomplete Coverage
===================

First of all, keep in mind that not 100% of the API is supported in Python.  Some of the reasons are:

Memory-management issues
########################

Some of the APIs involve pointers, which require knowledge of what kind of memory passing semantics (who owns what memory).
Such knowledge is not part of the function signatures, and is either documented or sometimes not even documented.
You may need to workaround these issues by instantiating variables on the C++ side with a Just-In-Time (JIT) compiled function.

For example, when handling command-line arguments, we could set additional parameters like in the following code:

.. sourcecode:: python

    # Import the ns-3 C++ modules with Cppyy
    from ns import ns

    # ns.cppyy.cppdef compiles the code defined in the block
    # The defined types, values and functions are available in ns.cppyy.gbl
    ns.cppyy.cppdef("""
    using namespace ns3;

    CommandLine& GetCommandLine(std::string filename, int& nCsma, bool& verbose)
    {
        static CommandLine cmd = CommandLine(filename);
        cmd.AddValue("nCsma", "Number of extra CSMA nodes/devices", nCsma);
        cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
        return cmd;
    }
    """)

    # To pass the addresses of the Python variables to c++, we need to use ctypes
    from ctypes import c_int, c_bool
    nCsma = c_int(3)
    verbose = c_bool(True)

    # Pass the addresses of Python variables to C++
    cmd = ns.cppyy.gbl.GetCommandLine(__file__, nCsma, verbose)
    cmd.Parse(sys.argv)

If you find a segmentation violation, be sure to wait for the stacktrace provided by Cppyy
and try to find the root cause of the issue. If you have multiple cores, the number of
stacktraces will correspond to the number of threads being executed by Cppyy. To limit them,
define the environment variable `OPENBLAS_NUM_THREADS=1`.

Operators
#########

Cppyy may fail to map C++ operators due to the implementation style used by ns-3.
This happens for the fundamental type `Time`. To provide the expected behavior, we
redefine these operators from the Python side during the setup of the ns-3 bindings
module (`ns-3-dev/bindings/python/ns__init__.py`).

.. sourcecode:: python

    # Redefine Time operators
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


A different operator used by ns-3 is `operator Address()`, used to
convert different types of Addresses into the generic type Address.
This is not supported by Cppyy and requires explicit conversion.
Some helpers have been added to handle the common cases.

.. sourcecode:: python

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

Most of the missing APIs can be wrapped, given enough time, patience, and expertise, and will likely be wrapped if bug reports are submitted.
However, don't file a bug report saying "bindings are incomplete", because the project does not have maintainers to maintain every API.


Tracing
=======

Callback based tracing is not yet properly supported for Python, as new |ns3| API needs to be provided for this to be supported.

Pcap file writing is supported via the normal API.

ASCII tracing is supported via the normal C++ API translated to Python.
However, ASCII tracing requires the creation of an ostream object to pass into the ASCII tracing methods.
In Python, the C++ std::ofstream has been minimally wrapped to allow this.  For example:

::

    ascii = ns.ofstream("wifi-ap.tr") # create the file
    ns.YansWifiPhyHelper.EnableAsciiAll(ascii)
    ns.Simulator.Run()
    ns.Simulator.Destroy()
    ascii.close() # close the file

There is one caveat: you must not allow the file object to be garbage collected while |ns3| is still using it.
That means that the 'ascii' variable above must not be allowed to go out of scope or else the program will crash.

Working with Python Bindings
****************************

Python bindings are built on a module-by-module basis, and can be found in each module's  ``bindings`` directory.

Overview
========

The python bindings are generated into an 'ns' namespace.  Examples:

::

  from ns import ns
  n1 = ns.network.Node()

or

::

  from ns import*
  n1 = ns.network.Node()

The best way to explore the bindings is to look at the various example
programs provided in |ns3|; some C++ examples have a corresponding Python
example.  There is no structured documentation for the Python bindings
like there is Doxygen for the C++ API, but the Doxygen can be consulted
to understand how the C++ API works.

Historical Information
**********************

If you are a developer and need more background information on |ns3|'s Python bindings,
please see the `Python Bindings wiki page <http://www.nsnam.org/wiki/NS-3_Python_Bindings>`_.
Please note, however, that some information on that page is stale.

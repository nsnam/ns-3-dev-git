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

Python bindings are also needed to run the Pyviz visualizer.

Introduction
************

Python bindings provide support for importing |ns3| model libraries as Python
modules.  Coverage of most of the |ns3| C++ API is provided.  The intent
has been to allow the programmer to write complete simulation scripts in
Python, to allow integration of |ns3| with other Python tools and workflows.
The intent is not to provide a different language choice to author new
|ns3| models implemented in Python.

As of ns-3.37 release or later,
Python bindings for |ns3| use a tool called Cppyy (https://cppyy.readthedocs.io/en/latest/)
to create a Python module from the C++ libraries built by CMake. The Python bindings that Cppyy
uses are built at runtime, by importing the C++ libraries and headers for each |ns3| module.
This means that even if the C++ API changes, the Python bindings will adapt to them
without requiring any preprocessing or scanning.

If a user is not interested in Python, no action is needed; the Python bindings
are only built on-demand by Cppyy, and only if the user enables them in the
configuration of |ns3|.

It is also important to note that the current capability is provided on a
lightly maintained basis and not officially supported by the project
(in other words, we are currently looking for a Python bindings maintainer).
The Cppyy framework could be replaced if it becomes too burdensome to
maintain as we are presently doing.

Prior to ns-3.37, the previous Python bindings framework was based on
`Pybindgen <https://github.com/gjcarneiro/pybindgen>`_.

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

  address = interfaces.GetAddress(1).ConvertTo()
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

For users that want to change upstream modules in C++ and got a copy of
ns-3 by Git cloning the ns-3-dev repository, or downloaded the
ns3-allinone package, or is using bake, continue to the next section.

`Note: models implemented in Python are not available from C++. If you want
your model to be available for both C++ and Python users, you must implement
it in C++.`

For users that want to exclusively run simulation scenarios and implement
simple modules in python, jump to the `Using the pip wheel`_ section.

Using the bindings from the ns-3 source
=======================================

The main prerequisite is to install `cppyy`, with version no later than 2.4.2.
Depending on how you may manage
Python extensions, the installation instructions may vary, but you can first
check if it installed by seeing if the `cppyy` module can be
successfully imported and the version is no later than 2.4.2:

.. sourcecode:: bash

  $ python3
  Python 3.8.10 (default, Jun 22 2022, 20:18:18)
  [GCC 9.4.0] on linux
  Type "help", "copyright", "credits" or "license" for more information.
  >>> import cppyy
  >>> print("%s" % cppyy.__version)
  2.4.2
  >>>

If not, you may try to install via `pip` or whatever other manager you are
using; e.g.:

.. sourcecode:: bash

  $ python3 -m pip install --user cppyy==2.4.2

First, we need to enable the build of Python bindings:

.. sourcecode:: bash

  $ ./ns3 configure --enable-python-bindings

Other options such as ``--enable-examples`` may be passed to the above command.
ns3 contains some options that automatically update the python path to find the ns3 module.
To run example programs, there are two ways to use ns3 to take care of this.  One is to run a ns3 shell; e.g.:

.. sourcecode:: bash

  $ ./ns3 shell
  $ python3 examples/wireless/mixed-wired-wireless.py

and the other is to use the 'run' option to ns3:

.. sourcecode:: bash

  $ ./ns3 run examples/wireless/mixed-wired-wireless.py

Use the ``--no-build`` option to run the program without invoking a project rebuild.
This option may be useful to improve execution time when running the same program
repeatedly but with different arguments, such as from scripts.

.. sourcecode:: bash

  $ ./ns3 run --no-build examples/wireless/mixed-wired-wireless.py

To run a python script under the C debugger:

.. sourcecode:: bash

  $ ./ns3 shell
  $ gdb --args python3 examples/wireless/mixed-wired-wireless.py

To run your own Python script that calls |ns3| and that has this path, ``/path/to/your/example/my-script.py``, do the following:

.. sourcecode:: bash

  $ ./ns3 shell
  $ python3 /path/to/your/example/my-script.py


Using the pip wheel
===================

Starting from ns-3.38, we provide a pip wheel for Python users using Linux.

.. sourcecode:: bash

  $ pip install --user ns3

You can select a specific ns-3 version by specifying the wheel version.
Specifying a nonexistent version will result in an error message listing the available versions.

.. sourcecode:: bash

  $ pip install --user ns3==3.37
  Defaulting to user installation because normal site-packages is not writeable
  ERROR: Could not find a version that satisfies the requirement ns3==3.37 (from versions: 3.37.post415)
  ERROR: No matching distribution found for ns3==3.37

You can also specify you want at least a specific version (e.g. which shipped a required feature).

.. sourcecode:: bash

  $ pip install --user ns3>=3.37
  Defaulting to user installation because normal site-packages is not writeable
  Requirement already satisfied: ns3==3.37.post415 in /home/username/.local/lib/python3.10/site-packages (3.37.post415)
  Requirement already satisfied: cppyy in /home/username/.local/lib/python3.10/site-packages (from ns3==3.37.post415) (2.4.2)
  Requirement already satisfied: cppyy-backend==1.14.10 in /home/username/.local/lib/python3.10/site-packages (from cppyy->ns3==3.37.post415) (1.14.10)
  Requirement already satisfied: CPyCppyy==1.12.12 in /home/username/.local/lib/python3.10/site-packages (from cppyy->ns3==3.37.post415) (1.12.12)
  Requirement already satisfied: cppyy-cling==6.27.1 in /home/username/.local/lib/python3.10/site-packages (from cppyy->ns3==3.37.post415) (6.27.1)

To check if the pip wheel was installed, use the pip freeze command to list the installed packages,
then grep ns3 to filter the line of interest.

.. sourcecode:: bash

  $ pip freeze | grep ns3
  ns3==3.37.post415

.. _ns3 wheel: https://pypi.org/project/ns3/#history

The available versions are also listed on the Pypi page for the `ns3 wheel`_.

After installing it, you can start using ns-3 right away. For example, using the following script.

::

    from ns import ns

    ns.cppyy.cppdef("""
            using namespace ns3;

            Callback<void,Ptr<const Packet>,const Address&,const Address&>
            make_sinktrace_callback(void(*func)(Ptr<Packet>,Address,Address))
            {
                return MakeCallback(func);
            }
        """)

    # Define the trace callback
    def SinkTracer(packet: ns.Packet, src_address: ns.Address, dst_address: ns.Address) -> None:
        print(f"At {ns.Simulator.Now().GetSeconds():.0f}s, '{dst_address}' received packet"
              f" with {packet.__deref__().GetSerializedSize()} bytes from '{src_address}'")

    # Create two nodes
    csmaNodes = ns.network.NodeContainer()
    csmaNodes.Create(2)

    # Connect the two nodes
    csma = ns.csma.CsmaHelper()
    csma.SetChannelAttribute("DataRate", ns.core.StringValue("100Mbps"))
    csma.SetChannelAttribute("Delay", ns.core.TimeValue(ns.core.NanoSeconds(6560)))
    csmaDevices = csma.Install(csmaNodes)

    # Install the internet stack
    stack = ns.internet.InternetStackHelper()
    stack.Install(csmaNodes)

    # Assign Ipv4 addresses
    address = ns.internet.Ipv4AddressHelper()
    address.SetBase(ns.network.Ipv4Address("10.1.2.0"), ns.network.Ipv4Mask("255.255.255.0"))
    csmaInterfaces = address.Assign(csmaDevices)

    # Setup applications
    echoServer = ns.applications.UdpEchoServerHelper(9)

    serverApps = echoServer.Install(csmaNodes.Get(0))
    serverApps.Start(ns.core.Seconds(1.0))
    serverApps.Stop(ns.core.Seconds(10.0))

    echoClient = ns.applications.UdpEchoClientHelper(csmaInterfaces.GetAddress(0).ConvertTo(), 9)
    echoClient.SetAttribute("MaxPackets", ns.core.UintegerValue(10))
    echoClient.SetAttribute("Interval", ns.core.TimeValue(ns.core.Seconds(1.0)))
    echoClient.SetAttribute("PacketSize", ns.core.UintegerValue(1024))

    clientApps = echoClient.Install(csmaNodes.Get(1))
    clientApps.Start(ns.core.Seconds(2.0))
    clientApps.Stop(ns.core.Seconds(10.0))

    # Populate routing tables
    ns.internet.Ipv4GlobalRoutingHelper.PopulateRoutingTables()

    # Setup the trace callback
    sinkTraceCallback = ns.cppyy.gbl.make_sinktrace_callback(SinkTracer)
    serverApps.Get(0).__deref__().TraceConnectWithoutContext("RxWithAddresses", sinkTraceCallback);

    # Set the simulation duration to 11 seconds
    ns.Simulator.Stop(ns.Seconds(11))

    # Run the simulator
    ns.Simulator.Run()
    ns.Simulator.Destroy()

Which should print:

.. sourcecode:: bash

  At 2s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 3s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 4s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 5s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 6s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 7s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 8s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'
  At 9s, '04-07-00:00:00:00:09:00:00' received packet with 60 bytes from '04-07-0a:01:02:02:01:c0:00'

Caveats
*******

Some of the limitations of the Cppyy-based bindings are listed here.

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

    # To pass the addresses of the Python variables to c++, we need to use ctypes
    from ctypes import c_bool, c_int, c_double, c_char_p, create_string_buffer
    verbose = c_bool(True)
    nCsma = c_int(3)
    throughputKbps = c_double(3.1415)
    BUFFLEN = 4096
    outputFileBuffer = create_string_buffer(b"default_output_file.xml", BUFFLEN)
    outputFile = c_char_p(outputFileBuffer.raw)

    # Cppyy will transform the ctype types into the appropriate reference or raw pointers
    cmd = ns.CommandLine(__file__)
    cmd.AddValue("verbose", "Tell echo applications to log if true", verbose)
    cmd.AddValue("nCsma", "Number of extra CSMA nodes/devices", nCsma)
    cmd.AddValue("throughputKbps", "Throughput of nodes", throughputKbps)
    cmd.AddValue("outputFile", "Output file name", outputFile, BUFFLEN)
    cmd.Parse(sys.argv)

    # Printing values of the different ctypes passed as arguments post parsing
    print("Verbose:", verbose.value)
    print("nCsma:", nCsma.value)
    print("throughputKbps:", throughputKbps.value)
    print("outputFile:", outputFile.value)

Note that the variables are passed as references or raw pointers. Reassigning them on the Python side
(e.g. ``verbose = verbose.value``) can result in the Python garbage collector destroying the object
since its only reference has been overwritten, allowing the garbage collector to reclaim that memory space.
The C++ side will then have a dangling reference to the variable, which can be overwritten with
unexpected values, which can be read later, causing ns-3 to behave erratically due to the memory corruption.

String values are problematic since Python and C++ string lifetimes are handled differently.
To workaround that, we need to use null-terminated C strings (``char*``) to exchange strings between
the bindings and ns-3 module libraries. However, C strings are particularly dangerous, since
overwriting the null-terminator can also result in memory corruption. When passing a C string, remember
to allocate a large buffer and perform bounds checking whenever possible. The CommandLine::AddValue
variant for ``char*`` performs bounds checking and aborts the execution in case the parsed value
does not fit in the buffer. Make sure to pass the complete size of the buffer, including the null terminator.

There is an example below demonstrating how the memory corruption could happen in case there was
no bounds checking in CommandLine::AddValue variant for ``char*``.

.. sourcecode:: python

    from ns import ns
    from ctypes import c_char_p, c_char, create_string_buffer, byref, cast

    # The following buffer represent the memory contents
    # of a program containing two adjacent C strings
    # This could be the result of two subsequent variables
    # on the stack or dynamically allocated
    memoryContents = create_string_buffer(b"SHORT_STRING_CONTENTS\0"+b"DoNotWriteHere_"*5+b"\0")
    lenShortString = len(b"SHORT_STRING_CONTENTS\0")

    # In the next lines, we pick pointers to these two C strings
    shortStringBuffer = cast(byref(memoryContents, 0), c_char_p)
    victimBuffer = cast(byref(memoryContents, lenShortString), c_char_p)

    cmd = ns.core.CommandLine(__file__)
    # in the real implementation, the buffer size of 21+1 bytes containing SHORT_STRING_CONTENTS\0 is passed
    cmd.AddValue("shortString", "", shortStringBuffer)

    print("Memory contents before the memory corruption")
    print("Full Memory contents", memoryContents.raw)
    print("shortStringBuffer contents: ", shortStringBuffer.value)
    print("victimBuffer contents: ", victimBuffer.value)

    # The following block should print to the terminal.
    # Note that the strings are correctly
    # identified due to the null terminator (\x00)
    #
    # Memory contents before the memory corruption
    # Full Memory contents b'SHORT_STRING_CONTENTS\x00DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_\x00\x00'
    # shortStringBuffer size=21, contents: b'SHORT_STRING_CONTENTS'
    # victimBuffer size=75, contents: b'DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_DoNotWriteHere_'

    # Write a very long string to a small buffer of size lenShortString = 22
    cmd.Parse(["python", "--shortString="+("OkToWrite"*lenShortString)[:lenShortString]+"CORRUPTED_"*3])

    print("\n\nMemory contents after the memory corruption")
    print("Full Memory contents", memoryContents.raw)
    print("shortStringBuffer contents: ", shortStringBuffer.value)
    print("victimBuffer contents: ", victimBuffer.value)

    # The following block should print to the terminal.
    #
    # Memory contents after the memory corruption
    # Full Memory contents b'OkToWriteOkToWriteOkToCORRUPTED_CORRUPTED_CORRUPTED_\x00oNotWriteHere_DoNotWriteHere_DoNotWriteHere_\x00\x00'
    # shortStringBuffer size=52, contents: b'OkToWriteOkToWriteOkToCORRUPTED_CORRUPTED_CORRUPTED_'
    # victimBuffer size=30, contents: b'CORRUPTED_CORRUPTED_CORRUPTED_'
    #
    # Note that shortStringBuffer invaded the victimBuffer since the
    # string being written was bigger than the shortStringBuffer.
    #
    # Since no bounds checks were performed, the adjacent memory got
    # overwritten and both buffers are now corrupted.
    #
    # We also have a memory leak of the final block in the memory
    # 'oNotWriteHere_DoNotWriteHere_DoNotWriteHere_\x00\x00', caused
    # by the null terminator written at the middle of the victimBuffer.

If you find a segmentation violation, be sure to wait for the stacktrace provided by Cppyy
and try to find the root cause of the issue. If you have multiple cores, the number of
stacktraces will correspond to the number of threads being executed by Cppyy. To limit them,
define the environment variable `OPENBLAS_NUM_THREADS=1`.

Operators
#########

Cppyy may fail to map C++ operators due to the implementation style used by |ns3|.
This happens for the fundamental type `Time`. To provide the expected behavior, we
redefine these operators from the Python side during the setup of the |ns3| bindings
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


A different operator used by |ns3| is `operator Address()`, used to
convert different types of Addresses into the generic type Address.
This is not supported by Cppyy and requires explicit conversion.

.. sourcecode:: python

    # Explicitly convert the InetSocketAddress to Address using InetSocketAddress.ConvertTo()
    sink.Bind(ns.network.InetSocketAddress(ns.network.Ipv4Address.GetAny(), 80).ConvertTo())

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

To inspect what function and classes are available, you can use
the ``dir`` function. Examples below:

.. sourcecode:: bash

  >>> print(dir(ns.Simulator))
  ['Cancel', 'Destroy', 'GetContext', 'GetDelayLeft', 'GetEventCount', 'GetImplementation', 'GetMaximumSimulationTime', 'GetSystemId', 'IsExpired', 'IsFinished', 'NO_CONTEXT', 'Now', 'Remove', 'Run', 'Schedule', 'ScheduleDestroy', 'ScheduleNow', 'ScheduleWithContext', 'SetImplementation', 'SetScheduler', 'Stop', '__add__', '__assign__', '__bool__', '__class__', '__delattr__', '__destruct__', '__dict__', '__dir__', '__dispatch__', '__doc__', '__eq__', '__format__', '__ge__', '__getattribute__', '__getitem__', '__gt__', '__hash__', '__init__', '__init_subclass__', '__invert__', '__le__', '__lt__', '__module__', '__mul__', '__ne__', '__neg__', '__new__', '__pos__', '__python_owns__', '__radd__', '__reduce__', '__reduce_ex__', '__repr__', '__reshape__', '__rmul__', '__rsub__', '__rtruediv__', '__setattr__', '__sizeof__', '__smartptr__', '__str__', '__sub__', '__subclasshook__', '__truediv__', '__weakref__']
  >>> print(dir(ns.DefaultSimulatorImpl))
  ['AggregateObject', 'Cancel', 'Destroy', 'Dispose', 'GetAggregateIterator', 'GetAttribute', 'GetAttributeFailSafe', 'GetContext', 'GetDelayLeft', 'GetEventCount', 'GetInstanceTypeId', 'GetMaximumSimulationTime', 'GetObject', 'GetReferenceCount', 'GetSystemId', 'GetTypeId', 'Initialize', 'IsExpired', 'IsFinished', 'IsInitialized', 'Now', 'PreEventHook', 'Ref', 'Remove', 'Run', 'Schedule', 'ScheduleDestroy', 'ScheduleNow', 'ScheduleWithContext', 'SetAttribute', 'SetAttributeFailSafe', 'SetScheduler', 'Stop', 'TraceConnect', 'TraceConnectWithoutContext', 'TraceDisconnect', 'TraceDisconnectWithoutContext', 'Unref', '__add__', '__assign__', '__bool__', '__class__', '__delattr__', '__destruct__', '__dict__', '__dir__', '__dispatch__', '__doc__', '__eq__', '__format__', '__ge__', '__getattribute__', '__getitem__', '__gt__', '__hash__', '__init__', '__init_subclass__', '__invert__', '__le__', '__lt__', '__module__', '__mul__', '__ne__', '__neg__', '__new__', '__pos__', '__python_owns__', '__radd__', '__reduce__', '__reduce_ex__', '__repr__', '__reshape__', '__rmul__', '__rsub__', '__rtruediv__', '__setattr__', '__sizeof__', '__smartptr__', '__str__', '__sub__', '__subclasshook__', '__truediv__', '__weakref__']
  >>> print(dir(ns.Time))
  ['AUTO', 'As', 'Compare', 'D', 'FS', 'From', 'FromDouble', 'FromInteger', 'GetDays', 'GetDouble', 'GetFemtoSeconds', 'GetHours', 'GetInteger', 'GetMicroSeconds', 'GetMilliSeconds', 'GetMinutes', 'GetNanoSeconds', 'GetPicoSeconds', 'GetResolution', 'GetSeconds', 'GetTimeStep', 'GetYears', 'H', 'IsNegative', 'IsPositive', 'IsStrictlyNegative', 'IsStrictlyPositive', 'IsZero', 'LAST', 'MIN', 'MS', 'Max', 'Min', 'NS', 'PS', 'RoundTo', 'S', 'SetResolution', 'StaticInit', 'To', 'ToDouble', 'ToInteger', 'US', 'Y', '__add__', '__assign__', '__bool__', '__class__', '__delattr__', '__destruct__', '__dict__', '__dir__', '__dispatch__', '__doc__', '__eq__', '__format__', '__ge__', '__getattribute__', '__getitem__', '__gt__', '__hash__', '__init__', '__init_subclass__', '__invert__', '__le__', '__lt__', '__module__', '__mul__', '__ne__', '__neg__', '__new__', '__pos__', '__python_owns__', '__radd__', '__reduce__', '__reduce_ex__', '__repr__', '__reshape__', '__rmul__', '__rsub__', '__rtruediv__', '__setattr__', '__sizeof__', '__smartptr__', '__str__', '__sub__', '__subclasshook__', '__truediv__', '__weakref__']


To get more information about expected arguments, you can use the ``help``
function.

.. sourcecode:: bash

  >>> help(ns.DefaultSimulatorImpl)
  class DefaultSimulatorImpl(SimulatorImpl)
  |  Method resolution order:
  |      DefaultSimulatorImpl
  |      SimulatorImpl
  |      Object
  |      SimpleRefCount<ns3::Object,ns3::ObjectBase,ns3::ObjectDeleter>
  |      ObjectBase
  |      cppyy.gbl.CPPInstance
  |      builtins.object
  |
  |  Methods defined here:
  |
  |  Cancel(...)
  |      void ns3::DefaultSimulatorImpl::Cancel(const ns3::EventId& id)
  |
  |  Destroy(...)
  |      void ns3::DefaultSimulatorImpl::Destroy()
  |
  |  GetContext(...)
  |      unsigned int ns3::DefaultSimulatorImpl::GetContext()
  |
  |  GetDelayLeft(...)
  |      ns3::Time ns3::DefaultSimulatorImpl::GetDelayLeft(const ns3::EventId& id)
  |
  |  GetEventCount(...)
  |      unsigned long ns3::DefaultSimulatorImpl::GetEventCount()
  |
  |  GetMaximumSimulationTime(...)
  |      ns3::Time ns3::DefaultSimulatorImpl::GetMaximumSimulationTime()
  |
  |  GetSystemId(...)
  |      unsigned int ns3::DefaultSimulatorImpl::GetSystemId()
  |
  |  GetTypeId(...)
  |      static ns3::TypeId ns3::DefaultSimulatorImpl::GetTypeId()
  |
  |  IsExpired(...)
  |      bool ns3::DefaultSimulatorImpl::IsExpired(const ns3::EventId& id)
  |
  |  IsFinished(...)
  |      bool ns3::DefaultSimulatorImpl::IsFinished()
  |
  |  Now(...)
  |      ns3::Time ns3::DefaultSimulatorImpl::Now()
  |
  |  Remove(...)
  |      void ns3::DefaultSimulatorImpl::Remove(const ns3::EventId& id)
  |
  |  Run(...)
  |      void ns3::DefaultSimulatorImpl::Run()


Pip wheel packaging
*******************

This section is meant exclusively for ns-3 maintainers and ns-3
users that want to redistribute their work as wheels for python.

The packaging process is defined in the following GitLab job.
The job is split into blocks explained below.

The manylinux image provides an old glibc compatible with most modern Linux
distributions, resulting on a pip wheel that is compatible across distributions.

.. sourcecode:: yaml

  .manylinux-pip-wheel:
    image: quay.io/pypa/manylinux_2_28_x86_64

Then we install the required toolchain and dependencies necessary for both
ns-3 (e.g. libxml2, gsl, sqlite, gtk, etc) and for the bindings and packaging
(e.g. setuptools, wheel, auditwheel, cmake-build-extension, cppyy).

.. sourcecode:: yaml

      # Install minimal toolchain
      - yum install -y libxml2-devel gsl-devel sqlite-devel gtk3-devel boost-devel
      # Create Python venv
      - $PYTHON -m venv ./venv
      - . ./venv/bin/activate
      # Upgrade the pip version to reuse the pre-build cppyy
      - $PYTHON -m pip install pip --upgrade
      - $PYTHON -m pip install setuptools setuptools_scm --upgrade
      - $PYTHON -m pip install wheel auditwheel cmake-build-extension cppyy

The project is then configured loading the configuration settings defined
in the ``ns-3-dev/setup.py`` file.

.. sourcecode:: yaml

      # Configure and build wheel
      - $PYTHON setup.py bdist_wheel build_ext "-DNS3_USE_LIB64=TRUE"

At this point, we have a wheel that only works in the current system,
since external libraries are not shipped.

Auditwheel needs to be called resolve and copy external libraries
that we need to ship along the ns-3 module libraries (e.g. libxml2, sqlite3,
gtk, gsl, etc). However, we need to prevent auditwheel from shipping copies of
the libraries built by the ns-3 project. A list of excluded libraries is generated
by the script ``ns-3-dev/build-support/pip-wheel/auditwheel-exclude-list.py``.

.. sourcecode:: yaml

      - export EXCLUDE_INTERNAL_LIBRARIES=`$PYTHON ./build-support/pip-wheel/auditwheel-exclude-list.py`
      # Bundle in shared libraries that were not explicitly packaged or depended upon
      - $PYTHON -m auditwheel repair ./dist/*whl -L /lib64 $EXCLUDE_INTERNAL_LIBRARIES


At this point, we should have our final wheel ready, but we need to check if it works
before submitting it to Pypi servers.

We first clean the environment and uninstall the packages previously installed.

.. sourcecode:: yaml

      # Clean the build directory
      - $PYTHON ./ns3 clean
      # Clean up the environment
      - deactivate
      - rm -R ./venv
      # Delete toolchain to check if required headers/libraries were really packaged
      - yum remove -y libxml2-devel gsl-devel sqlite-devel gtk3-devel boost-devel


Then we can install our newly built wheel and test it.

.. sourcecode:: yaml

      # Install wheel
      - $PYTHON -m pip install ./wheelhouse/*whl
      - $PYTHON -m pip install matplotlib numpy
      # Test the bindings
      - $PYTHON ./utils/python-unit-tests.py
      - $PYTHON ./examples/realtime/realtime-udp-echo.py
      - $PYTHON ./examples/routing/simple-routing-ping6.py
      - $PYTHON ./examples/tutorial/first.py
      - $PYTHON ./examples/tutorial/second.py
      - $PYTHON ./examples/tutorial/third.py
      - $PYTHON ./examples/wireless/wifi-ap.py
      - $PYTHON ./examples/wireless/mixed-wired-wireless.py
      - $PYTHON ./src/bridge/examples/csma-bridge.py
      - $PYTHON ./src/brite/examples/brite-generic-example.py
      - $PYTHON ./src/core/examples/sample-simulator.py
      - $PYTHON ./src/core/examples/sample-rng-plot.py --not-blocking
      - $PYTHON ./src/click/examples/nsclick-simple-lan.py
      - $PYTHON ./src/flow-monitor/examples/wifi-olsr-flowmon.py
      - $PYTHON ./src/flow-monitor/examples/flowmon-parse-results.py output.xml
      - $PYTHON ./src/openflow/examples/openflow-switch.py

If all programs finish normally, the bindings are working as expected,
and will be saved as an artifact.

.. sourcecode:: yaml

    artifacts:
      paths:
        - wheelhouse/*.whl

One can use ``gitlab-ci-local`` to build the pip wheels locally. After that, the wheels
will be stored in ``.gitlab-ci-local/artifacts/manylinux-pip-wheel-py3Lg10/wheelhouse``
(for Python 3.10).

The wheel names are based on the number of commits since the latest release.
For example, a wheel built 415 after the release 3.37 will be named
``ns3-3.37.post415-cp310-cp310-manylinux_2_28_x86_64.whl``.

The wheel name (``ns3``) is defined in the ``/ns-3-dev/setup.cfg`` file, and that
name should match the build prefix specified in ``/ns-3-dev/setup.py`` file.

The ``cp310-cp310`` indicates that this wheel is compatible from Python 3.10 and up to Python 3.10.

The ``manylinux_2_28`` indicates that this is a manylinux wheel targeting glibc 2.28.

The ``x86_64`` indicates that this is a 64-bit build targeting Intel/AMD processors.

.. _Pypi: https://pypi.org/account/register/
.. _Twine: https://twine.readthedocs.io/en/stable/

After packaging, we can either deploy that wheel locally or upload the wheel to Pypi for general availability.

Local deployment
****************

To deploy a wheel locally, simply share the wheel file across the desired machines.
Then install the wheel and its dependencies running the following command:

.. sourcecode:: bash

    $ pip install *.whl

Publishing the pip wheel via Pypi
*********************************

Publishing a pip wheel requires a `Pypi`_ account.

After creating your account, install `Twine`_, an utility to upload the wheel to Pypi.

Then run twine to upload the wheel to the Pypi servers.

.. sourcecode:: bash

  $ twine upload .gitlab-ci-local/artifacts/manylinux-pip-wheel-py3Lg10/wheelhouse/*.whl

Enter your Pypi username and password as requested.

Your wheel should be up and running. Give it a try just to make sure.

For the upstream pip wheel, try:

.. sourcecode:: bash

  $ pip install ns3
  $ python3 -c "from ns import ns; print(ns.Simulator.Now())"

Historical Information
**********************

If you are a developer and need more background information on |ns3|'s Python bindings,
please see the `Python Bindings wiki page <http://www.nsnam.org/wiki/NS-3_Python_Bindings>`_.
Please note, however, that some information on that page is stale.

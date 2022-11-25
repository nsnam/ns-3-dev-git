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

The main prerequisite is to install `cppyy`.  Depending on how you may manage
Python extensions, the installation instructions may vary, but you can first
check if it installed by seeing if the `cppyy` module can be
successfully imported:

.. sourcecode:: bash

  $ python3
  Python 3.8.10 (default, Jun 22 2022, 20:18:18)
  [GCC 9.4.0] on linux
  Type "help", "copyright", "credits" or "license" for more information.
  >>> import cppyy
  >>>

If not, you may try to install via `pip` or whatever other manager you are
using; e.g.:

.. sourcecode:: bash

  $ python3 -m pip install --user cppyy

First, we need to enable the build of Python bindings:

.. sourcecode:: bash

  $ ./ns3 configure --enable-python-bindings

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

Historical Information
**********************

If you are a developer and need more background information on |ns3|'s Python bindings,
please see the `Python Bindings wiki page <http://www.nsnam.org/wiki/NS-3_Python_Bindings>`_.
Please note, however, that some information on that page is stale.

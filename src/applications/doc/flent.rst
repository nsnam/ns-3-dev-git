.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)

Flent
-----

Flent is a wrapper around netperf (https://hewlettpackard.github.io/netperf/) and similar network benchmarking tools to run predefined tests,
originally authored by T. Høiland-Jørgensen [1]. On running any predefined test, Flent aggregates
results into a gzipped file of JSON format. One may open this file with the help of the Flent GUI to
visualize the aggregated results as plots in a very interactive window.

Inspired by Flent, a Flent application has been implemented in |ns3| as well.

Model Description
*****************

Just as the Flent application on real systems is a wrapper around other traffic benchmarking tools including netperf and ping, the ``FlentApplication`` in |ns3| also wraps
``BulkSendApplication``, and aims to support a subset of Flent's tests by also generating a
JSON trace file that can be read by the Flent GUI. Out of the several tests including RRUL tests,
TCP flow tests, UDP flow tests, VoIP tests, HTTP tests, etc. that Flent supports, the ``FlentApplication``
in |ns3| supports the following tests:

- Ping Test: This test makes RTT measurements between the host and the remote machine with the help of an ICMP Ping.
- RRUL (Realtime Response Under Load) Test: This test saturates the bottleneck link between the host and the remote machine with 4 TCP upload, 4 TCP download flows while measuring RTT with the help of an ICMP Ping and 3 UDP Ping flows with different DiffServ classifications.
- TCP_Upload: This test saturates the bottleneck link between the host and the remote machine with 1 TCP upload flow while measuring RTT with the help of an ICMP Ping.
- TCP_Download: This test saturates the bottleneck link between the host and the remote machine with 1 TCP Download flow while measuring RTT with the help of an ICMP Ping.

Scope and Limitations
=====================

1. ``FlentApplication`` is an orchestrator application. It installs and configures peer applications on the remote node at ``StartApplication`` time, unlike typical |ns3| apps in which the source and sink applications are installed separately by helpers.
2. Because sinks bind to fixed well-known ports (e.g., 9020), two ``FlentApplications`` that target the same host node simultaneously require that the ``HostAddress`` attribute contain ``InetSocketAddress`` instances with different ports (otherwise, socket creation will fail as they try to bind to the same default port).
3. Currently, the ``FlentApplication`` only supports IPv4. IPv6 support will be added in a future extension.

Design
======

The source code for the ``FlentApplication`` is located in the directory ``src/applications/model`` and
consists of 2 files ``flent-application.h`` and ``flent-application.cc`` defining a FlentApplication class.

Although Flent will typically gzip the JSON file, the |ns3| JSON file will be uncompressed and parsable by the Flent GUI. The Flent
GUI is a part of the installation of Flent.

The FlentApplication measures the following metrics:

- RTT: Each RTT sample is calculated as the difference of time between the ICMP Echo Request and (if received) ICMP Echo Reply.
- Throughput: Each throughput sample is calculated by counting the bytes sent by the sender or receiver in each sampling interval (depending upon if the test is upload or download) and excluding any TCP or UDP (or lower layer) header bytes, and dividing by the sampling interval.

In the actual Flent tool, at the start of a test, netperf establishes a TCP control connection
using BSD sockets. This connection initially helps exchange the details of the test before it is started.
A separate connection is made while running the  test. At the end of the test, statistics (like bytes
received, CPU utilization, service demand, time elapsed, CPU rate) about the remote machine are requested
via the control connection by the sender.

The following points provide useful insights about how Flent records metrics:

- Flent reuses the definition and implementation of throughput from netperf. It collects the throughput readings that netperf produces. Netperf is a network benchmark tool used to measure the network performance. Flent requires the user to run netserver on the remote machine and netperf on the local machine.
- The throughput data points are generated on the sender side when the test is running. The sender uses the Linux send function [2] to transmit “send_size” bytes at a time from the TCP socket. When the function returns a value less than send_size, it marks the end of the test. Hence when “send_size” is returned, the sender increments the cumulative bytes sent by send_size every time it sends a message and records data points for plotting the throughput [3].
- By default, netperf sets the socket send_size as equal to the initial socket buffer size. The socket buffer sizes are dynamic during the execution of the test. |ns3| on the other hand does not have dynamic socket buffer sizes [4].
- The TCP MSS used by netperf is read from the kernel.
- It uses socket stats ('ss') to measure congestion window, smooth RTT, pacing rate and delivery rate.
- It uses 'tc' to measure queue delay, queue backlog, drops, and marks.
- Since 'ss' and 'tc' sample data at regular intervals that cannot be less than the execution time of 'ss' and 'tc' itself (which may vary depending upon the hardware). However, |ns3| collects data with the help of writing into trace files via periodic callbacks. These callbacks have practically negligible execution time. Hence, one may expect to see different results for smaller sampling rates while comparing |ns3| and Flent.

In the Flent application we keep the ``MaxSize`` of DropTailQueue as 1p to keep the latency to a minimum value
and further align the results with Flent. We are enabling BQL as well which should control the latency for longer queue lengths.
But we keep the MaxSize as 1p to the latency minimum. This does not mean that BQL is working; it is just that even with BQL enabled
we would get a little bit higher latency that we are getting now. More details available at ``src/network/doc/queue-limits.rst``

Post processing of raw values
=============================

``FlentApplication::FillXValues`` function fills the x_values parameter in Flent file. x_values corresponds to the
x coordinates on the generated plot. We keep the x coordinates based on the step size as we are calculating our results for
all those x coordinates. In this function we incrementally add x_values with the difference of step size

``FlentApplication::ProcessRawValues`` process the raw values and calculates the y coordinate value for the points in x_values.
For each step we find the interpolated measurement value at time t (x_values point) by interpolating between the nearest measurements before
and after t. For the entry we keep the minimum interpolation distance as 0.5 * stepSize to avoid multiple interpolations to the same value.
For the first/last data point we might not have vPrev and vNext so we directly use the values without interpolating.
For all other datapoints we will find the previous and next values; interpolate between them. We assume that the rate of change dv/dt
is constant in the interval, and so can be calculated as (vNext-vPrev)/(tNext-tPrev). Then the value of result at t can be calculated
as v_t=vPrev + dv/dt * (t-tPrev). If the interpolation distance (default max interpolation distance = 5 * stepSize) is too long then
we won't use the value and keep the datapoint as null.

References
==========

[`1 <https://flent.org/intro.html>`_] Flent Documentation.

[`2 <https://man7.org/linux/man-pages/man2/send.2.html>`_] Man Page, Linux Send Function.

[`3 <https://github.com/HewlettPackard/netperf/blob/HEAD/src/nettest_bsd.c#L2212-L2228>`_] Netperf Github Repository.

[`4 <https://github.com/tohojo/flent/blob/HEAD/flent/settings.py#L371-L379>`_] Flent Github Repository.

Usage
*****

FlentApplication is similar to other |ns3| applications in its usage and configuration, with a helper
(FlentHelper) and allowing the user to use attributes to configure values away from defaults. The
``flent-example.cc`` example program may be consulted to see the basic usage of this Flent application.

The configurable parameters of Flent application are as follows:

- TestName: The name of the test to be performed. This shall be one of the predefined tests supported in the Flent application.
- HostAddress: The address of the remote host.
- LocalBindAddress: The address of the local host.
- ImageText: The text to be included in the plot.
- Length: The duration of the test (default is 60 seconds).
- OutputFilename: Path of the .flent output file to write, absolute or relative to the working directory. If empty (the default), the file is written as '<TestName>.flent' in the working directory.
- T0: Absolute start time, as an offset from the Unix epoch, used to anchor all timestamps in the output file. The default corresponds to 2026-01-01T00:00:00Z. Any fixed value keeps the output byte-for-byte reproducible across runs.
- UseWallClockT0: If true, read the system wall clock once at application start and use it in place of the T0 attribute as the timestamp anchor. Output files are then not reproducible across runs.
- StepSize: The sampling rate that Flent application should use.

**Note on Determinism:** By default, FlentApplication uses a fixed epoch anchor (``T0``) to ensure that the generated ``.flent`` output files are byte-for-byte reproducible across identical simulation runs. Users who wish to record the actual system time for comparison with real-world testbed data can set ``UseWallClockT0`` to true, which will override the fixed anchor with the current wall-clock time.

Examples
========

An example demonstrating the usage of Flent application is ``flent-example.cc`` which is present in
``src/applications/examples/``. The topology of this example looks like this:

Client <- - - 5000Mbps,1us - - -> router1 <- - - 50Mbps,5ms - - -> router2 <- - - 5000Mbps,1us - - -> Server

where the bottleneck is between router1 and router2. The default bandwidths are represented in the diagram.
It can be run as follows:

.. code-block:: bash

  ./ns3 run "flent-example"

The parameters for the example are as follows:

- test: Type of |ns3| Flent test
- rtt: Delay value
- bw: Data Rate
- delay: Time to delay test (--delay in Flent)
- verbose: To enable logging during the test. The logs will contain events including the sending and receiving of ICMP pings, UDP Pings and TCP Packets.

The output produced by this example is a '.flent' file that is of JSON format. However, not all keys in the
JSON file have been populated (like various fields in the metadata) and have been left null because they are not
needed for generating plots. Any user may parse this JSON file either as a text output file or view time
series plots with the help of Flent GUI (Since this file is compatible with Flent GUI). To use the Flent GUI,
one may install the Flent application, use its GUI and view time series plots of throughput and RTT.

Visualizing Results
===================

The Flent application can simulate the standard Realtime Response Under Load (RRUL) test to measure network performance, capturing both simultaneous upload and download throughput and latency under heavy load.

.. _fig-ns3-flent-rrul:

.. figure:: figures/ns3-flent-rrul.*
   :figwidth: 15cm
   :align: center

   Figure 1: Throughput and Latency results from a simulated RRUL test.

The figure above demonstrates a classic RRUL test output generated through the |ns3| FlentApplication. The application records the simulation data and formats it into a structure compatible with the standard Flent tools.

To reproduce this figure, first run the included example script to generate the data file:

.. code-block:: bash

   ./ns3 run "flent-example"

Once the simulation completes and outputs the data file in your working directory, you can view and export the plot using the Flent Graphical User Interface (GUI):

1. Open the Flent GUI by running ``flent-gui`` in your terminal.
2. Navigate to ``File`` > ``Open data file`` and select the data file generated by the |ns3| simulation (e.g., ``rrul.flent``).
3. From the plot selection menu (usually a dropdown at the right side of the interface), select the ``all_scaled`` plot, which is the standard visualization for RRUL tests.
4. To export the figure, go to ``File`` > ``Save plot to file`` and save it as a PNG file into your desired directory.

Tests
*****

The Flent application defined in `src/applications/test/flent-application-test-suite.cc` test suite consists of self-contained test cases for each supported test type.

Instead of writing to the current working directory, each test writes a `.flent` file into a runner-managed temporary directory. During its execution, each test runs the full simulation, generates the file, and independently verifies both the file's structural integrity (JSON metadata presence) and its numeric results (throughput bounds and latency limits).

The test suite can be run using the following commands::

  ./ns3 configure --enable-examples --enable-tests
  ./ns3 build
  ./test.py -s flent-application

Validation
==========

The FlentApplication model is tested using :cpp:class:`FlentApplicationTestSuite` class defined in ``src/applications/test/flent-application-test-suite.cc``. The suite includes independent test cases for each supported configuration:

- Test 1: Flent rrul test, checks if the test is running, validates metadata integrity, and checks if the average throughput and average ICMP latency are as expected.
- Test 2: Flent tcp_upload test, checks if the test is running, validates metadata integrity, and checks if the average throughput and average ICMP latency are as expected.
- Test 3: Flent tcp_download test, checks if the test is running, validates metadata integrity, and checks if the average throughput and average ICMP latency are as expected.
- Test 4: Flent ping test, checks if the test is running, validates metadata integrity, and checks if the average ICMP latency is as expected.

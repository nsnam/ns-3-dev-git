#! /usr/bin/env python3
#
# Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
#
"""Tests for the PyViz web visualizer.

Run from the ns-3 root directory with the Python bindings enabled:

    ./ns3 run src/visualizer/test/pyviz-web-test.py
    # or, with the bindings already on PYTHONPATH:
    python3 src/visualizer/test/pyviz-web-test.py [-v] [TestClass[.test_name]]

Each test class starts its own visualizer process from one of the test
scenarios (pyviz-test-scenario.py, pyviz-wireless-scenario.py) and talks to
it over HTTP, exactly as the browser does.

The UiTests class drives the real page with Selenium and headless Chrome or
Chromium. It is skipped when Selenium or a browser is missing, unless
PYVIZ_REQUIRE_SELENIUM=1 is set (as in CI), which turns that into a failure.
Command-line options (or the environment variables in parentheses):
  --require-selenium   (PYVIZ_REQUIRE_SELENIUM=1)  fail instead of skipping
  --chrome-binary      (PYVIZ_CHROME_BINARY)
  --chromedriver       (PYVIZ_CHROMEDRIVER)
  --webdriver-url      (PYVIZ_WEBDRIVER_URL)
  --selenium-debugger  (PYVIZ_SELENIUM_DEBUGGER)
  --help-options       show them; -h shows unittest's own options
Environment variables:
  PYVIZ_CHROME_BINARY      browser executable to use
  PYVIZ_CHROMEDRIVER       chromedriver executable to use
  PYVIZ_SELENIUM_DEBUGGER  host:port of an already running browser started
                           with --remote-debugging-port (instead of launching)
  PYVIZ_WEBDRIVER_URL      URL of a running chromedriver (e.g. on the host when
                           the tests run in a container without browser libraries)
"""

import json
import math
import os
import re
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest
import urllib.error
import urllib.request

## Directory of this test.
TEST_DIR = os.path.dirname(os.path.abspath(__file__))
## Default wired and Wi-Fi test scenario.
SCENARIO = os.path.join(TEST_DIR, "pyviz-test-scenario.py")
## Scenario with several wireless technologies.
WIRELESS_SCENARIO = os.path.join(TEST_DIR, "pyviz-wireless-scenario.py")
## Simulation stop time used by end-of-simulation tests.
STOP_TIME = 6.0  # PYVIZ_TEST_STOP default in the scenario


# ---------------------------------------------------------------------------
# Server fixture
# ---------------------------------------------------------------------------


class Server:
    """A visualizer process running a test scenario."""

    ## @var process
    ## Visualizer subprocess.
    ## @var output
    ## Lines of process output.
    ## @var url
    ## Visualizer URL.
    ## @var token
    ## Session token printed with the URL, required by every API request.
    def __init__(self, scenario=SCENARIO, args=(), env=None):
        """Start the scenario and wait until the visualizer serves its page.

        @param self The object pointer.
        @param scenario Path of the scenario script to run, or the command of a
                        program to run as a list.
        @param args Extra command-line arguments for the scenario.
        @param env Extra environment variables for the process.
        """
        environment = dict(os.environ)
        environment["BROWSER"] = "true"  # webbrowser.open() does nothing
        environment.update(env or {})
        self.process = subprocess.Popen(
            (
                [*scenario, *args]
                if isinstance(scenario, list)
                else [sys.executable, "-u", scenario, *args]
            ),
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            start_new_session=True,
        )
        self.output = []
        self.url = None
        ready = threading.Event()

        def reader():
            """Collect process output and detect the visualizer URL."""
            for line in self.process.stdout:
                self.output.append(line)
                match = re.search(r"PyViz is available at (http://\S+)/#token=(\S+)", line)
                if match and self.url is None:
                    self.token = match.group(2)
                    self.url = match.group(1)
                    ready.set()
            ready.set()

        threading.Thread(target=reader, daemon=True).start()
        # Cppyy parses headers on first use, so starting can take a while.
        ready.wait(timeout=300)
        if self.url is None:
            self.stop()
            missing = re.search(r"PYVIZ_TEST_SKIP: (.*)", "".join(self.output))
            if missing:
                raise unittest.SkipTest(missing.group(1))
            raise RuntimeError("Visualizer did not start:\n" + self.log())

    def log(self):
        """Return the process output without native stack trace noise.

        @param self The object pointer.
        @return The last part of the filtered output.
        """
        # Drop the native stack traces and compiler noise around the message.
        noise = (
            "addr2line",
            " 0x",
            "In file included",
            "Generating stack trace",
            "scalar expression",
        )
        lines = [
            l for l in self.output if not any(n in l for n in noise) and l.strip() not in ("^", ":")
        ]
        return "".join(lines)[-6000:]

    def alive(self):
        """Check whether the process is still running.

        @param self The object pointer.
        @return True while the process runs.
        """
        return self.process.poll() is None

    def stop(self):
        """Terminate the process group and close its output.

        @param self The object pointer.
        """
        if self.process.poll() is None:
            os.killpg(self.process.pid, signal.SIGTERM)
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                os.killpg(self.process.pid, signal.SIGKILL)
                self.process.wait()
        if self.process.stdout:
            self.process.stdout.close()

    # HTTP helpers. Errors come back as (status, payload) instead of raising.

    def request(self, path, body=None, token=True, raw=False, headers=None):
        """Send an HTTP request to the visualizer.

        @param self The object pointer.
        @param path Request path, including any query string.
        @param body Request body (bytes, or an object sent as JSON); None for a GET.
        @param token Whether to send the session token.
        @param raw Whether to return the raw bytes and content type.
        @param headers Extra request headers.
        @return (status, payload), or (status, content, content_type) when raw is set.
        """
        data = (
            None
            if body is None
            else (body if isinstance(body, bytes) else json.dumps(body).encode())
        )
        request = urllib.request.Request(
            self.url.rstrip("/") + path, data=data, headers=headers or {}
        )
        if token and "X-PyViz-Token" not in request.headers:
            request.add_header("X-PyViz-Token", self.token)
        try:
            with urllib.request.urlopen(request, timeout=120) as response:
                content = response.read()
                status, content_type = response.status, response.headers.get("Content-Type", "")
        except urllib.error.HTTPError as error:
            content, status, content_type = (
                error.read(),
                error.code,
                error.headers.get("Content-Type", ""),
            )
        if raw:
            return status, content, content_type
        return status, (
            json.loads(content) if "json" in content_type else content.decode(errors="replace")
        )

    def get(self, path):
        """GET a path and require status 200.

        @param self The object pointer.
        @param path Request path, including any query string.
        @return The decoded payload.
        """
        status, payload = self.request(path)
        assert status == 200, (path, status, payload)
        return payload

    def get_text(self, path):
        """GET a path whatever the status.

        @param self The object pointer.
        @param path Request path, including any query string.
        @return The decoded payload.
        """
        return self.request(path)[1]

    def post(self, path, body):
        """POST a body with the page token.

        @param self The object pointer.
        @param path Request path, including any query string.
        @param body Request body (bytes, or an object sent as JSON); None for a GET.
        @return (status, payload).
        """
        return self.request(path, body)

    def control(self, **body):
        """POST a request to /api/control.

        @param self The object pointer.
        @param body Request fields sent as JSON.
        @return (status, payload).
        """
        return self.post("/api/control", body)

    def state(self):
        """Fetch /api/state, which also advances the simulation.

        @param self The object pointer.
        @return The state dictionary.
        """
        return self.get("/api/state")

    def console(self, code):
        """Run code in the visualizer console.

        @param self The object pointer.
        @param code Python code to run in the visualizer console.
        @return The console output text.
        """
        status, payload = self.post("/api/console", {"code": code})
        assert status == 200, payload
        return payload["output"]

    def run_until(self, condition, timeout=60, interval=0.03):
        """Poll /api/state (which advances the simulation) until condition(state).

        @param self The object pointer.
        @param condition Callable checked until it returns a true value.
        @param timeout Maximum time to wait, in seconds.
        @param interval Delay between polls, in seconds.
        @return The first state satisfying the condition.
        """
        deadline = time.time() + timeout
        state = self.state()
        while not condition(state):
            if time.time() > deadline:
                raise AssertionError("Condition not met; last state time=%s" % state.get("time"))
            time.sleep(interval)
            state = self.state()
        return state

    def run_to(self, simulation_time, timeout=60):
        """Poll the state until a simulation time is reached or the simulation finishes.

        @param self The object pointer.
        @param simulation_time Simulation time to reach, in seconds.
        @param timeout Maximum time to wait, in seconds.
        @return The last state.
        """
        return self.run_until(lambda s: s["time"] >= simulation_time or s["finished"], timeout)


class ServerTestCase(unittest.TestCase):
    """Starts one server for the whole class."""

    ## Scenario script the server runs.
    scenario = SCENARIO
    ## Extra scenario arguments.
    args = ()
    ## Extra environment variables for the scenario.
    env = {}
    ## Whether to start the simulation, which starts paused.
    play = True
    ## Initial simulation speed (0 leaves the default).
    speed = 20.0
    ## Simulation stop time passed to the scenario.
    # Long by default so traffic keeps flowing while a class runs its tests;
    # classes that test the end of the simulation use STOP_TIME.
    stop = 1000.0

    ## @var server
    ## Server shared by the class.
    @classmethod
    def setUpClass(cls):
        """Start the server, set its speed and start the simulation, which starts paused.

        @param cls The test class.
        """
        cls.server = Server(cls.scenario, cls.args, {"PYVIZ_TEST_STOP": str(cls.stop), **cls.env})
        if cls.speed:
            cls.server.control(action="speed", value=cls.speed)
        if cls.play:
            cls.server.control(action="play")

    @classmethod
    def tearDownClass(cls):
        """Stop the server.

        @param cls The test class.
        """
        cls.server.stop()

    def tearDown(self):
        """Check that the visualizer is alive and did not assert.

        @param self The object pointer.
        """
        self.assertTrue(self.server.alive(), "visualizer process died:\n" + self.server.log())
        self.assertNotIn("NS_ASSERT", self.server.log())
        self.assertNotIn("NS_FATAL", self.server.log())


# ---------------------------------------------------------------------------
# HTTP layer
# ---------------------------------------------------------------------------


class HttpTests(ServerTestCase):
    """HTTP layer: assets, tokens and error handling."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    def test_page_has_token_and_assets(self):
        """Test page has token and assets.

        @param self The object pointer.
        """
        status, page, content_type = self.server.request("/", raw=True)
        self.assertEqual(status, 200)
        self.assertIn("text/html", content_type)
        page = page.decode()
        self.assertNotIn(self.server.token, page)
        self.assertIn('href="/favicon.ico"', page)
        self.assertIn('src="/app.js"', page)

    def test_static_assets(self):
        """Test static assets.

        @param self The object pointer.
        """
        for path, kind in (
            ("/app.js", "javascript"),
            ("/style.css", "css"),
            ("/favicon.ico", "image/x-icon"),
            ("/ns-3-logo.png", "image/png"),
        ):
            status, content, content_type = self.server.request(path, raw=True)
            self.assertEqual(status, 200, path)
            self.assertIn(kind, content_type, path)
            self.assertGreater(len(content), 100, path)

    def test_no_external_resources(self):
        # Fully offline: no CDN scripts, fonts or stylesheets.
        """Test no external resources.

        @param self The object pointer.
        """
        for path in ("/", "/app.js", "/style.css"):
            text = self.server.get_text(path)
            urls = [
                u for u in re.findall(r"https?://[^\s'\"]+", text) if "w3.org/2000/svg" not in u
            ]
            self.assertEqual(urls, [], path)

    def test_unknown_paths_and_traversal_are_404(self):
        """Test unknown paths and traversal are 404.

        @param self The object pointer.
        """
        for path in ("/nope", "/../web.py", "/web/app.js", "/%2e%2e/web.py", "/api/unknown"):
            self.assertEqual(self.server.request(path)[0], 404, path)

    def test_post_requires_token(self):
        """Test post requires token.

        @param self The object pointer.
        """
        status, payload = self.server.request("/api/control", {"action": "pause"}, token=False)
        self.assertEqual(status, 403)
        self.assertFalse(payload["ok"])
        status, _ = self.server.request(
            "/api/console", {"code": "1"}, token=False, headers={"X-PyViz-Token": "wrong"}
        )
        self.assertEqual(status, 403)

    def test_api_get_requires_token(self):
        """Test the API GETs need the token, unlike the page and its assets.

        @param self The object pointer.
        """
        for path in ("/api/state", "/api/types", "/api/objects?node=0", "/api/panels"):
            self.assertEqual(self.server.request(path, token=False)[0], 403, path)
        for path in ("/", "/app.js", "/style.css", "/favicon.ico"):
            self.assertEqual(self.server.request(path, token=False, raw=True)[0], 200, path)

    def test_foreign_host_is_rejected(self):
        """Test a request for another host name (DNS rebinding) is rejected.

        @param self The object pointer.
        """
        for path, body in (("/", None), ("/api/console", {"code": "1"})):
            status = self.server.request(path, body, headers={"Host": "attacker.example"})[0]
            self.assertEqual(status, 403, path)

    def test_post_rejects_large_and_malformed_bodies(self):
        """Test post rejects large and malformed bodies.

        @param self The object pointer.
        """
        status, payload = self.server.post("/api/console", b"x" * 70000)
        self.assertEqual(status, 400)
        self.assertIn("too large", payload["error"])
        status, payload = self.server.post("/api/control", b"{not json")
        self.assertEqual(status, 400)
        status, payload = self.server.post("/api/nope", {})
        self.assertEqual(status, 404)

    def test_bad_query_parameters_are_errors_not_crashes(self):
        """Test bad query parameters are errors not crashes.

        @param self The object pointer.
        """
        for path in ("/api/objects?node=abc", "/api/panel?node=0&id=missing", "/api/panels?node=x"):
            status, payload = self.server.request(path)
            self.assertEqual(status, 400, path)
            self.assertFalse(payload["ok"])

    def test_idle_connection_does_not_hang_server(self):
        """Test a connection that never sends a request times out.

        @param self The object pointer.
        """
        import socket

        host, port = self.server.url.split("//")[1].strip("/").split(":")
        connection = socket.create_connection((host, int(port)))
        try:
            started = time.monotonic()
            self.assertEqual(self.server.state()["nodes"][0]["id"], 0)
            self.assertLess(time.monotonic() - started, 15)
        finally:
            connection.close()

    def test_non_ascii_token_is_rejected(self):
        """Test a token header with non-ASCII characters gets a 403.

        @param self The object pointer.
        """
        status, payload = self.server.request(
            "/api/state", token=False, headers={"X-PyViz-Token": "\xff" * 8}
        )
        self.assertEqual(status, 403, payload)
        self.assertNotIn("Traceback", self.server.log())

    def test_dropped_connection_does_not_kill_server(self):
        """Test dropped connection does not kill server.

        @param self The object pointer.
        """
        import socket

        host, port = self.server.url.split("//")[1].strip("/").split(":")
        for _ in range(5):
            connection = socket.create_connection((host, int(port)))
            connection.sendall(b"GET /api/state HTTP/1.0\r\n\r\n")
            connection.close()
        time.sleep(0.5)
        self.assertEqual(self.server.state()["nodes"][0]["id"], 0)


# ---------------------------------------------------------------------------
# Simulation control and pacing
# ---------------------------------------------------------------------------


## Directory of the test browser command and of what it records
BROWSER_DIR = tempfile.mkdtemp(prefix="pyviz-test-")
## File the test browser command writes its argument and what it opened to
BROWSER_RECORD = os.path.join(BROWSER_DIR, "browser.txt")
## Test browser command; webbrowser splits BROWSER at ":", so it is a script
BROWSER_SCRIPT = os.path.join(BROWSER_DIR, "browser.sh")
with open(BROWSER_SCRIPT, "w") as _script:
    _script.write(
        '#!/bin/sh\necho "$1" > %s\ncat "${1#file://}" >> %s\n' % (BROWSER_RECORD, BROWSER_RECORD)
    )
os.chmod(BROWSER_SCRIPT, 0o700)
## Home directory of the scenario, whose download directory gets the page
BROWSER_HOME = os.path.join(BROWSER_DIR, "home")
os.makedirs(os.path.join(BROWSER_HOME, "Downloads"))


class BrowserLaunchTests(ServerTestCase):
    """The browser is opened without the token on its command line."""

    ## Extra environment variables for the scenario: a "browser" recording
    ## its argument and the content of the file it is given.
    env = {"BROWSER": BROWSER_SCRIPT, "HOME": BROWSER_HOME}

    def test_browser_gets_a_private_redirect_page(self):
        """Test the browser argument is a private file redirecting to the page.

        @param self The object pointer.
        """
        for _ in range(100):
            if os.path.exists(BROWSER_RECORD):
                with open(BROWSER_RECORD) as record:
                    if "</a>" in record.read():
                        break
            time.sleep(0.1)
        with open(BROWSER_RECORD) as record:
            argument, page = record.read().split("\n", 1)
        self.assertTrue(argument.startswith("file://"), argument)
        self.assertNotIn(self.server.token, argument)
        self.assertIn("%s/#token=%s" % (self.server.url, self.server.token), page)
        path = argument[len("file://") :]
        self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)
        self.assertEqual(os.stat(os.path.dirname(path)).st_mode & 0o777, 0o700)
        # Where sandboxed (snap, flatpak) browsers can read it.
        downloads = os.path.join(BROWSER_HOME, "Downloads")
        self.assertEqual(os.path.dirname(os.path.dirname(path)), downloads)


class SimulationControlTests(ServerTestCase):
    """Simulation speed, pause, stop and pacing."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    def setUp(self):
        """Resume the simulation at speed 20.

        @param self The object pointer.
        """
        self.server.control(action="play")
        self.server.control(action="speed", value=20)

    def test_default_speed_is_slowest(self):
        """Test default speed is slowest.

        @param self The object pointer.
        """
        fresh = Server()
        try:
            self.assertAlmostEqual(fresh.state()["speed"], 0.01)
        finally:
            fresh.stop()

    def test_time_advances_with_wall_clock(self):
        """Test time advances with wall clock.

        @param self The object pointer.
        """
        self.server.control(action="speed", value=1)
        self.server.state()
        start, wall = self.server.state()["time"], time.time()
        while time.time() - wall < 1.5:  # poll like the page does
            self.server.state()
            time.sleep(0.03)
        advanced = self.server.state()["time"] - start
        self.assertAlmostEqual(advanced, time.time() - wall, delta=0.3)

    def test_advances_between_polls(self):
        """Test the simulation keeps its speed while the page does not poll.

        @param self The object pointer.
        """
        self.server.control(action="speed", value=1)
        start = self.server.state()["time"]
        time.sleep(1.5)
        self.assertGreater(self.server.state()["time"] - start, 1.0)

    def test_speed_scales_and_is_clamped(self):
        """Test speed scales and is clamped.

        @param self The object pointer.
        """
        self.server.control(action="speed", value=0.1)
        self.server.state()
        start = self.server.state()["time"]
        time.sleep(1.0)
        self.assertLess(self.server.state()["time"] - start, 0.3)
        self.server.control(action="speed", value=1000)
        self.assertEqual(self.server.state()["speed"], 100.0)
        self.server.control(action="speed", value=0)
        self.assertEqual(self.server.state()["speed"], 0.01)

    def test_pause_and_stop_freeze_time_play_resumes(self):
        """Test pause and stop freeze time play resumes.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        frozen = self.server.state()["time"]
        time.sleep(0.3)
        state = self.server.state()
        self.assertEqual(state["time"], frozen)
        self.assertFalse(state["running"])
        self.server.control(action="stop")
        state = self.server.state()
        self.assertTrue(state["stopped"])
        self.assertEqual(state["time"], frozen)
        self.server.control(action="play")
        time.sleep(0.2)
        state = self.server.state()
        self.assertTrue(state["running"])
        self.assertFalse(state["stopped"])

    def test_paused_time_is_not_caught_up(self):
        """Test paused time is not caught up.

        @param self The object pointer.
        """
        self.server.control(action="speed", value=1)
        self.server.control(action="pause")
        before = self.server.state()["time"]
        time.sleep(1.0)
        self.server.control(action="play")
        self.server.state()
        self.assertLess(self.server.state()["time"] - before, 0.3)

    def test_smoothing_is_clamped(self):
        """Test smoothing is clamped.

        @param self The object pointer.
        """
        self.server.control(action="smoothing", value=0)
        self.assertEqual(self.server.state()["smoothing"], 1)
        self.server.control(action="smoothing", value=999)
        self.assertEqual(self.server.state()["smoothing"], 50)
        self.server.control(action="smoothing", value=5)

    def test_unknown_actions_are_rejected(self):
        """Test unknown actions are rejected.

        @param self The object pointer.
        """
        status, payload = self.server.control(action="explode")
        self.assertEqual(status, 400)
        status, payload = self.server.control(action="transmissions", value="some")
        self.assertEqual(status, 400)

    def test_pause_message_from_simulation_code(self):
        # PyViz::Pause() is meant to be called from simulation events. The
        # event is scheduled from C++: instantiating Simulator::Schedule from
        # Python with a std::string argument upsets Cppyy's later overload
        # resolution of ns.Seconds().
        """Test pause message from simulation code.

        @param self The object pointer.
        """
        output = self.server.console(
            "ns.cppyy.cppdef('void PyVizTestSchedulePause() { ns3::Simulator::Schedule("
            'ns3::Seconds(0.05), &ns3::visualizer::PyViz::Pause, std::string("checkpoint reached")); }\')\n'
            "ns.cppyy.gbl.PyVizTestSchedulePause()"
        )
        self.assertNotIn("Error", output)
        state = self.server.run_until(lambda s: s["pauseMessages"], timeout=10)
        self.assertIn("checkpoint reached", state["pauseMessages"])
        self.assertFalse(state["running"])
        self.server.control(action="play")
        self.assertEqual(self.server.state()["pauseMessages"], [])

    def test_step_budget_reports_effective_speed(self):
        """Test step budget reports effective speed.

        @param self The object pointer.
        """
        self.server.control(action="speed", value=100)
        state = self.server.run_until(lambda s: s["effectiveSpeed"] is not None, timeout=10)
        # Each step's wall-clock compute is bounded, so a request stays fast
        # even when the simulation cannot keep up.
        start = time.time()
        self.server.state()
        self.assertLess(time.time() - start, 1.0)
        self.assertGreater(state["effectiveSpeed"], 0)


class ReviewFeatureTests(ServerTestCase):
    """Scripted speed and skip, unreceived transmissions and types in use."""

    def test_drop_kinds_are_kept_apart(self):
        """Test each drop cause is reported separately.

        @param self The object pointer.
        """
        output = self.server.console(
            "for kind in ('queue', 'ttl'):\n"
            "    viz.sim_helper.RecordDrop(0, ns.Packet(100), kind)\n"
            "print(sorted(str(d.kind) for d in viz.sim_helper.GetPacketDropSamples()"
            " if d.transmitter.GetId() == 0))"
        )
        self.assertIn("'queue'", output)
        self.assertIn("'ttl'", output)

    def test_set_speed_clamps(self):
        """Test viz.set_speed() and its range.

        @param self The object pointer.
        """
        self.server.console("viz.set_speed(1000)")
        self.assertEqual(self.server.state()["speed"], 100.0)
        self.server.console("viz.set_speed(0.5)")
        self.assertEqual(self.server.state()["speed"], 0.5)

    def test_skip_to(self):
        """Test viz.skip_to() advances without animating what it skipped.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        target = self.server.state()["time"] + 1.5
        self.server.console("viz.skip_to(%r)" % target)
        state = self.server.state()
        self.assertAlmostEqual(state["time"], target, places=6)
        self.assertEqual(state["flows"], [])
        self.server.console("viz.skip_to(0.5)")
        self.assertAlmostEqual(self.server.state()["time"], target, places=6)

    def test_unreceived_excludes_received_senders(self):
        """Test a node whose packets are received has no unreceived entry.

        @param self The object pointer.
        """
        state = self.server.run_until(lambda s: s["transmissions"], timeout=60)
        sources = {t["source"] for t in state["transmissions"]}
        self.assertFalse(sources & {u["node"] for u in state["unreceived"]})

    def test_unreceived_reports_lost_transmission(self):
        """Test a transmission without a reception reports its sender.

        @param self The object pointer.
        """
        output = self.server.console(
            "viz._last_transmissions[:] = []\n"
            "viz._last_node_tx[:] = [(1.0, [(3, 1000)])]\n"
            "print(viz._unreceived([]))\n"
            "viz._unreceived_since[3] = -1.0\n"
            "print(viz._unreceived([]))\n"
            "print(viz._unreceived([{'source': 3}]))"
        )
        self.assertEqual(output.split("\n")[:3], ["[]", "[{'node': 3, 'kbps': 8.0}]", "[]"])

    def test_types_in_use(self):
        """Test only types of the simulation's objects are reported.

        @param self The object pointer.
        """
        names = self.server.get("/api/types-in-use")["types"]
        self.assertIn("ns3::Node", names)
        self.assertIn("ns3::PointToPointNetDevice", names)
        self.assertNotIn("ns3::UanNetDevice", names)


class TourExampleTests(ServerTestCase):
    """The tour example, which shows every cause of packet loss."""

    ## The example under test.
    scenario = os.path.join(TEST_DIR, "..", "examples", "pyviz-tour.py")

    def test_every_cause_is_shown_at_its_node(self):
        """Test each row of the example loses packets for its own reason.

        @param self The object pointer.
        """
        seen = set()

        def collect(state):
            seen.update((e["node"], e["kind"]) for e in state["dropEvents"])
            return {(1, "queue"), (5, "channel"), (7, "ttl"), (10, "other")} <= seen

        state = self.server.run_until(collect, timeout=60)
        self.assertLessEqual({kind for _, kind in seen}, {"queue", "channel", "ttl", "other"})
        state = self.server.run_until(lambda s: s["unreceived"], timeout=60)
        self.assertEqual([u["node"] for u in state["unreceived"]], [12])  # not the mesh

    def test_link_rates_match_the_offered_traffic(self):
        """Test sparse and saturated links show their real rate.

        Each 1000 byte UDP payload is 1028 bytes at the device.

        @param self The object pointer.
        """
        self.server.control(action="smoothing", value=50)  # 5 simulated seconds

        def rate(state, source, target):
            return [
                t["kbps"]
                for t in state["transmissions"]
                if (t["source"], t["target"]) == (source, target)
            ]

        target_time = self.server.state()["time"] + 10
        state = self.server.run_until(lambda s: s["time"] >= target_time, timeout=60)
        for source, target, expected in ((3, 4, 205.6), (6, 7, 41.1), (9, 10, 41.1)):
            self.assertAlmostEqual(rate(state, source, target)[0], expected, delta=expected * 0.1)
        # The 1.1 Mbit/s flow leaves the 1 Mbit/s bottleneck at the link's rate.
        self.assertAlmostEqual(rate(state, 1, 2)[0], 1000, delta=60)

    def test_stations_browse_while_bouncing_in_their_box(self):
        """Test the HTTP server answers the moving stations, which stay in their box.

        @param self The object pointer.
        """
        nodes = {n["label"]: n["id"] for n in self.server.state()["nodes"]}
        server = nodes["HTTP server"]
        self.server.run_until(
            lambda s: any(t["source"] == server for t in s["transmissions"]), timeout=60
        )
        client = nodes["HTTP client 0"]
        start = self.server.state()["nodes"][client]
        state = self.server.run_until(
            lambda s: (s["nodes"][client]["x"], s["nodes"][client]["y"])
            != (start["x"], start["y"]),
            timeout=30,
        )
        self.assertEqual(set(state["selectedBounds"]), {str(client + i) for i in range(4)})
        for index in range(4):
            node = state["nodes"][client + index]
            self.assertTrue(150 <= node["x"] <= 220 and 0 <= node["y"] <= 70, node)


class CommandLineTests(ServerTestCase):
    """The initial speed and time given on the command line."""

    ## Options under test.
    args = ("--PyVizSpeed=2.5", "--PyVizStartTime=3")
    ## Leave the simulation paused.
    play = False
    ## Keep the speed from the command line.
    speed = 0

    def test_speed_and_start_time(self):
        """Test the page starts at the requested time and speed, still paused.

        @param self The object pointer.
        """
        state = self.server.state()
        self.assertEqual(state["speed"], 2.5)
        self.assertAlmostEqual(state["time"], 3.0, places=6)
        self.assertFalse(state["running"])


class StartPausedTests(ServerTestCase):
    """The simulation waits for Play."""

    ## Leave the simulation paused.
    play = False

    def test_starts_paused(self):
        """Test the simulation does not advance until Play is pressed.

        @param self The object pointer.
        """
        for _ in range(5):
            state = self.server.state()
            time.sleep(0.2)
        self.assertFalse(state["running"])
        self.assertEqual(state["time"], 0)
        self.server.control(action="play")
        self.assertGreater(self.server.run_to(1.0)["time"], 0)


class StopTests(ServerTestCase):
    """Finishing at the scenario's stop time."""

    ## Simulation stop time passed to the scenario.
    stop = STOP_TIME

    def test_finishes_exactly_at_simulator_stop(self):
        """Test finishes exactly at simulator stop.

        @param self The object pointer.
        """
        state = self.server.run_until(lambda s: s["finished"], timeout=120)
        self.assertAlmostEqual(state["time"], STOP_TIME, places=6)
        time.sleep(0.2)
        self.assertEqual(self.server.state()["time"], state["time"])


class ExitTests(ServerTestCase):
    """Exit returns to the scenario, which runs its code after Simulator::Run()."""

    ## Simulation stop time passed to the scenario.
    stop = STOP_TIME

    def tearDown(self):
        """Skip the liveness check: the process ends after Exit.

        @param self The object pointer.
        """

    def test_exit_runs_rest_and_code_after_run(self):
        """Test Exit runs the rest of the simulation, then the code after Run.

        @param self The object pointer.
        """
        self.server.run_to(1.0)
        status, payload = self.server.control(action="exit")
        self.assertEqual(status, 200, payload)
        self.assertEqual(self.server.process.wait(timeout=120), 0, self.server.log())
        after = re.search(r"PYVIZ_TEST_AFTER_RUN (\S+)", "".join(self.server.output))
        self.assertIsNotNone(after, self.server.log())
        self.assertAlmostEqual(float(after.group(1)), STOP_TIME, places=6)


class VisualSimulatorImplExitTests(ExitTests):
    """Exit under VisualSimulatorImpl returns from Simulator::Run()."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_MODE": "visual"}


# ---------------------------------------------------------------------------
# Topology, samples and appearance
# ---------------------------------------------------------------------------


class TopologyTests(ServerTestCase):
    """Nodes, links, transmissions and selection."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_HOOKS": "1"}

    ## @var busy
    ## State at a time with traffic.
    @classmethod
    def setUpClass(cls):
        """Start the server and run to a time with traffic.

        @param cls The test class.
        """
        super().setUpClass()
        cls.busy = cls.server.run_to(2.0)

    def test_nodes_and_names(self):
        """Test nodes and names.

        @param self The object pointer.
        """
        nodes = self.busy["nodes"]
        self.assertEqual([n["id"] for n in nodes], list(range(8)))
        self.assertEqual(nodes[0]["name"], "source")

    def test_wired_links_and_csma_hub(self):
        """Test wired links and csma hub.

        @param self The object pointer.
        """
        links = {(l["source"], l["target"]) for l in self.busy["links"]}
        self.assertIn((0, 1), links)
        self.assertIn((3, 4), links)
        self.assertEqual(len(self.busy["hubs"]), 1)
        hub = self.busy["hubs"][0]["id"]
        self.assertEqual({s for s, t in links if t == hub}, {1, 2, 3})
        # Wireless devices do not produce wired links.
        self.assertFalse({5, 6} & {x for link in links for x in link})

    def test_positions_from_mobility_and_layout(self):
        """Test positions from mobility and layout.

        @param self The object pointer.
        """
        nodes = {n["id"]: n for n in self.busy["nodes"]}
        self.assertTrue(all(n["z"] == 0.0 for n in nodes.values()))
        self.assertEqual((nodes[1]["x"], nodes[1]["y"]), (40.0, 0.0))
        self.assertTrue(nodes[1]["mobility"])
        self.assertFalse(nodes[7]["mobility"])
        self.assertTrue(math.isfinite(nodes[7]["x"]) and math.isfinite(nodes[7]["y"]))

    def test_mobile_nodes_move(self):
        """Test mobile nodes move.

        @param self The object pointer.
        """
        before = {n["id"]: (n["x"], n["y"]) for n in self.server.state()["nodes"]}
        after_state = self.server.run_to(self.busy["time"] + 1.5)
        after = {n["id"]: (n["x"], n["y"]) for n in after_state["nodes"]}
        self.assertTrue(before[5] != after[5] or before[6] != after[6])
        self.assertEqual(before[1], after[1])

    def test_wired_and_wireless_transmissions(self):
        """Test wired and wireless transmissions.

        @param self The object pointer.
        """
        state = self.server.run_until(lambda s: len(s["transmissions"]) >= 3, timeout=30)
        pairs = {(t["source"], t["target"]) for t in state["transmissions"]}
        # Echo traffic n6 -> n4 (Wi-Fi) -> n3 (p2p) and back.
        self.assertTrue({(6, 4), (4, 6)} & pairs, pairs)
        self.assertTrue({(4, 3), (3, 4)} & pairs, pairs)
        for t in state["transmissions"]:
            self.assertGreater(t["kbps"], 0)
            self.assertGreaterEqual(t["width"], 0.5)

    def test_drops_reported(self):
        """Test drops reported.

        @param self The object pointer.
        """
        state = self.server.run_until(lambda s: s["drops"], timeout=30)
        self.assertEqual(state["drops"][0]["node"], 1)
        self.assertGreater(state["drops"][0]["kbps"], 0)
        state = self.server.run_until(lambda s: s["dropEvents"], timeout=30)
        event = state["dropEvents"][0]
        self.assertEqual(event["node"], 1)
        self.assertEqual(event["kind"], "other")  # no route
        self.assertGreaterEqual(event["bytes"], event["packets"])
        self.assertGreater(event["packets"], 0)

    def test_packet_flows_are_not_repeated(self):
        """Test packet flows are not repeated.

        @param self The object pointer.
        """
        seen = set()
        for _ in range(20):
            for flow in self.server.state()["flows"]:
                key = (flow["packet"], flow["startTx"], flow["source"], flow["target"])
                self.assertNotIn(key, seen)
                seen.add(key)
                self.assertGreaterEqual(flow["delay"], 0)
                self.assertGreaterEqual(flow["startRx"], flow["startTx"])
            time.sleep(0.05)
        self.assertTrue(seen)

    def test_wifi_association_overlay(self):
        """Test wifi association overlay.

        @param self The object pointer.
        """
        overlays = [o for o in self.busy["overlays"] if o["class"] == "wifi-link"]
        self.assertEqual({(o["source"], o["target"]) for o in overlays}, {(5, 4), (6, 4)})
        self.assertIn("SSID", overlays[0]["title"])

    def test_transmission_modes(self):
        """Test transmission modes.

        @param self The object pointer.
        """
        self.server.control(action="transmissions", value="none")
        time.sleep(0.3)
        self.server.state()
        state = self.server.run_until(lambda s: not s["transmissions"], timeout=10)
        self.assertEqual(state["showTransmissions"], "none")
        self.server.control(action="select", node=6)
        self.server.control(action="transmissions", value="selected")
        state = self.server.run_until(lambda s: s["transmissions"], timeout=20)
        for t in state["transmissions"]:
            self.assertTrue(6 in (t["source"], t["target"]) or t["source"] != 0)
        self.server.control(action="transmissions", value="all")

    def test_selection(self):
        """Test selection.

        @param self The object pointer.
        """
        self.server.control(action="select", node=2)
        state = self.server.state()
        self.assertEqual((state["selected"], state["selectedNodes"]), (2, [2]))
        self.server.control(action="select", node=4, add=True)
        self.server.control(action="select", node=5, add=True)
        state = self.server.state()
        self.assertEqual((state["selected"], state["selectedNodes"]), (5, [2, 4, 5]))
        self.server.control(action="select", node=5, add=True)  # toggles off
        state = self.server.state()
        self.assertEqual((state["selected"], state["selectedNodes"]), (4, [2, 4]))
        self.server.control(action="select", node=None)
        self.assertEqual(self.server.state()["selectedNodes"], [])

    def test_move_bounded_node_is_clamped(self):
        # Node 5 random-walks inside x 110..150, y -30..30; dragging it far
        # outside used to abort the simulation (RandomWalk2dMobilityModel
        # asserts on positions outside its bounds).
        """Test move bounded node is clamped.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        self.server.control(action="move", node=5, x=500.0, y=-400.0)
        node = self.server.state()["nodes"][5]
        self.assertEqual((node["x"], node["y"]), (150.0, -30.0))
        self.server.control(action="play")
        self.server.run_to(self.server.state()["time"] + 0.5)

    def test_selected_node_mobility_bounds(self):
        """Test selected node mobility bounds.

        @param self The object pointer.
        """
        self.server.control(action="select", node=5)
        self.server.control(action="select", node=1, add=True)
        bounds = self.server.state()["selectedBounds"]
        self.assertEqual(bounds, {"5": [110.0, -30.0, 150.0, 30.0]})  # node 1 has no bounds
        self.server.control(action="select", node=None)
        self.assertEqual(self.server.state()["selectedBounds"], {})

    def test_move_nodes(self):
        """Test move nodes.

        @param self The object pointer.
        """
        self.server.control(action="move", node=1, x=12.5, y=-7.0)
        node = self.server.state()["nodes"][1]
        self.assertEqual((node["x"], node["y"]), (12.5, -7.0))
        self.assertEqual(
            self.server.console(
                "ns.NodeList.GetNode(1).GetObject[ns.MobilityModel]().GetPosition().x"
            ).strip(),
            "12.5",
        )
        self.server.control(action="move", node=7, x=3.0, y=4.0)
        node = self.server.state()["nodes"][7]
        self.assertEqual((node["x"], node["y"]), (3.0, 4.0))


class EnergyAndTrailsTests(ServerTestCase):
    ## Initial simulation speed (0 leaves the default).
    # Battery history is sampled once per step (at most every 0.1 s); keep
    # steps short so a few simulated seconds give a useful history.
    """Battery reporting, energy panels and trails."""

    speed = 2.0

    def test_battery_on_nodes_with_energy_source(self):
        """Test battery on nodes with energy source.

        @param self The object pointer.
        """
        early = self.server.run_to(1.0)
        later = self.server.run_to(early["time"] + 5)
        with_battery = [n["id"] for n in later["nodes"] if "battery" in n]
        self.assertEqual(with_battery, [5, 6])
        before, after = early["nodes"][5]["battery"], later["nodes"][5]["battery"]
        self.assertLess(after["fraction"], before["fraction"])
        # 20 J at the default 3 V supply: 20 / 3 / 3.6 mAh.
        self.assertAlmostEqual(after["capacitymAh"], 20 / 3 / 3.6, places=6)
        self.assertAlmostEqual(after["remainingmAh"], after["remainingJ"] / 3 / 3.6, places=6)
        self.assertAlmostEqual(after["fraction"], after["remainingJ"] / 20, places=6)
        lines = dict(self.server.get("/api/tooltip?node=5")["lines"])
        self.assertIn("Battery", lines)
        self.assertIn("mAh", lines["Remaining"])
        self.assertEqual(self.server.get("/api/tooltip?node=0")["lines"], [])

    def test_battery_and_energy_panels(self):
        """Test battery and energy panels.

        @param self The object pointer.
        """
        self.server.run_to(self.server.state()["time"] + 2)
        ids = lambda node: [
            p["id"] for p in self.server.get("/api/panels?node=%d" % node)["panels"]
        ]
        self.assertIn("battery", ids(5))
        self.assertNotIn("battery", ids(0))
        panel = self.server.get("/api/panel?node=5&id=battery")
        sections = {s["title"]: s for s in panel["sections"]}
        charge = sections["Charge"]["plot"]["series"][0]["points"]
        self.assertGreater(len(charge), 5)
        self.assertLess(charge[-1][1], charge[0][1])  # draining
        self.assertTrue(all(b[0] - a[0] >= 0.099 for a, b in zip(charge, charge[1:])))
        drainers = sections["Drainers (device energy models)"]["rows"]
        self.assertEqual([r[2] for r in drainers], ["ns3::WifiRadioEnergyModel"])
        self.assertGreater(drainers[0][3], 0)  # current
        self.assertGreater(drainers[0][4], 0)  # consumed energy
        self.assertEqual(sections["Sources"]["rows"][0][2], "ns3::energy::BasicEnergySource")
        energy = self.server.get("/api/panel?id=energy")
        sections = {s["title"]: s for s in energy["sections"]}
        items = dict(sections["Network"]["items"])
        self.assertEqual(items["Nodes with an energy source"], "2")
        self.assertTrue(items["Energy at the start"].startswith("40.000 J"))  # 2 x 20 J
        self.assertEqual(
            [s["name"] for s in sections["Charge per node"]["plot"]["series"]], ["Node 5", "Node 6"]
        )
        total = sections["Total remaining energy"]["plot"]["series"][0]["points"]
        self.assertLess(total[-1][1], total[0][1])
        self.assertEqual(len(sections["Drainers (device energy models)"]["rows"]), 2)
        self.assertEqual(sections["Charge sources (harvesters)"]["rows"], [])

    def test_trails(self):
        """Test trails.

        @param self The object pointer.
        """
        self.assertFalse(self.server.state()["showTrails"])
        self.assertEqual(self.server.state()["trails"], {})
        self.server.control(action="trails", value=True)
        state = self.server.run_until(lambda s: len(s["trails"].get("5", [])) >= 5, timeout=60)
        self.assertTrue(state["showTrails"])
        self.assertTrue(set(state["trails"]) <= {"5", "6"})  # only nodes that move
        points = state["trails"]["5"]
        steps = [math.hypot(b[0] - a[0], b[1] - a[1]) for a, b in zip(points, points[1:])]
        self.assertTrue(all(step >= 0.5 - 1e-9 for step in steps))
        self.server.control(action="trails", value=False)
        state = self.server.state()
        self.assertFalse(state["showTrails"])
        self.assertEqual(state["trails"], {})


# ---------------------------------------------------------------------------
# Scenario API: hooks, plugins, appearance, bounds
# ---------------------------------------------------------------------------


class ExtensionApiTests(ServerTestCase):
    """Scenario hooks, plugins, appearance and bounds."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_HOOKS": "1"}

    def test_appearance_and_bounds_from_initialization_hook(self):
        """Test appearance and bounds from initialization hook.

        @param self The object pointer.
        """
        state = self.server.state()
        nodes = state["nodes"]
        self.assertEqual(nodes[0]["color"], "#ff00ff")
        self.assertEqual(nodes[1]["label"], "router")
        self.assertEqual(nodes[2]["size"], 2.5)
        self.assertTrue(nodes[3]["icon"])
        self.assertEqual(state["bounds"], [-10.0, -40.0, 160.0, 40.0])

    def test_node_icon_served(self):
        """Test node icon served.

        @param self The object pointer.
        """
        status, content, content_type = self.server.request("/api/icon?node=3", raw=True)
        self.assertEqual(status, 200)
        self.assertIn("svg", content_type)
        self.assertEqual(self.server.request("/api/icon?node=0", raw=True)[0], 404)

    def test_plugin_overlay_tooltip_and_periodic_update(self):
        """Test plugin overlay tooltip and periodic update.

        @param self The object pointer.
        """
        state = self.server.run_to(0.5)
        self.assertIn(
            {"source": 0, "target": 7, "class": "test-overlay", "title": "test"}, state["overlays"]
        )
        self.assertEqual(self.server.get("/api/tooltip?node=2")["lines"], [["Test info", "node 2"]])
        panel = self.server.get("/api/panel?id=test-global")
        self.assertGreater(int(panel["sections"][0]["items"][0][1]), 0)

    def test_node_panel_applies_filter(self):
        """Test node panel applies filter.

        @param self The object pointer.
        """
        ids = lambda node: [
            p["id"] for p in self.server.get("/api/panels?node=%d" % node)["panels"]
        ]
        self.assertIn("test-node", ids(0))
        self.assertNotIn("test-node", ids(1))
        panel = self.server.get("/api/panel?node=0&id=test-node")
        self.assertEqual(panel["title"], "Test node panel - node 0")
        self.assertEqual(panel["sections"][0]["items"], [["node", "0"]])

    def test_step_guard_holds_simulation(self):
        """Test step guard holds simulation.

        @param self The object pointer.
        """
        self.server.post("/api/panel-action", {"id": "test-global", "values": {"hold": True}})
        held = self.server.state()
        self.assertTrue(held["held"])
        time.sleep(0.3)
        self.assertEqual(self.server.state()["time"], held["time"])
        self.server.post("/api/panel-action", {"id": "test-global", "values": {"hold": True}})
        time.sleep(0.2)
        state = self.server.state()
        self.assertFalse(state["held"])
        self.assertGreater(self.server.state()["time"], held["time"])

    def test_global_panel_controls_reach_action(self):
        """Test global panel controls reach action.

        @param self The object pointer.
        """
        self.assertEqual(
            self.server.post("/api/panel-action", {"id": "test-global", "values": {"mode": "b"}})[
                0
            ],
            200,
        )
        panel = self.server.get("/api/panel?id=test-global")
        self.assertIn(["b"], panel["sections"][1]["rows"])
        kinds = {c["name"]: c["kind"] for c in panel["controls"]}
        self.assertEqual(kinds, {"mode": "radio", "hold": "button"})

    def test_global_panel_list(self):
        """Test global panel list.

        @param self The object pointer.
        """
        ids = [p["id"] for p in self.server.get("/api/panels")["panels"]]
        self.assertIn("test-global", ids)
        self.assertIn("flow-monitor", ids)


# ---------------------------------------------------------------------------
# Object tree and attributes
# ---------------------------------------------------------------------------


class ObjectTreeTests(ServerTestCase):
    """Object tree browsing and attribute editing."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    def node(self, index):
        """Fetch the object tree root of a node.

        @param self The object pointer.
        @param index Node index.
        @return The tree entry.
        """
        return self.server.get("/api/objects?node=%d" % index)

    def child(self, parent, predicate):
        """Find the first child of an entry matching a predicate.

        @param self The object pointer.
        @param parent Object tree entry whose children are searched.
        @param predicate Callable selecting the wanted child.
        @return The matching child entry.
        """
        return next(c for c in parent["children"] if predicate(c))

    def test_node_root(self):
        """Test node root.

        @param self The object pointer.
        """
        root = self.node(0)
        self.assertEqual(root["type"], "ns3::Node")
        self.assertIn({"name": "Id", "value": "0"}, root["details"])
        labels = [c["label"] for c in root["children"]]
        self.assertIn("DeviceList[0]", labels)
        self.assertIn("ApplicationList[0]", labels)
        types = {c["type"] for c in root["children"] if c["label"] == "Aggregated"}
        self.assertTrue({"ns3::ConstantPositionMobilityModel", "ns3::Ipv4L3Protocol"} <= types)
        self.assertNotIn("ns3::Node", types)  # the node itself is not its own child

    def test_expand_children(self):
        """Test expand children.

        @param self The object pointer.
        """
        root = self.node(0)
        device = self.server.get(
            "/api/objects?id=" + self.child(root, lambda c: c["label"] == "DeviceList[0]")["id"]
        )
        self.assertEqual(device["type"], "ns3::PointToPointNetDevice")
        names = {d["name"] for d in device["details"]}
        self.assertTrue({"Interface index", "Address", "MTU"} <= names)
        child_labels = {c["label"] for c in device["children"]}
        self.assertTrue({"Channel", "TxQueue"} <= child_labels)
        ipv4 = self.server.get(
            "/api/objects?id="
            + self.child(root, lambda c: c["type"] == "ns3::Ipv4L3Protocol")["id"]
        )
        self.assertTrue(any(d["value"].startswith("10.1.1.1/24") for d in ipv4["details"]))
        self.assertTrue(any(c["label"].startswith("InterfaceList[") for c in ipv4["children"]))

    def test_same_object_same_id(self):
        """Test same object same id.

        @param self The object pointer.
        """
        root = self.node(0)
        mobility = self.child(root, lambda c: c["type"] == "ns3::ConstantPositionMobilityModel")
        again = self.child(
            self.node(0), lambda c: c["type"] == "ns3::ConstantPositionMobilityModel"
        )
        self.assertEqual(mobility["id"], again["id"])

    def test_python_created_device_listed(self):
        # Objects built with a Python constructor bypass the object factory,
        # so they report the base TypeId; check the device is there.
        """Test python created device listed.

        @param self The object pointer.
        """
        root = self.node(7)
        labels = [c["label"] for c in root["children"] if c["label"].startswith("DeviceList")]
        self.assertEqual(labels, ["DeviceList[0]", "DeviceList[1]"])

    def test_unknown_node_and_object(self):
        """Test unknown node and object.

        @param self The object pointer.
        """
        self.assertIn("error", self.node(99))
        self.assertIn("error", self.server.get("/api/objects?id=deadbeef"))

    def test_attribute_metadata_and_editing(self):
        """Test attribute metadata and editing.

        @param self The object pointer.
        """
        root = self.node(0)
        device_id = self.child(root, lambda c: c["label"] == "DeviceList[0]")["id"]
        device = self.server.get("/api/objects?id=" + device_id)
        mtu = next(a for a in device["attributes"] if a["name"] == "Mtu")
        self.assertTrue(mtu["writable"])
        self.assertEqual(mtu["type"], "ns3::UintegerValue")
        self.assertTrue(mtu["help"])
        set_attribute = lambda name, value: self.server.post(
            "/api/object-attribute", {"object": device_id, "attribute": name, "value": value}
        )
        self.assertEqual(set_attribute("Mtu", "1400")[0], 200)
        device = self.server.get("/api/objects?id=" + device_id)
        self.assertEqual(
            next(a for a in device["attributes"] if a["name"] == "Mtu")["value"], "1400"
        )
        self.assertEqual(set_attribute("Mtu", "not a number")[0], 400)
        self.assertEqual(set_attribute("NoSuchAttribute", "1")[0], 400)
        node_id = root["id"]
        status, payload = self.server.post(
            "/api/object-attribute", {"object": node_id, "attribute": "Id", "value": "5"}
        )
        self.assertEqual(status, 400)  # read-only

    def test_selecting_other_node_invalidates_old_ids(self):
        """Test selecting other node invalidates old ids.

        @param self The object pointer.
        """
        device_id = self.child(self.node(0), lambda c: c["label"] == "DeviceList[0]")["id"]
        self.node(1)
        self.assertIn("error", self.server.get("/api/objects?id=" + device_id))


class TypeCatalogueTests(ServerTestCase):
    """TypeId catalogue and default attributes."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    ## @var types
    ## Type catalogue entries by type name.
    @classmethod
    def setUpClass(cls):
        """Start the server and fetch the type catalogue.

        @param cls The test class.
        """
        super().setUpClass()
        cls.types = {t["name"]: t for t in cls.server.get("/api/types")["types"]}

    def test_catalogue_contents(self):
        """Test catalogue contents.

        @param self The object pointer.
        """
        self.assertGreater(len(self.types), 200)
        node = self.types["ns3::Node"]
        self.assertEqual(node["group"], "Network")
        self.assertIn("Id", [a["name"] for a in node["attributes"]])
        self.assertIn("GetNDevices", node["methods"])
        device = self.types["ns3::PointToPointNetDevice"]
        trace = next(t for t in device["traces"] if t["name"] == "MacTx")
        self.assertIn("TracedCallback", trace["callback"])

    def test_namespaced_types_resolved(self):
        """Test namespaced types resolved.

        @param self The object pointer.
        """
        self.assertIn("ns3::lrwpan::LrWpanNetDevice", self.types)
        self.assertTrue(self.types["ns3::lrwpan::LrWpanNetDevice"]["methods"])

    def test_set_default_attribute(self):
        """Test set default attribute.

        @param self The object pointer.
        """
        attribute = next(
            a for a in self.types["ns3::PointToPointNetDevice"]["attributes"] if a["name"] == "Mtu"
        )
        self.assertTrue(attribute["flags"] & 4)  # construction-time settable
        self.assertEqual(attribute["initial"], "1500")
        post = lambda value: self.server.post(
            "/api/default-attribute",
            {"type": "ns3::PointToPointNetDevice", "attribute": "Mtu", "value": value},
        )
        self.assertEqual(post("1234")[0], 200)
        self.assertEqual(
            self.server.console("ns.CreateObject[ns.PointToPointNetDevice]().GetMtu()").strip(),
            "1234",
        )
        self.assertEqual(post("abc")[0], 400)
        self.assertEqual(
            self.server.post(
                "/api/default-attribute", {"type": "ns3::Node", "attribute": "Nope", "value": "1"}
            )[0],
            400,
        )


# ---------------------------------------------------------------------------
# Built-in panels
# ---------------------------------------------------------------------------


class PanelTests(ServerTestCase):
    """Built-in node panels."""

    @classmethod
    def setUpClass(cls):
        """Start the server and run to a time with traffic.

        @param cls The test class.
        """
        super().setUpClass()
        cls.server.run_to(2.5)

    def panel(self, node, panel_id):
        """Fetch a node panel.

        @param self The object pointer.
        @param node Node index.
        @param panel_id Panel identifier.
        @return The panel dictionary.
        """
        return self.server.get("/api/panel?node=%d&id=%s" % (node, panel_id))

    def test_panel_list(self):
        """Test panel list.

        @param self The object pointer.
        """
        ids = [p["id"] for p in self.server.get("/api/panels?node=0")["panels"]]
        for expected in ("node-info", "interface-statistics", "ipv4-routing", "last-packets"):
            self.assertIn(expected, ids)
        self.assertNotIn("olsr-routing", ids)  # no OLSR routing installed

    def test_node_summary(self):
        """Test node summary.

        @param self The object pointer.
        """
        items = [(k.strip(), v) for k, v in self.panel(0, "node-info")["sections"][0]["items"]]
        self.assertIn(("Name", "source"), items)
        self.assertIn(("Mobility model", "ns3::ConstantPositionMobilityModel"), items)
        self.assertIn(("NetDevice 0", "ns3::PointToPointNetDevice"), items)
        self.assertTrue(any(k == "IPv4 address" and v.startswith("10.1.1.1/24") for k, v in items))

    def test_interface_statistics(self):
        """Test interface statistics.

        @param self The object pointer.
        """
        deadline = time.time() + 30
        while time.time() < deadline:
            section = self.panel(4, "interface-statistics")["sections"][0]
            if any(row[1] > 0 for row in section["rows"]):
                break
            self.server.state()  # keep the simulation and its sampling going
            time.sleep(0.05)
        self.assertEqual(section["columns"][0], "Interface")
        self.assertTrue(section["rows"])
        busy = [row for row in section["rows"] if row[1] > 0]
        self.assertTrue(busy, section["rows"])
        self.assertGreater(busy[0][4], 0)  # Tx bit rate

    def test_ipv4_routing_table(self):
        """Test ipv4 routing table.

        @param self The object pointer.
        """
        rows = self.panel(0, "ipv4-routing")["sections"][0]["rows"]
        self.assertIn("10.99.0.0", [r[0] for r in rows])
        self.assertEqual({r[3] for r in rows}, {"static", "global"})

    def test_last_packets_and_header_filter(self):
        """Test last packets and header filter.

        @param self The object pointer.
        """
        panel = self.panel(3, "last-packets")  # starts capturing
        controls = {c["name"]: c for c in panel["controls"]}
        self.assertIn("ns3::UdpHeader", controls["headers"]["options"])
        self.assertEqual(controls["mode"]["value"], "OR")
        self.server.run_to(self.server.state()["time"] + 0.5)
        sections = self.panel(3, "last-packets")["sections"]
        received = sections[1]["rows"]
        self.assertTrue(received)
        self.assertIn("ns3::", received[0][-1])  # decoded headers, not a pointer
        # Source and destination MAC addresses of the CSMA frames.
        self.assertEqual(sections[1]["columns"][3], "From")
        self.assertTrue(all(re.fullmatch(r"([0-9a-f]{2}:){5}[0-9a-f]{2}", r[3]) for r in received))
        sent = sections[0]["rows"]
        self.assertEqual(sections[0]["columns"][3], "To")
        self.assertTrue(sent)
        self.assertTrue(any(re.fullmatch(r"([0-9a-f]{2}:){5}[0-9a-f]{2}", r[3]) for r in sent))
        # Only ARP: echo traffic (UDP) no longer captured.
        self.server.post(
            "/api/panel-action",
            {
                "node": 3,
                "id": "last-packets",
                "values": {"mode": "OR", "headers": ["ns3::ArpHeader"]},
            },
        )
        captured_before = len(self.panel(3, "last-packets")["sections"][1]["rows"])
        self.server.run_to(self.server.state()["time"] + 0.5)
        rows = self.panel(3, "last-packets")["sections"][1]["rows"]
        self.assertTrue(all("UdpHeader" not in r[-1] for r in rows[captured_before:]))
        panel = self.panel(3, "last-packets")
        self.assertEqual(
            {c["name"]: c["value"] for c in panel["controls"]}["headers"], ["ns3::ArpHeader"]
        )

    def test_panel_action_without_handler(self):
        """Test panel action without handler.

        @param self The object pointer.
        """
        status, payload = self.server.post(
            "/api/panel-action", {"node": 0, "id": "node-info", "values": {}}
        )
        self.assertEqual(status, 400)


def flow_monitor_unavailable(server):
    """Tell whether the flow monitor panel is listed as unavailable.

    @param server The visualizer server.
    @return The reason the panel is unavailable, or None.
    """
    panels = {p["id"]: p for p in server.get("/api/panels")["panels"]}
    return panels["flow-monitor"]["unavailable"]


class FlowMonitorTests(ServerTestCase):
    """Flow monitor panel with flow monitoring enabled."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_FLOWMON": "1"}

    def test_flows_series_and_selection_filter(self):
        """Test flows series and selection filter.

        @param self The object pointer.
        """
        if flow_monitor_unavailable(self.server):
            self.assertIn("Flow monitor unavailable", self.server.log())
            self.skipTest("flow-monitor module not built")
        self.server.run_to(3.0)
        panel = self.server.get("/api/panel?id=flow-monitor")
        titles = [s["title"] for s in panel["sections"]]
        self.assertEqual(
            titles[:4],
            ["Throughput", "Total bytes", "Latency (interval mean)", "Jitter (interval mean)"],
        )
        throughput = panel["sections"][0]["plot"]
        self.assertEqual(throughput["unit"], "bit/s")
        self.assertTrue(throughput["series"])
        slots = [s["slot"] for s in throughput["series"]]
        self.assertEqual(slots, sorted(set(slots)))
        points = throughput["series"][0]["points"]
        times = [p[0] for p in points]
        self.assertEqual(times, sorted(times))
        self.assertTrue(
            all(b - a >= 0.099 for a, b in zip(times, times[1:]))
        )  # sampled every 0.1 s
        totals = panel["sections"][1]["plot"]["series"]
        self.assertEqual([s["name"] for s in totals], ["Transmitted", "Received"])
        all_flows = len(panel["sections"][-1]["rows"])
        self.server.control(action="select", node=6)
        filtered = self.server.get("/api/panel?id=flow-monitor")
        self.assertIn("nodes 6", filtered["sections"][0]["note"])
        rows = filtered["sections"][-1]["rows"]
        self.assertTrue(rows)
        self.assertLessEqual(len(rows), all_flows)
        self.assertTrue(all("10.1.4." in r[0] for r in rows))
        self.server.control(action="select", node=None)


class ScenarioFlowMonitorTests(ServerTestCase):
    """Flow monitor panel next to a FlowMonitor installed by a Python scenario."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_FLOWMON": "scenario"}
    ## Expression giving the scenario's FlowMonitor in the console.
    monitor = "__main__.SCENARIO_FLOW_MONITOR.GetMonitor()"
    ## Expression giving the scenario's FlowMonitorHelper in the console.
    helper = "__main__.SCENARIO_FLOW_MONITOR"

    def test_panel_and_scenario_monitor_coexist(self):
        """Test the panel and the scenario's FlowMonitor both see every flow.

        @param self The object pointer.
        """
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow-monitor module not built")
        self.server.post("/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}})
        self.server.run_to(3.0)
        self.server.control(action="pause")
        panel = self.server.get("/api/panel?id=flow-monitor")
        rows = panel["sections"][-1]["rows"]
        self.assertTrue(any(row[2] > 0 for row in rows), rows)
        # Charts span the simulation from 0 s to now.
        span = panel["sections"][0]["plot"]["range"]
        self.assertEqual(span[0], 0)
        self.assertGreaterEqual(span[1], 3.0)
        # Samples are taken inside the simulation every 0.1 s, however far
        # each step advances.
        times = [p[0] for p in panel["sections"][0]["plot"]["series"][0]["points"]]
        gaps = sorted(b - a for a, b in zip(times, times[1:]))
        self.assertAlmostEqual(gaps[len(gaps) // 2], 0.1, places=6)
        # The histogram view shows delay, jitter and packet size per flow.
        self.server.post(
            "/api/panel-action", {"id": "flow-monitor", "values": {"view": "Histograms"}}
        )
        panel = self.server.get("/api/panel?id=flow-monitor")
        histograms = [sec for sec in panel["sections"] if "histogram" in sec]
        self.assertEqual([h["title"] for h in histograms], ["Delay", "Jitter", "Packet size"])
        self.assertTrue(any(s["bins"] for s in histograms[0]["histogram"]["series"]))
        self.assertEqual(panel["sections"][-1]["title"], "Flows")
        self.server.post(
            "/api/panel-action", {"id": "flow-monitor", "values": {"view": "Time series"}}
        )
        panel = self.server.get("/api/panel?id=flow-monitor")
        self.assertEqual(panel["sections"][0]["title"], "Throughput")
        output = self.server.console(
            "stats = %s.GetFlowStats()\n"
            "print(len(stats), sum(int(i.second.rxPackets) for i in stats))" % self.monitor
        )
        flows, rx = map(int, output.split())
        self.assertEqual(flows, len(rows))
        self.assertGreater(rx, 0)

    def test_panel_statistics_match_scenario_monitor(self):
        """Test the panel counts each flow exactly as the scenario's FlowMonitor does.

        @param self The object pointer.
        """
        self.compare_with_references()

    def compare_with_references(self):
        """Rerun the scenario and compare the panel's flows with reference FlowMonitors.

        @param self The object pointer.
        @return Dict of flow 5-tuple text -> reference statistics.
        """
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow-monitor module not built")
        # Rerun the scenario, which comes back paused, so both count from 0 s.
        status, payload = self.server.control(action="restart")
        self.assertEqual(status, 200, payload)
        references = self.install_references()
        self.server.post("/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}})
        self.server.control(action="play")
        self.server.run_to(2.5)
        self.server.control(action="pause")
        output = self.server.console(
            "import json\n"
            "references = %s\n"
            "flows = ns.cppyy.gbl.pyviz_adapters.flows\n"
            "flows.CheckForLostPackets()\n"
            "def histogram(h):\n"
            "    return {str(b): int(h.GetBinCount(b)) for b in range(int(h.GetNBins()))\n"
            "            if h.GetBinCount(b)}\n"
            "def own(h):\n"
            "    return {str(int(i.first)): int(i.second) for i in h}\n"
            "def key(src, dst, sp, dp, proto):\n"
            "    return '%%s:%%d>%%s:%%d/%%d' %% (src, sp, dst, dp, proto)\n"
            "cpp = {}\n"
            "for monitor, classifier in references:\n"
            "    monitor.CheckForLostPackets()\n"
            "    for item in monitor.GetFlowStats():\n"
            "        t = classifier.FindFlow(item.first)\n"
            "        s = item.second\n"
            "        cpp[key(t.sourceAddress, t.destinationAddress, t.sourcePort,\n"
            "                t.destinationPort, t.protocol)] = [\n"
            "            int(s.txPackets), int(s.txBytes), int(s.rxPackets), int(s.rxBytes),\n"
            "            int(s.lostPackets), s.delaySum.GetTimeStep(), s.jitterSum.GetTimeStep(),\n"
            "            histogram(s.delayHistogram), histogram(s.jitterHistogram),\n"
            "            histogram(s.packetSizeHistogram)]\n"
            "py = {}\n"
            "for f in flows.flows:\n"
            "    py[key(f.source, f.destination, f.sourcePort, f.destinationPort,\n"
            "           f.protocol)] = [\n"
            "        int(f.txPackets), int(f.txBytes), int(f.rxPackets), int(f.rxBytes),\n"
            "        int(f.lostPackets), int(f.delaySum), int(f.jitterSum),\n"
            "        own(f.delayHistogram), own(f.jitterHistogram),\n"
            "        own(f.packetSizeHistogram)]\n"
            "print(json.dumps([cpp, py]))" % references
        )
        cpp, py = json.loads(output)
        self.assertTrue(cpp)
        self.assertTrue(any(stats[2] > 0 for stats in cpp.values()), cpp)
        self.assertEqual(py, cpp)
        return cpp

    def install_references(self):
        """Get the FlowMonitors the panel is compared with, called at 0 s.

        @param self The object pointer.
        @return Console expression of a list of (FlowMonitor, classifier) pairs.
        """
        return "[(%s, ns.DynamicCast[ns.Ipv4FlowClassifier](%s.GetClassifier()))]" % (
            self.monitor,
            self.helper,
        )

    def test_console_main_is_the_rerun_scenario(self):
        """Test the console's __main__ is, live, the module of the scenario run last.

        @param self The object pointer.
        """
        first = self.server.console("print(id(__main__.SCENARIO_FLOW_MONITOR))").strip()
        status, payload = self.server.control(action="restart")
        self.assertEqual(status, 200, payload)
        output = self.server.console(
            "print(id(__main__.SCENARIO_FLOW_MONITOR))\n"
            "print(__main__.__file__.endswith('pyviz-test-scenario.py'))\n"
            "__main__.PYVIZ_PROBE = 5\n"
            "print(__main__.build.__globals__.get('PYVIZ_PROBE'))\n"
            "__main__.build.__globals__['PYVIZ_PROBE'] = 6\n"
            "print(__main__.PYVIZ_PROBE, 'main' in dir(__main__))"
        ).split("\n")
        # The restarted scenario comes back paused.
        self.server.control(action="play")
        self.assertNotEqual(output[0], first)
        self.assertEqual(output[1:4], ["True", "5", "6 True"])


class CppFlowMonitorTests(ScenarioFlowMonitorTests):
    """Flow monitor panel next to a FlowMonitor installed from C++."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_FLOWMON": "cpp"}
    ## Expression giving the scenario's FlowMonitor in the console.
    monitor = "ns.cppyy.gbl.pyviz_test.Helper().GetMonitor()"
    ## Expression giving the scenario's FlowMonitorHelper in the console.
    helper = "ns.cppyy.gbl.pyviz_test.Helper()"
    ## The C++ FlowMonitor is not a scenario global; covered by the base class.
    test_console_main_is_the_rerun_scenario = None


class CongestedFlowMonitorTests(ScenarioFlowMonitorTests):
    """Flow monitor panel on a link whose device queue and queue disc drop packets."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_FLOWMON": "scenario", "PYVIZ_TEST_CONGEST": "1"}
    ## Covered by the base class.
    test_panel_and_scenario_monitor_coexist = None
    ## Covered by the base class.
    test_console_main_is_the_rerun_scenario = None

    def test_panel_statistics_match_scenario_monitor(self):
        """Test queue drops are counted as lost exactly as FlowMonitor counts them.

        @param self The object pointer.
        """
        reference = self.compare_with_references()
        self.assertTrue(any(stats[4] > 0 for stats in reference.values()), reference)


class DualStackFlowMonitorTests(ServerTestCase):
    """Flow monitor panel on a network carrying IPv4 and IPv6 flows together."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_IPV6": "1"}

    def install_references(self):
        """Install one FlowMonitor per IP version, called at 0 s.

        A FlowMonitorHelper cannot be the reference here: its IPv4 and IPv6
        classifiers number their flows from 1 each, so its FlowMonitor merges
        an IPv4 and an IPv6 flow with the same id.

        @param self The object pointer.
        @return Console expression of a list of (FlowMonitor, classifier) pairs.
        """
        self.server.console(
            "reference_probes = []\n"
            "reference_monitors = []\n"
            "for probe, classifier_type, l3 in (\n"
            "        (ns.Ipv4FlowProbe, ns.Ipv4FlowClassifier, ns.Ipv4L3Protocol),\n"
            "        (ns.Ipv6FlowProbe, ns.Ipv6FlowClassifier, ns.Ipv6L3Protocol)):\n"
            "    monitor = ns.CreateObject[ns.FlowMonitor]()\n"
            "    classifier = ns.Create[classifier_type]()\n"
            "    monitor.AddFlowClassifier(classifier)\n"
            "    for i in range(int(ns.NodeList.GetNNodes())):\n"
            "        node = ns.NodeList.GetNode(i)\n"
            "        if node.GetObject[l3]():\n"
            "            reference_probes.append(ns.CreateObject[probe](monitor, classifier, node))\n"
            "    reference_monitors.append((monitor, classifier))\n"
        )
        return "reference_monitors"

    ## The comparison of ScenarioFlowMonitorTests, against the monitors above.
    compare_with_references = ScenarioFlowMonitorTests.compare_with_references

    def test_ipv4_and_ipv6_flows_match(self):
        """Test the panel counts IPv4 and IPv6 flows of one network as FlowMonitor does.

        @param self The object pointer.
        """
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow-monitor module not built")
        flows = self.compare_with_references()
        self.assertTrue(any(k.startswith("10.") and v[2] > 0 for k, v in flows.items()), flows)
        self.assertTrue(any(k.startswith("2001:db8:") and v[2] > 0 for k, v in flows.items()))
        rows = self.server.get("/api/panel?id=flow-monitor")["sections"][-1]["rows"]
        self.assertTrue(any("2001:db8:" in row[0] for row in rows), rows)


class FlowMonitorOptInTests(ServerTestCase):
    """Flow monitor enabled from the UI."""

    def test_unavailable_panel_fails_gracefully(self):
        """Test the panel is listed, says why and rejects actions without the flow-monitor module.

        @param self The object pointer.
        """
        reason = flow_monitor_unavailable(self.server)
        if not reason:
            self.skipTest("flow-monitor module is built")
        self.assertIn("not built", reason)
        panel = self.server.get("/api/panel?id=flow-monitor")
        self.assertEqual(panel["sections"][0]["title"], reason)
        self.assertEqual(panel["controls"], [])
        status, _ = self.server.post(
            "/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}}
        )
        self.assertEqual(status, 400)
        self.server.run_to(1.0)  # still alive

    def test_disabled_until_enabled_from_ui(self):
        """Test disabled until enabled from ui.

        @param self The object pointer.
        """
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow-monitor module not built")
        self.server.run_to(1.0)
        panel = self.server.get("/api/panel?id=flow-monitor")
        self.assertEqual(panel["sections"][0]["title"], "Flow monitor is off")
        self.assertEqual(panel["controls"][0]["kind"], "button")
        # Enabling schedules events from a request handler; this used to abort
        # with "Thread-unsafe invocation".
        status, _ = self.server.post(
            "/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}}
        )
        self.assertEqual(status, 200)
        self.server.run_to(2.5)
        panel = self.server.get("/api/panel?id=flow-monitor")
        self.assertEqual(panel["sections"][0]["title"], "Throughput")
        self.assertTrue(panel["sections"][-1]["rows"])


# ---------------------------------------------------------------------------
# Console and traces module
# ---------------------------------------------------------------------------


class ConsoleTests(ServerTestCase):
    """Python console."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    def test_expressions_statements_and_errors(self):
        """Test expressions statements and errors.

        @param self The object pointer.
        """
        self.assertEqual(self.server.console("1 + 2").strip(), "3")
        self.assertEqual(self.server.console("value = 41").strip(), "")
        self.assertEqual(self.server.console("value + 1").strip(), "42")
        self.assertEqual(self.server.console("print('hi')").strip(), "hi")
        output = self.server.console("x = 1; undefined_name")
        self.assertIn("NameError", output)
        self.assertNotIn("SyntaxError", output)
        self.assertIn("SyntaxError", self.server.console("def broken(:"))

    def test_namespace_follows_selection(self):
        """Test namespace follows selection.

        @param self The object pointer.
        """
        self.assertIn("8", self.server.console("ns.NodeList.GetNNodes()"))
        self.server.control(action="select", node=3)
        self.server.control(action="select", node=5, add=True)
        self.assertEqual(self.server.console("node.GetId()").strip(), "5")
        self.assertEqual(self.server.console("[n.GetId() for n in nodes]").strip(), "[3, 5]")
        self.assertIn("Visualizer", self.server.console("type(viz).__name__"))


class TracesModuleTests(ServerTestCase):
    """Trace connection helpers."""

    ## Initial simulation speed (0 leaves the default).
    speed = 0

    def test_unregistered_types_are_skipped(self):
        """Test unregistered types are skipped.

        @param self The object pointer.
        """
        output = self.server.console(
            "import importlib\n"
            # In the pip package the real module is ns3.visualizer.traces.
            "traces = importlib.import_module(type(viz).__module__.rsplit('.', 1)[0] + '.traces')\n"
            "connect_traces, type_is_registered = traces.connect_traces, traces.type_is_registered\n"
            "print(type_is_registered('ns3::Node'), type_is_registered('ns3::DoesNotExist'))\n"
            "print(connect_traces(viz.sim_helper, [('ns3::DoesNotExist', 'Tx',"
            " '/NodeList/*/DeviceList/*/$ns3::DoesNotExist/Mac/MacTx')]))"
        )
        self.assertEqual(output.split(), ["True", "False", "[]"])

    def test_unknown_handler_rejected(self):
        """Test unknown handler rejected.

        @param self The object pointer.
        """
        output = self.server.console(
            "viz.sim_helper.ConnectTrace('NoSuchHandler', '/NodeList/*/DeviceList/*/TxQueue/Drop')"
        )
        self.assertEqual(output.strip(), "False")


# ---------------------------------------------------------------------------
# Restart and VisualSimulatorImpl
# ---------------------------------------------------------------------------


class RestartTests(ServerTestCase):
    """Restarting the simulation."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_HOOKS": "1"}

    def test_restart_rebuilds_and_reruns(self):
        """Test restart rebuilds and reruns.

        @param self The object pointer.
        """
        for _ in range(2):
            before = self.server.run_to(1.5)
            self.server.control(action="select", node=2)
            status, payload = self.server.control(action="restart")
            self.assertEqual(status, 200, payload)
            after = self.server.state()
            self.assertLess(after["time"], 0.5)
            self.assertFalse(after["running"])
            self.server.control(action="play")
            self.assertEqual(len(after["nodes"]), len(before["nodes"]))
            # Hub ids contain the channel's address, which changes on rebuild.
            normalize = lambda links: sorted(
                (
                    l["source"],
                    "hub" if str(l["target"]).startswith("channel:") else str(l["target"]),
                )
                for l in links
            )
            self.assertEqual(normalize(after["links"]), normalize(before["links"]))
            self.assertEqual(after["selectedNodes"], [])
            # Hooks and plugins registered again by the rerun scenario.
            self.assertEqual(after["nodes"][0]["color"], "#ff00ff")
            self.assertEqual(len([o for o in after["overlays"] if o["class"] == "test-overlay"]), 1)
            # The Python-created device on node 7 survived the rerun.
            labels = [c["label"] for c in self.server.get("/api/objects?node=7")["children"]]
            self.assertIn("DeviceList[1]", labels)
        self.server.run_until(lambda s: s["transmissions"], timeout=30)


class VisualSimulatorImplTests(ServerTestCase):
    """Running with VisualSimulatorImpl."""

    ## Simulation stop time passed to the scenario.
    stop = STOP_TIME
    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_MODE": "visual"}

    def test_runs_stops_and_restarts(self):
        """Test runs stops and restarts.

        @param self The object pointer.
        """
        state = self.server.run_until(lambda s: s["finished"], timeout=120)
        self.assertAlmostEqual(state["time"], STOP_TIME, places=6)
        for _ in range(2):
            status, payload = self.server.control(action="restart")
            self.assertEqual(status, 200, payload)
            self.assertLess(self.server.state()["time"], 0.5)
            self.server.control(action="play")
            self.server.run_until(lambda s: s["transmissions"], timeout=30)
        status, _ = self.server.post(
            "/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}}
        )
        self.assertEqual(status, 200)
        self.server.run_to(self.server.state()["time"] + 1)


def find_cpp_program(name):
    """Find a built C++ ns-3 program that can run under the visualizer.

    PYVIZ_TEST_CPP_PROGRAM overrides the search. Otherwise the program is
    looked up in the ns-3 build these tests run from: its Python bindings
    must be the ones on the path, so the program embeds the same ns-3.

    @param name Program name, such as "simple-global-routing".
    @return Path of the program, or None if it is not available.
    """
    if os.environ.get("PYVIZ_TEST_CPP_PROGRAM"):
        return os.environ["PYVIZ_TEST_CPP_PROGRAM"]
    root = os.path.dirname(os.path.dirname(os.path.dirname(TEST_DIR)))
    for lock in sorted(f for f in os.listdir(root) if f.startswith(".lock-ns3")):
        with open(os.path.join(root, lock), encoding="utf-8") as f:
            text = f.read()
        out_dir = re.search(r"^out_dir = '([^']*)'", text, re.M)
        bindings = out_dir and os.path.join(out_dir.group(1), "bindings", "python")
        if not bindings or bindings not in os.environ.get("PYTHONPATH", "").split(os.pathsep):
            continue
        for program in re.findall(r"'([^']*/ns3[^/']*-%s-[^/']*)'" % re.escape(name), text):
            if os.path.isfile(program):
                return program
    return None


class CppProgramTests(ServerTestCase):
    """A C++ program run under VisualSimulatorImpl."""

    ## Extra scenario arguments.
    args = ("--EnableMonitor=true", "--SimulatorImplementationType=ns3::VisualSimulatorImpl")

    @classmethod
    def setUpClass(cls):
        """Start the C++ program, or skip if it is not built.

        @param cls The test class.
        """
        program = find_cpp_program("simple-global-routing")
        if program is None:
            raise unittest.SkipTest(
                "simple-global-routing not found (build examples, or set PYVIZ_TEST_CPP_PROGRAM)"
            )
        cls.scenario = [program]
        super().setUpClass()

    def wait_for_server(self, timeout=120):
        """Wait until the program executed again serves the page.

        @param self The object pointer.
        @param timeout Maximum time to wait, in seconds.
        @return The first state served.
        """
        deadline = time.time() + timeout
        while True:
            try:
                return self.server.state()
            except (urllib.error.URLError, ConnectionError):
                if time.time() > deadline:
                    raise
                time.sleep(0.2)

    def flow_labels(self):
        """Get the flows listed by the flow monitor panel.

        @param self The object pointer.
        @return List of flow labels.
        """
        panel = self.server.get("/api/panel?id=flow-monitor")
        self.assertEqual(panel["sections"][-1]["title"], "Flows", "flow monitor panel is off")
        return [row[0] for row in panel["sections"][-1]["rows"]]

    def test_flow_monitor_and_restart(self):
        """Test flows are shown next to the program's FlowMonitor, and restart executes it again.

        @param self The object pointer.
        """
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow-monitor module not built")
        pid = self.server.process.pid
        for _ in range(2):
            # The program installs its own FlowMonitor; the panel collects
            # its statistics next to it.
            self.server.post(
                "/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}}
            )
            self.server.run_to(2.0)
            self.assertTrue(self.flow_labels())
            status, payload = self.server.control(action="restart")
            self.assertEqual(status, 200, payload)
            # The program is executed again in the same process, on the same
            # port, which it keeps listening on meanwhile: nobody else can
            # bind it to collect the token the page keeps sending.
            import socket

            port = int(self.server.url.rsplit(":", 1)[1])
            for _ in range(20):
                with socket.socket() as squatter:
                    with self.assertRaises(OSError):
                        squatter.bind(("127.0.0.1", port))
                time.sleep(0.05)
            state = self.wait_for_server()
            self.assertLess(state["time"], 0.5)
            self.assertEqual(state["speed"], self.speed)
            self.assertEqual(self.server.process.pid, pid)
            self.assertIsNone(self.server.process.poll())
            for body in ({"action": "speed", "value": self.speed}, {"action": "play"}):
                status, payload = self.server.control(**body)
                self.assertEqual(status, 200, payload)
        self.server.post("/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}})
        self.server.run_to(2.0)
        self.assertTrue(self.flow_labels())


# ---------------------------------------------------------------------------
# Wireless technologies
# ---------------------------------------------------------------------------


class WirelessTests(ServerTestCase):
    """Transmissions of the wireless technologies."""

    ## Scenario script the server runs.
    scenario = WIRELESS_SCENARIO
    ## Initial simulation speed (0 leaves the default).
    speed = 5.0

    ## @var ids
    ## Node ids by node name.
    ## @var pairs
    ## (source, target) pairs of transmissions seen.
    @classmethod
    def setUpClass(cls):
        """Start the server and record the transmissions shown until the end.

        @param cls The test class.
        """
        super().setUpClass()
        state = cls.server.state()
        cls.ids = {n["name"]: n["id"] for n in state["nodes"] if n["name"]}
        cls.pairs = set()
        deadline = time.time() + 60
        while time.time() < deadline:
            state = cls.server.state()
            cls.pairs |= {(t["source"], t["target"]) for t in state["transmissions"]}
            if state["finished"]:
                break
            time.sleep(0.03)

    def require(self, *names):
        """Skip the test unless all the named nodes exist.

        @param self The object pointer.
        @param names Arguments passed to the script.
        """
        missing = [n for n in names if n not in self.ids]
        if missing:
            self.skipTest("module not built (%s missing)" % ", ".join(missing))

    def assertTransmission(self, source, target):
        """Assert that a transmission between two named nodes was shown.

        @param self The object pointer.
        @param source Name of the transmitting node.
        @param target Name of the receiving node.
        """
        pair = (self.ids[source], self.ids[target])
        self.assertIn(
            pair,
            self.pairs,
            "%s -> %s not displayed; seen %s" % (source, target, sorted(self.pairs)),
        )

    def test_wifi_adhoc(self):
        """Test wifi adhoc.

        @param self The object pointer.
        """
        self.require("wifi-a", "wifi-b")
        self.assertTransmission("wifi-a", "wifi-b")

    def test_lrwpan(self):
        """Test lrwpan.

        @param self The object pointer.
        """
        self.require("lrwpan-a", "lrwpan-b")
        self.assertTransmission("lrwpan-a", "lrwpan-b")

    def test_device_traces_are_discovered_on_one_object(self):
        """Test MacTx and MacRx are discovered on the same object of each device.

        Wi-Fi devices have a "Phy" Pointer attribute before "Mac", so this
        also checks the choice does not depend on the attribute order.

        @param self The object pointer.
        """
        self.require("wifi-a", "lrwpan-a")
        output = self.server.console(
            "import json, sys\n"
            "traces = sys.modules[viz.__module__.rpartition('.')[0] + '.traces']\n"
            "class Recorder:\n"
            "    def __init__(self):\n"
            "        self.paths = []\n"
            "    def ConnectTrace(self, handler, path):\n"
            "        self.paths.append((handler, path))\n"
            "        return True\n"
            "recorder = Recorder()\n"
            "traces._discover_device_traces(recorder)\n"
            "types = {}\n"
            "for i in range(int(ns.NodeList.GetNNodes())):\n"
            "    node = ns.NodeList.GetNode(i)\n"
            "    for d in range(int(node.GetNDevices())):\n"
            "        types['/NodeList/%d/DeviceList/%d/' % (i, d)] = str(\n"
            "            node.GetDevice(d).GetInstanceTypeId().GetName())\n"
            "print(json.dumps([recorder.paths, types]))"
        )
        paths, types = json.loads(output)
        by_device = {}
        for handler, path in paths:
            base = re.match(r"/NodeList/\d+/DeviceList/\d+/", path).group(0)
            by_device.setdefault(base, {})[path[len(base) :]] = handler
        expected = {
            "ns3::WifiNetDevice": {"Mac/MacTx": "Tx", "Mac/MacRx": "Rx"},
            "ns3::lrwpan::LrWpanNetDevice": {"Mac/MacTx": "Tx", "Mac/MacRx": "Rx"},
            "ns3::PointToPointNetDevice": {"MacTx": "Tx", "MacRx": "Rx"},
        }
        seen = set()
        for base, type_name in types.items():
            if type_name in expected:
                self.assertEqual(by_device.get(base), expected[type_name], (base, type_name))
                seen.add(type_name)
        self.assertEqual(seen, set(expected))

    def test_lte_downlink_and_uplink(self):
        """Test lte downlink and uplink.

        @param self The object pointer.
        """
        self.require("enb", "ue")
        self.assertTransmission("enb", "ue")
        self.assertTransmission("ue", "enb")

    def test_node_info_of_lte_devices(self):
        """Test the node info panel of LTE nodes, whose devices have 64-bit MAC addresses.

        @param self The object pointer.
        """
        self.require("enb", "ue")
        for name, device in (("enb", "ns3::LteEnbNetDevice"), ("ue", "ns3::LteUeNetDevice")):
            panel = self.server.get("/api/panel?node=%d&id=node-info" % self.ids[name])
            items = [[k.strip(), v] for k, v in panel["sections"][0]["items"]]
            mac = items[items.index(["NetDevice 0", device]) + 1]
            self.assertEqual(mac[0], "MAC address", items)
            self.assertEqual(mac[1].count(":"), 7, items)
        self.assertTrue(self.server.alive(), self.server.log())

    def test_lte_flows_are_counted_end_to_end(self):
        """Test LTE flows are counted end to end, without their GTP-U tunnels.

        @param self The object pointer.
        """
        self.require("enb", "ue")
        if flow_monitor_unavailable(self.server):
            self.skipTest("flow monitor unavailable")
        # Rerun the scenario, which comes back paused, to count from 0 s.
        status, payload = self.server.control(action="restart")
        self.assertEqual(status, 200, payload)
        self.server.post("/api/panel-action", {"id": "flow-monitor", "values": {"enable": True}})
        self.server.control(action="play")
        self.server.run_to(2.0)
        rows = self.server.get("/api/panel?id=flow-monitor")["sections"][-1]["rows"]
        self.assertFalse([r for r in rows if ":2152 " in r[0] + " "], rows)
        self.assertTrue([r for r in rows if "7.0.0." in r[0] and r[2] > 0], rows)

    def test_wireless_devices_draw_no_wired_links(self):
        """Test wireless devices draw no wired links.

        @param self The object pointer.
        """
        linked = {
            x for link in self.server.state()["links"] for x in (link["source"], link["target"])
        }
        for name in ("wifi-a", "wifi-b", "lrwpan-a", "lrwpan-b", "ue"):
            if name in self.ids:
                self.assertNotIn(self.ids[name], linked, name)


# ---------------------------------------------------------------------------
# Browser UI (Selenium)
# ---------------------------------------------------------------------------


def make_driver():
    """Return a Selenium Chrome driver, or a reason string when unavailable.

    @return The driver, or a string with the reason it is unavailable.
    """
    try:
        from selenium import webdriver
        from selenium.webdriver.chrome.service import Service
    except ImportError:
        return "selenium is not installed (pip install selenium)"
    options = webdriver.ChromeOptions()
    debugger = os.environ.get("PYVIZ_SELENIUM_DEBUGGER")
    if debugger:
        options.debugger_address = debugger
    else:
        for argument in (
            "--headless=new",
            "--no-sandbox",
            "--disable-gpu",
            "--window-size=1600,1000",
            # Headless pages can be treated as hidden, which throttles timers
            # to about one per second and stops requestAnimationFrame.
            "--disable-background-timer-throttling",
            "--disable-renderer-backgrounding",
            "--disable-backgrounding-occluded-windows",
        ):
            options.add_argument(argument)
        if os.environ.get("PYVIZ_CHROME_BINARY"):
            options.binary_location = os.environ["PYVIZ_CHROME_BINARY"]
    try:
        if os.environ.get("PYVIZ_WEBDRIVER_URL"):
            driver = webdriver.Remote(
                command_executor=os.environ["PYVIZ_WEBDRIVER_URL"], options=options
            )
        else:
            service = (
                Service(os.environ["PYVIZ_CHROMEDRIVER"])
                if os.environ.get("PYVIZ_CHROMEDRIVER")
                else Service()
            )
            driver = webdriver.Chrome(options=options, service=service)
    except Exception as error:  # no browser or driver
        return "no usable Chrome/Chromium for Selenium: %s" % str(error).splitlines()[0]
    driver.set_window_size(1600, 1000)
    return driver


class UiTests(ServerTestCase):
    """Browser UI driven with Selenium."""

    ## Extra environment variables for the scenario.
    env = {"PYVIZ_TEST_HOOKS": "1", "PYVIZ_TEST_FLOWMON": "1"}
    ## Initial simulation speed (0 leaves the default).
    speed = 0  # the page itself sets and shows the speed

    ## @var driver
    ## Selenium WebDriver.
    ## @var By
    ## Selenium By class.
    ## @var ActionChains
    ## Selenium ActionChains class.
    ## @var Keys
    ## Selenium Keys class.
    ## @var Select
    ## Selenium Select class.
    @classmethod
    def setUpClass(cls):
        """Start the browser and the server, and import the Selenium helpers.

        @param cls The test class.
        """
        driver = make_driver()
        if isinstance(driver, str):
            if os.environ.get("PYVIZ_REQUIRE_SELENIUM") == "1":
                raise RuntimeError(driver)
            raise unittest.SkipTest(driver)
        cls.driver = driver
        try:
            super().setUpClass()
        except unittest.SkipTest:
            driver.quit()
            raise
        from selenium.webdriver.common.action_chains import ActionChains
        from selenium.webdriver.common.by import By
        from selenium.webdriver.common.keys import Keys
        from selenium.webdriver.support.ui import Select

        cls.By, cls.ActionChains, cls.Keys, cls.Select = By, ActionChains, Keys, Select

    @classmethod
    def tearDownClass(cls):
        """Close the browser and stop the server.

        @param cls The test class.
        """
        if hasattr(cls, "driver"):
            cls.driver.quit()
        if hasattr(cls, "server"):
            super().tearDownClass()

    ## @var errors_at_start
    ## Browser errors present after the page loaded.
    def setUp(self):
        # The tests share one server; start each from the same state.
        """Reset the shared simulation state and load the page.

        @param self The object pointer.
        """
        self.server.control(action="select", node=None)
        self.server.control(action="transmissions", value="all")
        self.server.control(action="play")
        self.server.control(action="speed", value=0.01)
        self.server.control(action="trails", value=False)
        if self.server.state()["held"]:
            self.server.post("/api/panel-action", {"id": "test-global", "values": {"hold": True}})
        # Leave the page first: a URL differing only in its fragment would not
        # reload it, and the token fragment is removed once the page reads it.
        self.driver.get("about:blank")
        self.driver.get(self.server.url + "/#token=" + self.server.token)
        self.wait(lambda: "Running" in self.text("#clock") or "Finished" in self.text("#clock"))
        self.errors_at_start = self.browser_errors()

    def tearDown(self):
        """Check the server and that no new browser errors appeared.

        @param self The object pointer.
        """
        super().tearDown()
        errors = [
            e for e in self.browser_errors() if e not in self.errors_at_start and "favicon" not in e
        ]
        self.assertEqual(errors, [])

    # Helpers

    def browser_errors(self):
        """Return the severe messages in the browser log.

        @param self The object pointer.
        @return List of error messages (empty when the log is unavailable).
        """
        try:
            return [e["message"] for e in self.driver.get_log("browser") if e["level"] == "SEVERE"]
        except Exception:
            return []

    def wait(self, condition, timeout=30):
        """Wait until a condition holds, saving a screenshot on timeout.

        @param self The object pointer.
        @param condition Callable checked until it returns a true value.
        @param timeout Maximum time to wait, in seconds.
        """
        deadline = time.time() + timeout
        while True:
            try:
                if condition():
                    return
            except Exception:
                pass
            if time.time() > deadline:
                path = os.path.join(
                    tempfile.gettempdir(), "pyviz-ui-failure-%s.png" % self._testMethodName
                )
                try:
                    self.driver.save_screenshot(path)
                except Exception:
                    path = "(no screenshot)"
                raise AssertionError(
                    "timed out waiting; clock=%r; screenshot %s" % (self.text("#clock"), path)
                )
            time.sleep(0.1)

    def find(self, selector):
        """Find one element by CSS selector.

        @param self The object pointer.
        @param selector CSS selector.
        @return The element.
        """
        return self.driver.find_element(self.By.CSS_SELECTOR, selector)

    def find_all(self, selector):
        """Find all elements matching a CSS selector.

        @param self The object pointer.
        @param selector CSS selector.
        @return List of elements.
        """
        return self.driver.find_elements(self.By.CSS_SELECTOR, selector)

    def text(self, selector):
        """Return the text of an element.

        @param self The object pointer.
        @param selector CSS selector.
        @return The text, or an empty string if not found.
        """
        try:
            return self.find(selector).text
        except Exception:
            return ""

    def js(self, script, *args):
        """Run JavaScript in the page.

        @param self The object pointer.
        @param script JavaScript code to run in the page.
        @param args Arguments passed to the script.
        @return The script's return value.
        """
        return self.driver.execute_script(script, *args)

    def set_speed(self, exponent):
        """Move the speed slider.

        @param self The object pointer.
        @param exponent Speed slider value (base 10 exponent of the speed).
        """
        self.js(
            "const s = document.querySelector('#speed'); s.value = arguments[0]; s.dispatchEvent(new Event('input'));",
            exponent,
        )

    # The topology SVG is redrawn on every poll (~30 times a second), so
    # element references to nodes go stale almost immediately. Nodes are read
    # with one script call and clicked by position instead.

    def node_info(self, index):
        """Return {x, y, width, fill, classes, image} of a node in viewport coordinates.

        @param self The object pointer.
        @param index Node index.
        @return Dictionary with the node's mark geometry and style, or None.
        """
        return self.js(
            "const g = document.querySelector('.node-group[data-node=\"' + arguments[0] + '\"]');"
            "if (!g) return null;"
            "const mark = g.querySelector('circle, image'), r = mark.getBoundingClientRect();"
            "return {x: r.left + r.width / 2, y: r.top + r.height / 2, width: r.width,"
            " fill: getComputedStyle(mark).fill, classes: mark.getAttribute('class') || '',"
            " image: mark.tagName === 'image'};",
            index,
        )

    def at_node(self, index):
        """ActionChains positioned over a node.

        @param self The object pointer.
        @param index Node index.
        @return An ActionChains moved to the node's centre.
        """
        info = self.node_info(index)
        scene = self.find("#scene")
        rect = self.js("return document.querySelector('#scene').getBoundingClientRect().toJSON()")
        dx = info["x"] - (rect["left"] + rect["width"] / 2)
        dy = info["y"] - (rect["top"] + rect["height"] / 2)
        return self.ActionChains(self.driver).move_to_element_with_offset(scene, int(dx), int(dy))

    def click_node(self, index, modifier=None):
        """Click a node, optionally with a modifier key held.

        @param self The object pointer.
        @param index Node index.
        @param modifier Key held during the click, or None.
        """
        actions = self.at_node(index)
        if modifier:
            actions.key_down(modifier).click().key_up(modifier).perform()
        else:
            actions.click().perform()

    def open_global_panel(self, panel_id):
        """Open a global panel from the toolbar menu.

        @param self The object pointer.
        @param panel_id Panel identifier.
        @return The new panel window element.
        """
        self.Select(self.find("#global-panels")).select_by_value(panel_id)
        self.wait(lambda: self.find_all(".panel-window"))
        return self.find_all(".panel-window")[-1]

    # Tests

    def test_title_icon_logo_and_default_speed(self):
        """Test title icon logo and default speed.

        @param self The object pointer.
        """
        self.assertEqual(self.driver.title, "ns-3 PyViz")
        self.assertEqual(
            self.js("return document.querySelector('link[rel=icon]').getAttribute('href')"),
            "/favicon.ico",
        )
        self.assertTrue(
            self.js(
                "const i = document.querySelector('.brand img'); return i.complete && i.naturalWidth > 0"
            )
        )
        self.assertEqual(self.text("#speed-value"), "0.01x")
        self.assertEqual(self.find("#speed").get_attribute("value"), "-2")

    def test_topology_rendered(self):
        """Test topology rendered.

        @param self The object pointer.
        """
        self.wait(lambda: len(self.find_all(".node-group")) == 8)
        self.assertEqual(len(self.find_all(".hub")), 1)
        self.assertEqual(len(self.find_all("line.link")), 5)
        self.wait(lambda: self.find_all("line.overlay.wifi-link"))
        labels = self.js(
            "return [...document.querySelectorAll('.node-group text')].map(t => t.textContent)"
        )
        self.assertIn("source (0)", labels)
        self.assertIn("router", labels)
        self.assertEqual(self.node_info(0)["fill"], "rgb(255, 0, 255)")
        self.assertTrue(self.node_info(3)["image"])
        self.assertGreater(self.node_info(2)["width"], 2 * self.node_info(1)["width"])

    def test_traffic_rates_drops_and_packets(self):
        """Test traffic rates drops and packets.

        @param self The object pointer.
        """
        self.set_speed(0.7)  # 5x
        self.wait(lambda: self.find_all("line.transmission"), timeout=60)
        self.wait(lambda: self.find_all("text.rate"), timeout=30)
        # Read in one call: the scene is redrawn on every poll.
        label = self.js("const t = document.querySelector('text.rate'); return t && t.textContent")
        self.assertRegex(label, r"\d+\.\d\d [kMG]bit/s")
        self.wait(lambda: self.find_all(".drop-packet"), timeout=30)
        self.wait(lambda: self.find_all("#packets .packet-hop"), timeout=30)
        hop = self.js(
            "const g = document.querySelector('#packets .packet-hop');"
            " return g && [g.dataset.hop, getComputedStyle(g).pointerEvents]"
        )
        self.assertRegex(hop[0], r"^\d+->\d+$")
        self.assertEqual(hop[1], "none")  # never in the way of node clicks

    def test_busy_node_clickable_under_packets(self):
        """Test busy node clickable under packets.

        @param self The object pointer.
        """
        self.set_speed(0)  # 1x: node 4 (access point) has packets crossing it
        self.wait(lambda: len(self.find_all("#packets .packet-hop")) > 5, timeout=60)
        for _ in range(5):
            self.click_node(4)
            self.wait(lambda: self.server.state()["selected"] == 4, timeout=5)
            self.server.control(action="select", node=None)

    def test_controls(self):
        """Test controls.

        @param self The object pointer.
        """
        self.set_speed(0)
        self.wait(lambda: self.server.state()["speed"] == 1.0)
        self.assertEqual(self.text("#speed-value"), "1.00x")
        # One button toggles: it reads "Pause" while running, "Play" while paused.
        toggle = self.find("#play-pause")
        self.assertEqual(toggle.text, "Pause")
        toggle.click()
        self.wait(lambda: "Paused" in self.text("#clock"))
        self.wait(lambda: toggle.text == "Play")
        frozen = self.text("#clock")
        time.sleep(0.5)
        self.assertEqual(self.text("#clock"), frozen)
        self.assertFalse(self.find_all('[data-action="stop"]'))  # merged into Pause
        toggle.click()
        self.wait(lambda: "Running" in self.text("#clock"))
        self.wait(lambda: toggle.text == "Pause")
        self.Select(self.find("#transmissions")).select_by_value("none")
        self.wait(lambda: self.server.state()["showTransmissions"] == "none")
        self.Select(self.find("#transmissions")).select_by_value("all")
        self.js(
            "const s = document.querySelector('#smoothing'); s.value = 12; s.dispatchEvent(new Event('change'));"
        )
        self.wait(lambda: self.server.state()["smoothing"] == 12)
        self.set_speed(-2)

    def test_battery_indicator(self):
        """Test battery indicator.

        @param self The object pointer.
        """
        self.set_speed(0.7)
        self.wait(lambda: len(self.find_all(".battery")) == 2)
        batteries = self.js(
            "return [...document.querySelectorAll('.battery')].map(b => [b.closest('.node-group').dataset.node,"
            " Number(b.dataset.fraction), b.querySelector('.battery-text').textContent, b.querySelector('title').textContent])"
        )
        self.assertEqual(sorted(b[0] for b in batteries), ["5", "6"])
        for _node, fraction, label, title in batteries:
            self.assertEqual(label, "%d%%" % round(fraction * 100))
            self.assertIn("mAh", title)
        first = batteries[0][1]
        self.wait(
            lambda: self.js("return Number(document.querySelector('.battery').dataset.fraction)")
            < first - 0.01,
            timeout=60,
        )

    def test_trails_button(self):
        """Test trails button.

        @param self The object pointer.
        """
        button = self.find("#toggle-trails")
        self.assertEqual(button.get_attribute("aria-pressed"), "false")
        button.click()
        self.wait(lambda: self.server.state()["showTrails"])
        self.wait(lambda: button.get_attribute("aria-pressed") == "true")
        self.set_speed(0.7)
        self.wait(lambda: self.find_all("polyline.trail"), timeout=60)
        nodes = set(
            self.js(
                "return [...document.querySelectorAll('polyline.trail[data-node]')].map(p => p.dataset.node)"
            )
        )
        self.assertTrue(nodes <= {"5", "6"} and nodes)
        button.click()
        self.wait(lambda: not self.find_all("polyline.trail"))
        self.assertEqual(button.get_attribute("aria-pressed"), "false")

    def test_status_bar_does_not_move_when_text_changes(self):
        """Test status bar does not move when text changes.

        @param self The object pointer.
        """
        boxes = lambda: [
            tuple(self.find(s).rect.values())
            for s in ("#speed", "#transmissions", "#zoom-fit", "#scene")
        ]
        before = boxes()
        for exponent in (-2, -1.3, 0.7, 2, -2):
            self.set_speed(exponent)
            time.sleep(0.2)
            self.assertEqual(boxes(), before)

    def test_restart_button(self):
        """Test restart button.

        @param self The object pointer.
        """
        self.set_speed(1)
        self.wait(lambda: self.server.state()["time"] > 1.0)
        self.find('[data-action="restart"]').click()
        self.wait(lambda: self.server.state()["time"] < 1.0)
        self.wait(lambda: len(self.find_all(".node-group")) == 8)
        self.find("#play-pause").click()
        self.set_speed(-2)

    def test_select_object_tree_and_edit_attribute(self):
        """Test select object tree and edit attribute.

        @param self The object pointer.
        """
        self.click_node(0)
        self.wait(lambda: "Node 0 object tree" in self.text("#objects"))
        root = self.find("#objects details.object")
        self.assertIn("ns3::Node", root.find_element(self.By.CSS_SELECTOR, "summary").text)
        device = next(
            d
            for d in self.find_all("#objects details.object details.object")
            if "DeviceList[0]" in d.text
        )
        device.find_element(self.By.CSS_SELECTOR, "summary").click()
        self.wait(lambda: "Mtu" in device.text)
        row = next(
            r
            for r in device.find_elements(self.By.CSS_SELECTOR, ".attribute")
            if r.text.startswith("Mtu")
        )
        field = row.find_element(self.By.CSS_SELECTOR, "input")
        field.clear()
        field.send_keys("1300")
        row.find_element(self.By.CSS_SELECTOR, "button").click()
        self.wait(lambda: "Updated" in row.text)
        self.assertIn("1300", self.server.console("ns.NodeList.GetNode(0).GetDevice(0).GetMtu()"))
        self.wait(lambda: "selected" in self.node_info(0)["classes"])

    def test_mobility_bounds_drawn_for_selected_node(self):
        """Test mobility bounds drawn for selected node.

        @param self The object pointer.
        """
        self.assertFalse(self.find_all("rect.mobility-bounds"))
        self.server.control(action="pause")
        self.click_node(5)
        self.wait(
            lambda: self.js(
                "return [...document.querySelectorAll('rect.mobility-bounds')].map(r => r.dataset.node)"
            )
            == ["5"]
        )
        self.click_node(1)  # no bounds
        self.wait(lambda: not self.find_all("rect.mobility-bounds"))
        self.server.control(action="play")

    def test_multi_select(self):
        """Test multi select.

        @param self The object pointer.
        """
        self.click_node(2)
        self.click_node(4, self.Keys.SHIFT)
        self.click_node(5, self.Keys.CONTROL)
        self.wait(lambda: self.server.state()["selectedNodes"] == [2, 4, 5])
        self.wait(lambda: len(self.find_all(".node.selected")) == 3)
        self.assertEqual(len(self.find_all(".node.primary")), 1)

    def test_tooltip(self):
        """Test tooltip.

        @param self The object pointer.
        """
        self.at_node(2).perform()
        self.wait(
            lambda: self.find("#tooltip").is_displayed() and "Node 2" in self.text("#tooltip")
        )
        self.assertIn("Test info", self.text("#tooltip"))
        # Next to the sidebar it must stay whole and inside the topology.
        area = self.js("return document.querySelector('.topology').getBoundingClientRect().right")
        tip = self.find("#tooltip")
        self.js("placeInTopology(document.querySelector('#tooltip'), {x: 0, y: 5})")
        width = tip.rect["width"]
        # The browser computes the width from the position set here, as a hover does.
        self.js(
            "const t = document.querySelector('#tooltip');"
            "t.style.left = `${t.parentElement.clientWidth - 5}px`;"
            "placeInTopology(t, {x: t.parentElement.clientWidth - 5, y: 5});"
        )
        self.assertEqual(tip.rect["width"], width)
        self.assertLessEqual(tip.rect["x"] + tip.rect["width"], area + 1)
        self.js("document.querySelector('#scene').dispatchEvent(new PointerEvent('pointerleave'))")
        self.wait(lambda: not self.find("#tooltip").is_displayed())

    def test_context_menu_panels_and_window_management(self):
        """Test context menu panels and window management.

        @param self The object pointer.
        """
        self.at_node(0).context_click().perform()
        self.wait(lambda: self.find("#node-menu").is_displayed())
        entries = [b.text for b in self.find_all("#node-menu button")]
        for expected in (
            "Show Node summary",
            "Show Interface statistics",
            "Show IPv4 routing table",
            "Show Last packets",
            "Follow node",
        ):
            self.assertIn(expected, entries)
        next(
            b for b in self.find_all("#node-menu button") if b.text == "Show IPv4 routing table"
        ).click()
        self.wait(lambda: self.find_all(".panel-window"))
        window = self.find_all(".panel-window")[-1]
        self.wait(lambda: "10.99.0.0" in window.text)
        # Drag the window by its title bar.
        before = window.rect
        header = window.find_element(self.By.CSS_SELECTOR, "header strong")
        self.ActionChains(self.driver).drag_and_drop_by_offset(header, -150, 80).perform()
        after = window.rect
        self.assertAlmostEqual(after["x"] - before["x"], -150, delta=3)
        self.assertAlmostEqual(after["y"] - before["y"], 80, delta=3)
        # Minimize to the taskbar and restore.
        window.find_element(self.By.CSS_SELECTOR, "button.minimize").click()
        self.wait(lambda: not window.is_displayed())
        task = next(t for t in self.find_all("#taskbar .task") if "IPv4 routing table" in t.text)
        self.assertIn("minimized", task.get_attribute("class"))
        task.click()
        self.wait(lambda: window.is_displayed())
        # Restored windows leave the taskbar; only minimized ones are listed.
        self.wait(
            lambda: not any("IPv4 routing table" in t.text for t in self.find_all("#taskbar .task"))
        )
        # A second window comes to the front when clicked.
        self.open_global_panel("test-global")
        second = self.find_all(".panel-window")[-1]
        z = lambda w: int(w.value_of_css_property("z-index"))
        self.assertGreater(z(second), z(window))
        # Any pointer press on a window brings it to the front.
        self.js(
            "arguments[0].dispatchEvent(new PointerEvent('pointerdown', {bubbles: true}))", window
        )
        self.assertGreater(z(window), z(second))
        # Close removes the window and its taskbar button.
        window.find_element(self.By.CSS_SELECTOR, "button.close").click()
        self.wait(
            lambda: not any("IPv4 routing table" in t.text for t in self.find_all("#taskbar .task"))
        )

    def test_panel_controls(self):
        """Test panel controls.

        @param self The object pointer.
        """
        window = self.open_global_panel("test-global")
        self.wait(lambda: window.find_elements(self.By.CSS_SELECTOR, "input[type=radio]"))
        window.find_element(self.By.CSS_SELECTOR, "input[type=radio][value=b]").click()
        self.wait(
            lambda: "b" in [r.text for r in window.find_elements(self.By.CSS_SELECTOR, "tbody td")]
        )
        window.find_element(self.By.CSS_SELECTOR, ".panel-controls button").click()
        self.wait(lambda: self.server.state()["held"])
        self.wait(lambda: "Held" in self.text("#clock"))
        window.find_element(self.By.CSS_SELECTOR, ".panel-controls button").click()
        self.wait(lambda: not self.server.state()["held"])

    def chart_box(self):
        """Viewport box of the first chart's hit area, read in one call (charts re-render every second).

        @param self The object pointer.
        @return The bounding box as a dictionary.
        """
        return self.js(
            "return document.querySelector('.panel-window .plot-hit').getBoundingClientRect().toJSON()"
        )

    def pointer(self, steps):
        """Run pointer steps at viewport coordinates, without element references.

        @param self The object pointer.
        @param steps List of ("move", x, y), ("down",) or ("up",) tuples.
        """
        from selenium.webdriver.common.actions.action_builder import ActionBuilder

        builder = ActionBuilder(self.driver)
        for step, *args in steps:
            if step == "move":
                builder.pointer_action.move_to_location(int(args[0]), int(args[1]))
            elif step == "down":
                builder.pointer_action.pointer_down()
            elif step == "up":
                builder.pointer_action.pointer_up()
        builder.perform()

    def test_flow_monitor_plots_zoom_and_reset(self):
        """Test flow monitor plots zoom and reset.

        @param self The object pointer.
        """
        self.set_speed(0.7)
        self.wait(lambda: self.server.state()["time"] > 2.5, timeout=60)
        self.open_global_panel("flow-monitor")
        count = lambda selector: self.js(
            "return document.querySelectorAll(arguments[0]).length", selector
        )
        self.wait(lambda: count(".panel-window .plot-svg") == 4)
        self.wait(lambda: count(".panel-window path.series") > 0)
        self.assertGreater(count(".panel-window .legend"), 0)
        box = self.chart_box()
        middle = box["top"] + box["height"] / 2
        self.pointer([("move", box["left"] + box["width"] / 2, middle)])
        tip = lambda: self.js(
            "const t = document.querySelector('.panel-window .plot .plot-tip');"
            "return t && !t.hidden ? t.textContent : null"
        )
        self.wait(lambda: tip())
        self.assertRegex(tip(), r"[kMG]bit/s")
        axis = lambda: self.js(
            "const plot = document.querySelectorAll('.panel-window .plot')[0];"
            "return [...plot.querySelectorAll('text.axis-label')].slice(-5).map(t => t.textContent)"
        )
        before = axis()
        # Drag across a fifth of the chart to zoom in.
        self.pointer(
            [
                ("move", box["left"] + box["width"] * 0.2, middle),
                ("down",),
                ("move", box["left"] + box["width"] * 0.3, middle),
                ("move", box["left"] + box["width"] * 0.4, middle),
                ("up",),
            ]
        )
        self.wait(lambda: count(".panel-window .zoom-reset") == 1)
        self.assertNotEqual(axis(), before)
        time.sleep(1.3)  # survives a panel refresh
        self.assertEqual(count(".panel-window .zoom-reset"), 1)
        self.js("document.querySelector('.panel-window .zoom-reset').click()")
        self.wait(lambda: count(".panel-window .zoom-reset") == 0)
        # Zoom again, then double-click to reset.
        box = self.chart_box()
        middle = box["top"] + box["height"] / 2
        self.pointer(
            [
                ("move", box["left"] + box["width"] * 0.3, middle),
                ("down",),
                ("move", box["left"] + box["width"] * 0.5, middle),
                ("up",),
            ]
        )
        self.wait(lambda: count(".panel-window .zoom-reset") == 1)
        box = self.chart_box()
        x, y = box["left"] + box["width"] / 2, box["top"] + box["height"] / 2
        self.pointer([("move", x, y), ("down",), ("up",), ("down",), ("up",)])
        self.wait(lambda: count(".panel-window .zoom-reset") == 0)
        self.set_speed(-2)

    def test_sidebar_minimizes_to_taskbar(self):
        """Test sidebar minimizes to taskbar.

        @param self The object pointer.
        """
        scene_width = self.find("#scene").rect["width"]
        self.find("aside #minimize-sidebar").click()
        self.wait(lambda: not self.find("aside").is_displayed())
        self.assertGreater(self.find("#scene").rect["width"], scene_width + 200)
        task = self.find(".topology #taskbar .task")
        self.assertEqual(task.text, "Sidebar")
        self.driver.refresh()  # remembered per browser
        self.wait(lambda: not self.find("aside").is_displayed())
        self.find(".topology #taskbar .task").click()
        self.wait(lambda: self.find("aside").is_displayed())
        self.assertFalse(self.find_all("#taskbar .task"))

    def test_top_bar_is_one_row(self):
        """Test top bar is one row.

        @param self The object pointer.
        """
        # With a minimized window, which must not end up in the toolbar.
        self.open_global_panel("test-global")
        self.find(".panel-window button.minimize").click()
        for width in (1600, 1400, 1280, 1100, 1024):
            self.driver.set_window_size(width, 1000)
            time.sleep(0.3)
            height, centres = self.js(
                "const h = document.querySelector('body > header');"
                "return [h.getBoundingClientRect().height,"
                " [...h.children].filter(e => e.offsetParent !== null)"
                ".map(e => Math.round(e.getBoundingClientRect().top + e.getBoundingClientRect().height / 2))]"
            )
            self.assertLess(height, 50, width)
            self.assertLessEqual(max(centres) - min(centres), 3, (width, centres))
            # Every control and the whole status line fit; nothing overflows.
            self.assertFalse(
                self.js(
                    "const c = document.querySelector('#clock'); return c.scrollWidth > c.clientWidth"
                ),
                width,
            )
            overflow = self.js(
                "const h = document.querySelector('body > header');"
                "return [document.documentElement.scrollWidth - window.innerWidth,"
                " Math.max(...[...h.children].map(e => e.getBoundingClientRect().right)) - h.getBoundingClientRect().right]"
            )
            self.assertLessEqual(overflow[0], 0, width)
            self.assertLessEqual(overflow[1], 0, width)
            self.assertTrue(self.find("aside #minimize-sidebar").is_displayed())
            # Minimized windows wait in the top left corner of the topology.
            corner = self.js(
                "const t = document.querySelector('#taskbar .task').getBoundingClientRect();"
                "const s = document.querySelector('#scene').getBoundingClientRect();"
                "return [t.left - s.left, t.top - s.top, !!document.querySelector('body > header #taskbar')]"
            )
            self.assertLess(corner[0], 20)
            self.assertLess(corner[1], 20)
            self.assertFalse(corner[2])
        self.driver.set_window_size(1600, 1000)
        time.sleep(0.5)  # let the layout settle before the next test

    def test_battery_click_opens_charge_history(self):
        """Test battery click opens charge history.

        @param self The object pointer.
        """
        self.set_speed(0.7)
        self.wait(lambda: len(self.find_all(".battery")) == 2)
        self.server.control(action="pause")
        point = self.js(
            "const r = document.querySelector('.node-group[data-node=\"5\"] .battery-body').getBoundingClientRect();"
            "const s = document.querySelector('#scene').getBoundingClientRect();"
            "return [r.left + r.width / 2 - (s.left + s.width / 2), r.top + r.height / 2 - (s.top + s.height / 2)]"
        )
        self.ActionChains(self.driver).move_to_element_with_offset(
            self.find("#scene"), int(point[0]), int(point[1])
        ).click().perform()
        self.wait(lambda: any("Battery - node 5" in w.text for w in self.find_all(".panel-window")))
        window = next(w for w in self.find_all(".panel-window") if "Battery - node 5" in w.text)
        self.wait(lambda: len(window.find_elements(self.By.CSS_SELECTOR, ".plot-svg")) == 2)
        self.assertIn("Drainers", window.text)
        self.assertIn("WifiRadioEnergyModel", window.text)
        self.server.control(action="play")

    def test_energy_panel(self):
        """Test energy panel.

        @param self The object pointer.
        """
        window = self.open_global_panel("energy")
        self.wait(lambda: "Energy at the start" in window.text)
        self.wait(lambda: len(window.find_elements(self.By.CSS_SELECTOR, ".plot-svg")) == 2)
        for title in (
            "Charge per node",
            "Total remaining energy",
            "Sources",
            "Drainers",
            "Charge sources",
        ):
            self.assertIn(title, window.text)

    def test_types_tab(self):
        """Test types tab.

        @param self The object pointer.
        """
        self.find('[data-tab="types"]').click()
        self.wait(lambda: self.find_all("#types .type"))
        in_use = len(self.find_all("#types .type"))
        self.find("#types-in-use").click()
        self.wait(lambda: len(self.find_all("#types .type")) > in_use)
        search = self.find("#type-filter")
        search.send_keys("PointToPointNetDevice")
        self.wait(lambda: len(self.find_all("#types .type")) <= 3)
        next(
            b for b in self.find_all("#types .type") if b.text == "ns3::PointToPointNetDevice"
        ).click()
        self.wait(lambda: "Attributes (" in self.text("#details"))
        self.assertIn("Trace sources", self.text("#details"))
        self.assertIn("Cppyy methods", self.text("#details"))
        self.assertTrue(self.find_all("#details button"))

    def test_legend(self):
        """Test the legend explains the symbols and can be collapsed.

        @param self The object pointer.
        """
        self.assertIn("full queue", self.text("#legend"))
        self.assertIn("TTL expired", self.text("#legend"))
        self.find("#legend summary").click()
        self.assertFalse(self.find("#legend ul").is_displayed())
        self.find("#legend summary").click()

    def test_theme_button(self):
        """Test the theme button switches the theme and remembers it.

        @param self The object pointer.
        """
        background = "return getComputedStyle(document.documentElement).backgroundColor"
        self.js("document.documentElement.dataset.theme = 'dark'")
        self.addCleanup(self.js, "localStorage.removeItem('pyviz-theme')")
        dark = self.js(background)
        self.find("#theme-toggle").click()
        self.assertNotEqual(self.js(background), dark)
        self.assertEqual(self.js("return localStorage.getItem('pyviz-theme')"), "light")
        self.assertEqual(self.text("#theme-toggle"), "Dark")
        self.find("#theme-toggle").click()
        self.assertEqual(self.js(background), dark)
        self.assertEqual(self.text("#theme-toggle"), "Light")

    def test_console_tab(self):
        """Test console tab.

        @param self The object pointer.
        """
        self.click_node(1)
        self.find('[data-tab="console"]').click()
        self.find("#console-input").send_keys("node.GetId() + 100")
        self.find("#console-run").click()
        self.wait(lambda: "101" in self.text("#console-output"))
        self.find("#console-input").send_keys("raise ValueError('boom')")
        self.find("#console-input").send_keys(self.Keys.SHIFT, self.Keys.ENTER)
        self.wait(lambda: "ValueError: boom" in self.text("#console-output"))

    def test_zoom_pan_fit_and_drag(self):
        """Test zoom pan fit and drag.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        start = self.node_info(1)
        self.js(
            "document.querySelector('#scene').dispatchEvent(new WheelEvent('wheel', {deltaY: -500, clientX: 400, clientY: 400, bubbles: true, cancelable: true}))"
        )
        # Zooming changes positions but keeps marks the same size on screen.
        self.wait(lambda: abs(self.node_info(1)["x"] - start["x"]) > 5)
        self.assertAlmostEqual(self.node_info(1)["width"], start["width"], delta=0.5)
        self.find("#zoom-fit").click()
        self.wait(lambda: abs(self.node_info(1)["x"] - start["x"]) < 3)
        # Panning: drag on empty background (the top left corner of the scene).
        scene = self.find("#scene")
        offset_x, offset_y = -scene.rect["width"] / 2 + 8, -scene.rect["height"] / 2 + 8
        (
            self.ActionChains(self.driver)
            .move_to_element_with_offset(scene, int(offset_x), int(offset_y))
            .click_and_hold()
            .move_by_offset(60, 40)
            .release()
            .perform()
        )
        self.wait(lambda: abs(self.node_info(1)["x"] - start["x"] - 60) < 4)
        self.find("#zoom-fit").click()
        self.wait(lambda: abs(self.node_info(1)["x"] - start["x"]) < 3)
        # Dragging a node moves it in the simulation too. Node 7 is placed by
        # the layout, outside the bounds this scenario sets, so bring it into
        # view first.
        self.server.control(action="move", node=7, x=60.0, y=30.0)
        self.wait(lambda: self.js("return state.nodes[7].x") == 60)
        before = self.server.state()["nodes"][7]
        self.at_node(7).click_and_hold().move_by_offset(40, 0).move_by_offset(
            40, 0
        ).release().perform()
        self.wait(lambda: self.server.state()["nodes"][7]["x"] > before["x"])
        self.server.control(action="play")

    def drag_node(self, node, dx, dy, release=True):
        """Drag a node by (dx, dy) pixels with real pointer events; return the label shown mid-drag.

        @param self The object pointer.
        @param node Node index.
        @param dx Horizontal offset in pixels.
        @param dy Vertical offset in pixels.
        @param release Whether to release the pointer at the end.
        @return (label, pointer): label text and pointer position in simulation coordinates.
        """
        from selenium.webdriver.common.actions.action_builder import ActionBuilder

        info = self.node_info(node)
        # Keep the end point inside the viewport, or WebDriver rejects the move.
        width, height = self.js("return [window.innerWidth, window.innerHeight]")
        dx = min(max(dx, 2 - info["x"]), width - 2 - info["x"])
        dy = min(max(dy, 2 - info["y"]), height - 2 - info["y"])
        builder = ActionBuilder(self.driver)
        builder.pointer_action.move_to_location(int(info["x"]), int(info["y"]))
        builder.pointer_action.pointer_down()
        builder.pointer_action.move_to_location(int(info["x"] + dx / 2), int(info["y"] + dy / 2))
        builder.pointer_action.move_to_location(int(info["x"] + dx), int(info["y"] + dy))
        builder.perform()
        label = self.js(
            "const l = document.querySelector('#drag-coords'); return l.hidden ? null : l.textContent"
        )
        # Where the pointer is, in simulation coordinates, per the page's own transform.
        pointer = self.js(
            "const r = document.querySelector('#scene').getBoundingClientRect();"
            "return toWorld(arguments[0] - r.left, arguments[1] - r.top)",
            int(info["x"] + dx),
            int(info["y"] + dy),
        )
        if release:
            builder = ActionBuilder(self.driver)
            builder.pointer_action.pointer_up()
            builder.perform()
        return label, pointer

    @staticmethod
    def parse_coordinates(label):
        """Parse a drag coordinates label.

        @param label Coordinates label text shown while dragging.
        @return List [x, y, z] of floats.
        """
        match = re.fullmatch(r"\((-?\d+\.\d\d), (-?\d+\.\d\d), (-?\d+\.\d\d)\) m", label or "")
        assert match, label
        return [float(v) for v in match.groups()]

    def test_drag_shows_coordinates(self):
        """Test drag shows coordinates.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        self.server.control(action="move", node=7, x=60.0, y=30.0)
        self.wait(lambda: self.js("return state.nodes[7].x") == 60)
        self.assertTrue(self.js("return document.querySelector('#drag-coords').hidden"))
        label, pointer = self.drag_node(7, 60, -20)
        x, y, z = self.parse_coordinates(label)
        # The label is the position under the pointer, to the displayed precision.
        self.assertAlmostEqual(x, pointer[0], delta=0.01)
        self.assertAlmostEqual(y, pointer[1], delta=0.01)
        self.assertEqual(z, 0.0)
        self.wait(lambda: self.js("return document.querySelector('#drag-coords').hidden"))
        # The node is dropped where the label said.
        self.wait(lambda: abs(self.server.state()["nodes"][7]["x"] - x) < 0.01)
        self.assertAlmostEqual(self.server.state()["nodes"][7]["y"], y, delta=0.01)
        self.server.control(action="play")

    def test_drag_keeps_and_shows_height(self):
        """Test drag keeps and shows height.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        # Node 1 has a constant position model; lift it to z = 5 m.
        self.server.console(
            "ns.NodeList.GetNode(1).GetObject[ns.MobilityModel]().SetPosition(ns.Vector(40, 0, 5))"
        )
        self.wait(lambda: self.js("return state.nodes[1].z") == 5)
        label, pointer = self.drag_node(1, 50, 30)
        x, y, z = self.parse_coordinates(label)
        self.assertEqual(z, 5.0)
        self.assertAlmostEqual(x, pointer[0], delta=0.01)
        self.wait(lambda: abs(self.server.state()["nodes"][1]["x"] - x) < 0.01)
        self.assertEqual(self.server.state()["nodes"][1]["z"], 5.0)  # height kept
        position = self.server.console(
            "p = ns.NodeList.GetNode(1).GetObject[ns.MobilityModel]().GetPosition(); print(p.x, p.y, p.z)"
        )
        self.assertEqual([round(float(v), 2) for v in position.split()], [x, y, 5.0])
        # The tests share one simulation: put node 1 back.
        self.server.console(
            "ns.NodeList.GetNode(1).GetObject[ns.MobilityModel]().SetPosition(ns.Vector(40, 0, 0))"
        )
        self.server.control(action="play")

    def test_drag_bounded_node_outside_its_area(self):
        """Test drag bounded node outside its area.

        @param self The object pointer.
        """
        self.server.control(action="pause")
        label, pointer = self.drag_node(5, -400, 300)  # far outside x 110..150, y -30..30
        self.assertGreater(abs(pointer[0] - 130), 20)
        self.wait(lambda: self.js("return document.querySelector('#drag-coords').hidden"))
        node = self.server.state()["nodes"][5]
        self.assertGreaterEqual(node["x"], 110.0)
        self.assertLessEqual(node["x"], 150.0)
        self.assertGreaterEqual(node["y"], -30.0)
        self.assertLessEqual(node["y"], 30.0)
        self.server.control(action="play")
        self.server.run_to(self.server.state()["time"] + 0.5)  # still alive and moving

    def test_no_coordinates_when_panning(self):
        """Test no coordinates when panning.

        @param self The object pointer.
        """
        scene = self.find("#scene")
        offset_x, offset_y = -scene.rect["width"] / 2 + 8, -scene.rect["height"] / 2 + 8
        (
            self.ActionChains(self.driver)
            .move_to_element_with_offset(scene, int(offset_x), int(offset_y))
            .click_and_hold()
            .move_by_offset(40, 20)
            .perform()
        )
        self.assertTrue(self.js("return document.querySelector('#drag-coords').hidden"))
        self.ActionChains(self.driver).release().perform()
        self.find("#zoom-fit").click()

    def test_follow_node(self):
        """Test follow node.

        @param self The object pointer.
        """
        self.server.control(action="pause")  # so the node stays under the pointer
        self.at_node(5).context_click().perform()
        self.wait(lambda: self.find("#node-menu").is_displayed())
        next(b for b in self.find_all("#node-menu button") if b.text == "Follow node").click()
        self.server.control(action="play")
        rect = self.js("return document.querySelector('#scene').getBoundingClientRect().toJSON()")
        self.set_speed(0.7)
        for _ in range(5):
            time.sleep(0.4)
            info = self.node_info(5)
            self.assertAlmostEqual(info["x"], rect["left"] + rect["width"] / 2, delta=4)
            self.assertAlmostEqual(info["y"], rect["top"] + rect["height"] / 2, delta=4)
        self.find("#zoom-fit").click()
        self.set_speed(-2)

    def test_screenshot_download(self):
        """Test screenshot download.

        @param self The object pointer.
        """
        self.js(
            "window.__downloads = []; const orig = HTMLAnchorElement.prototype.click;"
            "HTMLAnchorElement.prototype.click = function() { window.__downloads.push(this.download); };"
        )
        self.find("#screenshot").click()
        downloads = self.js("return window.__downloads")
        self.assertEqual(len(downloads), 1)
        self.assertRegex(downloads[0], r"^pyviz-[\d.]+s\.svg$")


def parse_command_line(argv):
    """Parse the Selenium options; return the remaining arguments for unittest.

    Each option sets the environment variable of the same meaning (see the
    module docstring), which therefore remain usable as well.

    @param argv Full command line, including the program name.
    @return argv without the Selenium options.
    """
    import argparse

    parser = argparse.ArgumentParser(
        description="PyViz web visualizer tests. Other arguments (-v, test names) go to unittest.",
        add_help=False,
    )
    parser.add_argument(
        "--require-selenium", action="store_true", help="fail instead of skipping the browser tests"
    )
    parser.add_argument("--chrome-binary", help="Chrome or Chromium executable")
    parser.add_argument("--chromedriver", help="chromedriver executable")
    parser.add_argument("--webdriver-url", help="URL of a running chromedriver")
    parser.add_argument(
        "--selenium-debugger", help="host:port of a browser started with --remote-debugging-port"
    )
    parser.add_argument("--help-options", action="help", help="show these options and exit")
    options, rest = parser.parse_known_args(argv[1:])
    for name, variable in (
        ("chrome_binary", "PYVIZ_CHROME_BINARY"),
        ("chromedriver", "PYVIZ_CHROMEDRIVER"),
        ("webdriver_url", "PYVIZ_WEBDRIVER_URL"),
        ("selenium_debugger", "PYVIZ_SELENIUM_DEBUGGER"),
    ):
        if getattr(options, name):
            os.environ[variable] = getattr(options, name)
    if options.require_selenium:
        os.environ["PYVIZ_REQUIRE_SELENIUM"] = "1"
    return [argv[0]] + rest


if __name__ == "__main__":
    unittest.main(argv=parse_command_line(sys.argv))

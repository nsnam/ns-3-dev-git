"""PyViz: live, browser based visualizer for ns-3 Python simulations."""


def start():
    """Open the HTML5 visualizer and run the configured simulation.

    @return The result of the web visualizer start function.
    """
    from .web import start

    return start()


def add_initialization_hook(hook, *args):
    """Call hook(viz, *args) once the visualizer is attached to the simulation.

    @param hook Callable invoked as hook(viz, *args).
    @param args Extra positional arguments passed to the hook.
    @return The result of the web add_initialization_hook function.
    """
    from .web import add_initialization_hook as add_hook

    return add_hook(hook, *args)


def register_plugin(plugin):
    """Call plugin(viz) when the visualizer starts.

    @param plugin Callable invoked as plugin(viz).
    @return The result of the web register_plugin function.
    """
    from .web import register_plugin as register

    return register(plugin)


def enable_flow_monitor():
    """Plot per-flow statistics (throughput, bytes, latency, jitter) in the browser.

    @return The result of the web enable_flow_monitor function.
    """
    from .web import enable_flow_monitor as enable

    return enable()


def set_speed(speed):
    """Set the initial playback speed, in simulated seconds per wall-clock second.

    @param speed Speed, from 0.01 to 100.
    @return The result of the web set_speed function.
    """
    from .web import set_speed as set_visual_speed

    return set_visual_speed(speed)


def skip_to(seconds):
    """Run up to a simulated time, without animating it, before the page shows it.

    @param seconds Simulated time.
    @return The result of the web skip_to function.
    """
    from .web import skip_to as skip

    return skip(seconds)


def set_bounds(x1, y1, x2, y2):
    """Set the initially visible area, in simulation coordinates.

    @param x1 Minimum x coordinate.
    @param y1 Minimum y coordinate.
    @param x2 Maximum x coordinate.
    @param y2 Maximum y coordinate.
    @return The result of the web set_bounds function.
    """
    from .web import set_bounds as set_visual_bounds

    return set_visual_bounds(x1, y1, x2, y2)

.. _sionna-rt-installation:

Sionna-RT Installation
======================

This chapter describes how to install the Sionna-RT extension for ns-3.
It provides the entry point for the Sionna-RT installation process, including
prerequisites, build instructions, and verification steps.

Installation Steps for Ubuntu
=============================

Follow these steps to create a Python 3.12 virtual environment, install Python
dependencies, and build ns-3 with Python bindings.
"Instructions are for Python 3.12 because that is the version we have tested; other versions may or may not work."

Prerequisites
-------------

Update your system and install the required packages:

.. code-block:: bash

   sudo apt update && sudo apt upgrade -y
   sudo apt install -y \
       build-essential git \
       python3.12 python3.12-venv python3.12-dev \
       cmake pkg-config \
       libsqlite3-dev libxml2-dev libbz2-dev \
       libssl-dev libc-ares-dev libgsl-dev libgtk-3-dev

Clone ns-3
----------

.. code-block:: bash

   cd ns3dev

Set Up the Python Environment
------------------------------

Create and activate a Python 3.12 virtual environment, then install the
required Python packages:

.. code-block:: bash

   python3.12 -m venv ns3venv
   source ns3venv/bin/activate
   python -m pip install --upgrade pip
   pip install \
       sionna==1.2.0 \
       sionna-rt==1.2.0 \
       pybind11==2.11.1 \
       cppyy==3.5.0

.. note::

   The virtual environment must remain active for all subsequent ``./ns3``
   commands so that the build system picks up the correct Python interpreter
   and installed packages.

Configure and Build
-------------------

Run the configure step from within the active ``ns3venv``, then build:
you must see the message "Sionna-RT support enabled: all required dependencies were found." in the output of the configure step for Sionna-RT to be available in the build.

.. code-block:: bash

   ./ns3 configure --enable-examples --enable-tests --enable-python-bindings
   ./ns3 build

Run the Example
---------------

Once the build completes, run the Sionna RT channel example to verify the
installation:

.. code-block:: bash

   ./ns3 run sionna-rt-channel-example

.. _sionna-rt-expected-output:

Expected Output
===============

When the example runs successfully, you will see output in three parts.

**Python environment banner** — printed before the simulation starts,
confirming which Python interpreter is in use.  This lets you verify
that ns-3 picked up the correct virtual environment:

.. code-block:: text

   Python executable: /home/user/ns3dev/ns3venv/bin/python3.12
   Python version: 3.12.x (main, ...) [GCC ...]

   --- Sionna-RT Channel Example ---

**Per-iteration SNR** — one line per time step, produced after each call
to the Sionna RT ray-tracing engine.  Each line shows the simulated time,
the signal-to-noise ratio (SNR) in dB, and the received power in dBm:

.. code-block:: text

     [t=0.000s]  SNR = 23.41 dB  |  Rx power = -67.52 dBm
     [t=0.010s]  SNR = 22.87 dB  |  Rx power = -68.06 dBm

The SNR reflects the full channel computed by Sionna RT, including all
configured propagation effects (line-of-sight, reflections, diffraction,
etc.) and the DFT beamforming gain applied at both ends.

**Success summary** — printed after all events have been processed,
confirming that the complete ray-tracing pipeline ran without errors:

.. code-block:: text

   [SUCCESS] Simulation completed successfully.
     Computed 2 SNR sample(s), average SNR = 23.14 dB
     Results written to snr-trace.txt

The file ``snr-trace.txt`` is written to the working directory.  It
contains one row per time step: the simulation time in seconds followed
by the SNR in dB:

.. code-block:: text

   0.0 23.41
   0.01 22.87

Failure output
--------------

If the Python environment is not active or the Sionna package is missing,
the example prints a ``[FAILURE]`` message with a description of the
problem:

.. code-block:: text

   [FAILURE] Sionna/Python error during simulation:
     ModuleNotFoundError: No module named 'sionna'
     Check that the Sionna virtual environment is active and the
     scene name (--Scenario) is correct.

In this case, verify that the virtual environment is active
(``source ns3venv/bin/activate`` on Ubuntu, or
``source .venv/bin/activate`` on macOS) and that the following command
succeeds before re-running the example:

.. code-block:: bash

   python -c "import sionna; print('sionna OK', sionna.__version__)"


Installation Steps for macOS
=============================================

Follow these steps to set up a Conda environment with Python 3.12 and
prepare the LLVM backend required by Sionna RT's Dr.Jit ray-tracing engine.
"The instruction are not limited to Mac OS, but they are tested on Mac OS.  On Linux, you can use the same steps to set up a Conda environment and install the dependencies, but you can skip the LLVM backend configuration and use GPU acceleration directly if you have a compatible NVIDIA GPU."

.. note::

   These instructions target **Apple** Macs. NVIDIA CUDA is
   not available on macOS, so Sionna RT will run on CPU via the LLVM backend.
   Expect longer simulation times compared to a Linux system with a GPU.

Before proceeding, ensure that the system-level build tools (CMake, Ninja,
git, etc.) are already installed.  See :ref:`macOS` for instructions on
installing them via Homebrew or MacPorts.

Install Miniconda
-----------------

Download and install the arm64 Miniconda installer:

.. code-block:: bash

   curl -O https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-arm64.sh
   bash ./Miniconda3-latest-MacOSX-arm64.sh -b -c -u

Reload your shell configuration so that the ``conda`` command is available:

.. code-block:: bash

   source ~/.zshrc

Set Up the Conda Environment
-----------------------------

Create and activate a dedicated Python 3.12 environment, then install the
required packages including ``cppyy`` and the LLVM 17 library:
"Instructions are for Python 3.12 because that is the version we have tested; other versions may or may not work."

.. code-block:: bash

   conda create -n venv python=3.12 -y
   conda activate venv
   conda install pip \
       conda-forge::cppyy=3.5.0 \
       conda-forge::libllvm17 \
       -y

.. note::

   ``libllvm17`` provides the LLVM shared library that Dr.Jit (the JIT
   backend used by Mitsuba 3 / Sionna RT) requires for CPU-mode ray tracing
   on macOS.

Configure the LLVM Backend
---------------------------

Export the path to the LLVM shared library so that Dr.Jit can locate it at
runtime. Add this line to your ``~/.zshrc`` (or run it in every new shell
session before building or running ns-3):

.. code-block:: bash

   export DRJIT_LIBLLVM_PATH="$CONDA_PREFIX/lib/libLLVM-17.dylib"

.. tip::

   To make this permanent, append the line above to ``~/.zshrc`` and run
   ``source ~/.zshrc``.

Teardown (Optional)
--------------------

To remove the Conda environment after you are done, or to start fresh:

.. code-block:: bash

   source ~/.zshrc
   conda deactivate
   conda remove --name venv --all -y

Installation Steps using uv
===========================

`uv <https://github.com/astral-sh/uv>`_ is a fast Python package manager
that can install Python interpreters and manage virtual environments.
Follow these steps to set up a Python 3.12 virtual environment with ``uv``,
install Sionna RT, and build ns-3 with Python bindings.

.. note::

   These instructions work on both Linux and macOS.  On macOS, NVIDIA CUDA
   is not available, so Sionna RT will run on CPU via the LLVM backend.
   The LLVM backend configuration steps below are macOS-specific; Linux
   users with a CUDA-capable GPU can skip that section and use GPU
   acceleration directly.

Clone ns-3
----------

.. code-block:: bash

   cd ns3dev/

Set Up the Python Environment
------------------------------

Install ``uv`` if it is not already available.  On macOS with Homebrew:

.. code-block:: bash

   brew install uv

On Linux (and as an alternative on macOS):

.. code-block:: bash

   curl -LsSf https://astral.sh/uv/install.sh | sh

Then install Python 3.12 and create a virtual environment:

.. code-block:: bash

   uv python install 3.12 >/dev/null

Create and activate a virtual environment inside the repository root:

.. code-block:: bash

   uv venv --python 3.12
   source .venv/bin/activate

Install Python Dependencies
-----------------------------

Upgrade the toolchain and install Sionna RT along with its required bindings:

.. code-block:: bash

   PY="$(pwd)/.venv/bin/python"

   "$PY" -m ensurepip --upgrade
   "$PY" -m pip install --upgrade pip setuptools wheel
   "$PY" -m pip install cppyy pybind11 "sionna==1.2.0" "sionna-rt==1.2.0"

Verify the Sionna installation:

.. code-block:: bash

   "$PY" -c "import sionna; print('sionna OK', sionna.__version__)"

Expose the virtual environment's site-packages to the build system:

.. code-block:: bash

   SP="$("$PY" -c "import site; print(site.getsitepackages()[0])")"
   export PYTHONPATH="$SP"

Configure the LLVM Backend
---------------------------

Install LLVM 17 via Homebrew and export the paths required by Dr.Jit (the
JIT backend used by Mitsuba 3 / Sionna RT) for CPU-mode ray tracing:

.. code-block:: bash

   brew install llvm@17
   LLVM_PREFIX="$(brew --prefix llvm@17)"

   export DRJIT_LIBLLVM_PATH="$LLVM_PREFIX/lib/libLLVM.dylib"
   export DYLD_FALLBACK_LIBRARY_PATH="/opt/homebrew/lib:${DYLD_FALLBACK_LIBRARY_PATH}"

.. tip::

   To avoid re-exporting these variables in every new shell session, append
   the two ``export`` lines above to your ``~/.zshrc``.

Build ns-3
----------

Clean any previous build artefacts, configure with Ninja and Python bindings
enabled, then build:
you must see the message "Sionna-RT support enabled: all required dependencies were found." in the output of the configure step for Sionna-RT to be available in the build.
.. code-block:: bash

   ./ns3 clean

   "$PY" ./ns3 configure -G Ninja \
       --enable-python-bindings \
       --enable-examples \
       --filter-module-examples-and-tests=spectrum

   ./ns3 build

.. note::

   Passing ``"$PY"`` explicitly to ``./ns3 configure`` ensures the build
   system uses the virtual environment's interpreter rather than the system
   Python.

Run the Example
---------------

Once the build completes, run the Sionna RT channel example to verify the
full installation:

.. code-block:: bash

   "$PY" ./ns3 run sionna-rt-channel-example

The expected output is identical to the section :ref:`sionna-rt-expected-output`.

Failure output
--------------

If the Python environment is not active or the Sionna package is missing,
the example prints a ``[FAILURE]`` message with a description of the
problem:

.. code-block:: text

   [FAILURE] Sionna/Python error during simulation:
     ModuleNotFoundError: No module named 'sionna'
     Check that the Sionna virtual environment is active and the
     scene name (--Scenario) is correct.

In this case, verify that the virtual environment is active
(``source ns3venv/bin/activate`` on Ubuntu, or
``source .venv/bin/activate`` on macOS) and that the following command
succeeds before re-running the example:

.. code-block:: bash

   python -c "import sionna; print('sionna OK', sionna.__version__)"

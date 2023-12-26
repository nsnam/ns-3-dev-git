import sys
import sysconfig

import cmake_build_extension
import setuptools

setuptools.setup(
    cmdclass=dict(build_ext=cmake_build_extension.BuildExtension),
    packages=["ns", "visualizer"],
    package_dir={
        "ns": "./build-support/pip-wheel/ns",
        "visualizer": "./build-support/pip-wheel/visualizer",
    },
    ext_modules=[
        cmake_build_extension.CMakeExtension(
            name="BuildAndInstall",
            install_prefix="ns3",
            cmake_configure_options=[
                "-DCMAKE_BUILD_TYPE:STRING=release",
                "-DNS3_ASSERT:BOOL=ON",
                "-DNS3_LOG:BOOL=ON",
                "-DNS3_WARNINGS_AS_ERRORS:BOOL=OFF",
                "-DNS3_PYTHON_BINDINGS:BOOL=ON",
                "-DNS3_BINDINGS_INSTALL_DIR:STRING=INSTALL_PREFIX",
                "-DNS3_FETCH_OPTIONAL_COMPONENTS:BOOL=ON",
                "-DNS3_PIP_PACKAGING:BOOL=ON",
                # Make CMake find python components from the currently running python
                # https://catherineh.github.io/programming/2021/11/16/python-binary-distributions-whls-with-c17-cmake-auditwheel-and-manylinux
                f"-DPython3_LIBRARY_DIRS={sysconfig.get_config_var('LIBDIR')}",
                f"-DPython3_INCLUDE_DIRS={sysconfig.get_config_var('INCLUDEPY')}",
                f"-DPython3_EXECUTABLE={sys.executable}",
            ],
        ),
    ],
)

set(name visualizer)

set(source_files model/pyviz.cc model/visual-simulator-impl.cc)

set(header_files model/pyviz.h model/visual-simulator-impl.h)

include_directories(${Python3_INCLUDE_DIRS})

set(libraries_to_link
    ${Python3_LIBRARIES}
    ${libcore}
    ${libinternet}
    ${libwifi}
    ${libpoint-to-point}
    ${libcsma}
    ${libbridge}
    ${libwimax}
    ${liblte}
    ${libmesh}
)

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")

# move visualizer folder to build/bindings/python, which allows us to add only PYTHONPATH=ns-3-dev/build/bindings/python
file(COPY visualizer DESTINATION ${CMAKE_OUTPUT_DIRECTORY}/bindings/python/)

set(source_files
    model/si-units-parser.cc
    model/units-angle.cc
    model/units-power.cc
    model/units-frequency-attributes.cc
    model/units-time.cc
    model/units-ratio.cc
)

set(header_files
    model/si-units-parser.h
    model/si-units.h
    model/units-aliases.h
    model/units-angle.h
    model/units-power.h
    model/units-frequency-attributes.h
    model/units-frequency.h
    model/units-time.h
    model/units-ratio.h
)

build_lib(
  LIBNAME si-units
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK ${libcore}
  TEST_SOURCES test/si-units-test-suite.cc
)

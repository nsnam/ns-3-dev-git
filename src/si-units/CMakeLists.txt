set(source_files
    model/string-utils.cc
    model/units-angle.cc
    model/units-energy.cc
    model/units-frequency.cc
    model/units-time.cc
    model/units-ratio.cc
)

set(header_files
    model/format-string.h
    model/si-units.h
    model/string-utils.h
    model/units-aliases.h
    model/units-angle.h
    model/units-energy.h
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

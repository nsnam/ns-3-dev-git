build_lib(
        LIBNAME test-contrib-dependency
        SOURCE_FILES contrib-source.cc
        HEADER_FILES contrib-header.h
        LIBRARIES_TO_LINK ${libcore}
)

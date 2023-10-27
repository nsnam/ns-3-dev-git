build_lib(
        LIBNAME test-src-dependent-on-contrib
        SOURCE_FILES src-source.cc
        HEADER_FILES src-header.h
        LIBRARIES_TO_LINK ${libcore}
                          ${libtest-contrib-dependency}
)


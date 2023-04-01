build_lib_example(
        NAME source-example
        SOURCE_FILES source-example.cc
        LIBRARIES_TO_LINK
        ${libtest-src-dependant-on-contrib}
)

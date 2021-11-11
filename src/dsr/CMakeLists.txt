set(name dsr)
set(source_files
    helper/dsr-helper.cc
    helper/dsr-main-helper.cc
    model/dsr-errorbuff.cc
    model/dsr-fs-header.cc
    model/dsr-gratuitous-reply-table.cc
    model/dsr-maintain-buff.cc
    model/dsr-network-queue.cc
    model/dsr-option-header.cc
    model/dsr-options.cc
    model/dsr-passive-buff.cc
    model/dsr-rcache.cc
    model/dsr-routing.cc
    model/dsr-rreq-table.cc
    model/dsr-rsendbuff.cc
)

set(header_files
    helper/dsr-helper.h
    helper/dsr-main-helper.h
    model/dsr-errorbuff.h
    model/dsr-fs-header.h
    model/dsr-gratuitous-reply-table.h
    model/dsr-maintain-buff.h
    model/dsr-network-queue.h
    model/dsr-option-header.h
    model/dsr-options.h
    model/dsr-passive-buff.h
    model/dsr-rcache.h
    model/dsr-routing.h
    model/dsr-rreq-table.h
    model/dsr-rsendbuff.h
)

set(libraries_to_link ${libinternet} ${libwifi})

set(test_sources test/dsr-test-suite.cc)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")

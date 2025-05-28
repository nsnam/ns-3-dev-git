build_lib(
  LIBNAME coex
  SOURCE_FILES
    helper/coex-helper.cc
    model/coex-arbitrator.cc
    model/device-coex-manager.cc
    model/wifi-coex-arbitrator.cc
  HEADER_FILES
    helper/coex-helper.h
    model/coex-arbitrator.h
    model/coex-types.h
    model/device-coex-manager.h
    model/wifi-coex-arbitrator.h
  LIBRARIES_TO_LINK ${libnetwork}
)

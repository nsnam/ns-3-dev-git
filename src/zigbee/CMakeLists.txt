build_lib(
  LIBNAME zigbee
  SOURCE_FILES
    helper/zigbee-helper.cc
    helper/zigbee-stack-container.cc
    model/zigbee-aps-header.cc
    model/zigbee-aps-tables.cc
    model/zigbee-aps.cc
    model/zigbee-group-table.cc
    model/zigbee-nwk-fields.cc
    model/zigbee-nwk.cc
    model/zigbee-stack.cc
    model/zigbee-nwk-header.cc
    model/zigbee-nwk-payload-header.cc
    model/zigbee-nwk-tables.cc
  HEADER_FILES
    helper/zigbee-helper.h
    helper/zigbee-stack-container.h
    model/zigbee-aps-header.h
    model/zigbee-aps-tables.h
    model/zigbee-aps.h
    model/zigbee-group-table.h
    model/zigbee-nwk-fields.h
    model/zigbee-nwk.h
    model/zigbee-stack.h
    model/zigbee-nwk-header.h
    model/zigbee-nwk-payload-header.h
    model/zigbee-nwk-tables.h
  LIBRARIES_TO_LINK ${liblr-wpan}
  TEST_SOURCES test/zigbee-rreq-test.cc
               test/zigbee-aps-data-test.cc
)

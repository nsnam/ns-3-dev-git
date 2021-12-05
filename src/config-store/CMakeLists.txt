set(name config-store)

if(${GTK3_FOUND})
  set(gtk3_sources model/display-functions.cc model/gtk-config-store.cc
                   model/model-node-creator.cc model/model-typeid-creator.cc
  )

  set(gtk3_headers model/gtk-config-store.h)
  include_directories(${GTK3_INCLUDE_DIRS} ${HarfBuzz_INCLUDE_DIRS})
  set(gtk_libraries ${GTK3_LIBRARIES})
endif()

if(${LIBXML2_FOUND})
  set(xml2_sources model/xml-config.cc)
  set(xml2_libraries ${LIBXML2_LIBRARIES})
  include_directories(${LIBXML2_INCLUDE_DIR})
endif()

set(source_files
    ${gtk3_sources}
    ${xml2_sources}
    model/attribute-default-iterator.cc
    model/attribute-iterator.cc
    model/config-store.cc
    model/file-config.cc
    model/raw-text-config.cc
)

set(header_files ${gtk3_headers} model/file-config.h model/config-store.h)

set(libraries_to_link ${libcore} ${libnetwork} ${xml2_libraries}
                      ${gtk_libraries}
)

set(test_sources)

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}"
          "${test_sources}"
)

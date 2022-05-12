if(${GTK3_FOUND})
  set(gtk3_sources
      model/display-functions.cc
      model/gtk-config-store.cc
      model/model-node-creator.cc
      model/model-typeid-creator.cc
  )

  set(gtk3_headers
      model/gtk-config-store.h
  )
endif()

if(${LIBXML2_FOUND})
  set(xml2_sources
      model/xml-config.cc
  )
endif()

build_lib(
  LIBNAME config-store
  SOURCE_FILES
    ${gtk3_sources}
    ${xml2_sources}
    model/attribute-default-iterator.cc
    model/attribute-iterator.cc
    model/config-store.cc
    model/file-config.cc
    model/raw-text-config.cc
  HEADER_FILES
    ${gtk3_headers}
    model/file-config.h
    model/config-store.h
  LIBRARIES_TO_LINK ${libcore}
                    ${libnetwork}
)

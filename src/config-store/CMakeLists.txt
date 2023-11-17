set(gtk3_sources)
set(gtk3_headers)
set(gtk_libraries)
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
  set(gtk_libraries
      ${GTK3_LIBRARIES}
  )
  if(${NS3_PIP_PACKAGING})
    # In case we are packaging ns-3, leave gtk symbols undefined and pray for
    # the linker to find the correct libraries locally
    set(gtk_libraries
        -Wl,--allow-shlib-undefined
    )
  endif()
endif()

set(xml2_sources)
set(xml2_libraries)
if(${LIBXML2_FOUND})
  set(xml2_sources
      model/xml-config.cc
  )
  set(xml2_libraries
      ${LIBXML2_LIBRARIES}
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
  LIBRARIES_TO_LINK
    ${libcore}
    ${xml2_libraries}
    ${gtk_libraries}
)

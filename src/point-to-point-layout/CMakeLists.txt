set(name point-to-point-layout)

set(source_files model/point-to-point-dumbbell.cc model/point-to-point-grid.cc model/point-to-point-star.cc)

set(header_files model/point-to-point-dumbbell.h model/point-to-point-grid.h model/point-to-point-star.h)

set(libraries_to_link ${libinternet} ${libpoint-to-point} ${libmobility})

build_lib("${name}" "${source_files}" "${header_files}" "${libraries_to_link}" "${test_sources}")

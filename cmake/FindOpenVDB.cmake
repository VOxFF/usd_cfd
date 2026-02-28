# FindOpenVDB.cmake
#
# Locate an OpenVDB installation (works with distro packages that have no
# CMake config files, e.g. libopenvdb-dev 8.x on Ubuntu/Debian).
#
# Provides:
#   OpenVDB_FOUND
#   OpenVDB::openvdb  (imported target)

find_path(OpenVDB_INCLUDE_DIR
    NAMES openvdb/openvdb.h
    PATHS /usr/include /usr/local/include
)

find_library(OpenVDB_LIBRARY
    NAMES openvdb
    PATHS /usr/lib /usr/local/lib
    PATH_SUFFIXES x86_64-linux-gnu
)

find_library(_OpenVDB_TBB_LIBRARY
    NAMES tbb
    PATHS /usr/lib /usr/local/lib
          ${USD_ROOT}/lib $ENV{USD_ROOT}/lib
    PATH_SUFFIXES x86_64-linux-gnu
)

find_library(_OpenVDB_Half_LIBRARY
    NAMES Half-2_5 Half
    PATHS /usr/lib /usr/local/lib
    PATH_SUFFIXES x86_64-linux-gnu
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenVDB
    REQUIRED_VARS OpenVDB_INCLUDE_DIR OpenVDB_LIBRARY
)

if(OpenVDB_FOUND AND NOT TARGET OpenVDB::openvdb)
    add_library(OpenVDB::openvdb SHARED IMPORTED)
    set_target_properties(OpenVDB::openvdb PROPERTIES
        IMPORTED_LOCATION             "${OpenVDB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OpenVDB_INCLUDE_DIR}"
    )
    set(_ovdb_deps "")
    if(_OpenVDB_TBB_LIBRARY)
        list(APPEND _ovdb_deps "${_OpenVDB_TBB_LIBRARY}")
    endif()
    if(_OpenVDB_Half_LIBRARY)
        list(APPEND _ovdb_deps "${_OpenVDB_Half_LIBRARY}")
    endif()
    if(_ovdb_deps)
        set_property(TARGET OpenVDB::openvdb APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "${_ovdb_deps}"
        )
    endif()
endif()

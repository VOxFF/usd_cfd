# FindUSD.cmake
#
# Locate an OpenUSD (formerly Pixar USD) installation.
#
# Set PXR_DIR to the root of your USD install, e.g.:
#   cmake .. -DPXR_DIR=/opt/usd
#
# Provides:
#   USD_FOUND
#   USD_INCLUDE_DIRS
#   USD_LIBRARY_DIR
#   USD_LIBRARIES

if(NOT PXR_DIR AND DEFINED ENV{PXR_DIR})
    set(PXR_DIR $ENV{PXR_DIR})
endif()

find_path(USD_INCLUDE_DIRS
    NAMES pxr/usd/usd/stage.h
    HINTS ${PXR_DIR}/include
    PATH_SUFFIXES include
)

find_library(_USD_LIB_usd
    NAMES usd_usd
    HINTS ${PXR_DIR}/lib
)

if(NOT _USD_LIB_usd)
    find_library(_USD_LIB_usd
        NAMES usd
        HINTS ${PXR_DIR}/lib
    )
endif()

if(USD_INCLUDE_DIRS AND _USD_LIB_usd)
    get_filename_component(USD_LIBRARY_DIR ${_USD_LIB_usd} DIRECTORY)

    set(_USD_COMPONENT_NAMES usd usdGeom sdf tf vt gf arch)
    set(USD_LIBRARIES "")

    foreach(_comp ${_USD_COMPONENT_NAMES})
        find_library(_USD_LIB_${_comp}
            NAMES usd_${_comp} ${_comp}
            HINTS ${USD_LIBRARY_DIR}
        )
        if(_USD_LIB_${_comp})
            list(APPEND USD_LIBRARIES ${_USD_LIB_${_comp}})
        endif()
    endforeach()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(USD
    REQUIRED_VARS USD_INCLUDE_DIRS USD_LIBRARIES
)

if(USD_FOUND AND NOT TARGET USD::USD)
    add_library(USD::USD INTERFACE IMPORTED)
    set_target_properties(USD::USD PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${USD_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES "${USD_LIBRARIES}"
        INTERFACE_LINK_DIRECTORIES "${USD_LIBRARY_DIR}"
    )
endif()

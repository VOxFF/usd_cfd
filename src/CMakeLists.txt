add_library(ufc STATIC
    SimConfig.cpp
    WarpSolver.cpp
)

target_include_directories(ufc
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(ufc
    SYSTEM PUBLIC ${PXR_INCLUDE_DIRS}
)

target_compile_definitions(ufc
    PUBLIC TBB_SUPPRESS_DEPRECATED_MESSAGES=1
)

target_compile_options(ufc
    PUBLIC -Wno-deprecated -Wno-cpp
)

target_link_libraries(ufc
    PUBLIC usd usdGeom usdShade sdf tf vt gf arch
           OpenVDB::openvdb
)

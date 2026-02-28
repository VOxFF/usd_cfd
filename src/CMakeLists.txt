add_library(ufc STATIC
    CfdSimulation.cpp
    SimConfig.cpp
    WarpSolver.cpp
)

target_include_directories(ufc
    PUBLIC ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ufc
    PUBLIC ufd
)

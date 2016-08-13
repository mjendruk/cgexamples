
# 
# External dependencies
# 

find_package(GLM REQUIRED)
find_package(GLFW)
find_package(glbinding REQUIRED)

find_package(OpenMP QUIET)

# 
# Executable name and options
# 

# Target name
set(target particles)

# Exit here if required dependencies are not met
if (NOT GLFW_FOUND)
    message("${target} skipped: GLFW not found")
    return()
endif()

if (NOT OPENMP_FOUND)
    message("Loop parallelization in ${target} skipped: OpenMP not found")
endif()

message(STATUS "${target}")


# 
# Sources
# 

set(data "${CMAKE_SOURCE_DIR}/data/${target}")

set(sources
    main.cpp

    particles.cpp
    particles.h
    allocator.h
    allocator.inl
    ${data}/particles.vert
    ${data}/particles.geom
    ${data}/particles.frag
    ${data}/particles-circle.frag
    ${data}/particles-sphere.frag
    ${data}/particles.comp
    ${data}/particles-fluid.vert
    ${data}/particles-fluid.geom
    ${data}/particles-fluid.frag
)    


# 
# Create executable
# 

# Build executable
add_executable(${target}
    ${sources}
)

# Create namespaced alias
add_executable(${META_PROJECT_NAME}::${target} ALIAS ${target})


# 
# Project options
# 

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
)


# 
# Include directories
# 

target_include_directories(${target}
    PRIVATE
    ${DEFAULT_INCLUDE_DIRECTORIES}
    ${PROJECT_BINARY_DIR}/source/include
    SYSTEM
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR} 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


# 
# Libraries
# 

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${META_PROJECT_NAME}::cgutils
    glbinding::glbinding
)


# 
# Compile definitions
# 

target_compile_definitions(${target}
    PRIVATE
    $<$<BOOL:${OPENMP_FOUND}>:USE_OPENMP>
    $<$<BOOL:${OPTION_USE_AVX2}>:BUILD_WITH_AVX2>
    ${DEFAULT_COMPILE_DEFINITIONS}
    PUBLIC
    GLFW_INCLUDE_NONE
    GLM_FORCE_RADIANS
)


# 
# Compile options
# 

target_compile_options(${target}
    PRIVATE
    $<$<BOOL:${OPENMP_FOUND}>:${OpenMP_CXX_FLAGS}>
    ${DEFAULT_COMPILE_OPTIONS}
    $<$<CXX_COMPILER_ID:GNU>:
        -msse4.1
        $<$<BOOL:${OPTION_USE_AVX2}>:-mavx2 -mfma>
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<BOOL:${OPTION_USE_AVX2}>:/arch:AVX2>
    >
    $<$<CXX_COMPILER_ID:AppleClang>:
    -msse4.1
    $<$<BOOL:${OPTION_USE_AVX2}>:-mavx2 -mfma>
    >
)


# 
# Linker options
# 

target_link_libraries(${target}
    PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:$<$<BOOL:"${OPENMP_FOUND}">:${OpenMP_CXX_FLAGS}>>
    ${DEFAULT_LINKER_OPTIONS}
)


# 
# Deployment
# 

# Executable
install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN} COMPONENT examples
    BUNDLE  DESTINATION ${INSTALL_BIN} COMPONENT examples
)

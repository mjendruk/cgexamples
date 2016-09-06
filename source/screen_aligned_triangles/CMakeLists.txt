
# 
# External dependencies
# 

find_package(GLM REQUIRED)
find_package(GLFW)
find_package(glbinding REQUIRED)


# 
# Executable name and options
# 

# Target name
set(target screen_aligned_triangles)

#exit here if using OS X
if (APPLE)
    message("${target} skipped: OS X does not support OpenGl 4.2")
    return()
endif()

# Exit here if required dependencies are not met
if (NOT GLFW_FOUND)
    message("${target} skipped: GLFW not found")
    return()
endif()

message(STATUS "${target}")


# 
# Sources
# 

set(data "${CMAKE_SOURCE_DIR}/data/${target}")

set(sources
    main.cpp

    scrat.cpp
    scrat.h
    ${data}/record.frag
    ${data}/record.vert
    ${data}/replay.frag
    ${data}/replay.vert
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
    ${DEFAULT_COMPILE_OPTIONS}
)


# 
# Linker options
# 

target_link_libraries(${target}
    PRIVATE
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

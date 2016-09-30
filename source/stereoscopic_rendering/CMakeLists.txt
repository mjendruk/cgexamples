
# 
# External dependencies
# 

find_package(GLM REQUIRED)
find_package(GLFW)
find_package(glbinding REQUIRED)
find_package(OVR)


# 
# Executable name and options
# 

# Target name
set(target stereoscopic_rendering)

# Exit here if required dependencies are not met
if (NOT GLFW_FOUND)
    message("${target} skipped: GLFW not found")
    return()
endif()

if (NOT OVR_FOUND)
    message("${target} will be built without OVR: libOVR not found")
endif()

message(STATUS "${target}")


# 
# Sources
# 

set(data "${CMAKE_SOURCE_DIR}/data/${target}")

set(sources
    main.cpp

    abstractrenderer.cpp
    abstractrenderer.h
    icosahedron.cpp
    icosahedron.h
    make_unique.h
    scene.cpp
    scene.h
    sidebyside.cpp
    sidebyside.h
    ${data}/icosahedron.frag
    ${data}/icosahedron.vert
)

if (OVR_FOUND)
    set(sources ${sources} ovr.cpp ovr.h)
endif()

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
    $<$<BOOL:${OVR_FOUND}>:${OVR_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


# 
# Libraries
# 

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${GLFW_LIBRARIES}
    $<$<BOOL:${OVR_FOUND}>:${OVR_LIBRARY}>
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
    $<$<BOOL:${OVR_FOUND}>:USE_OVR>
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

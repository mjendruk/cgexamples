
# 
# Configuration for all sub-projects
# 

# Generate version-header
string(TOUPPER ${META_PROJECT_NAME} META_PROJECT_NAME_UPPER)
configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME}/${META_PROJECT_NAME}-version.h)


# 
# Sub-projects
# 


# Libraries
set(IDE_FOLDER "")
add_subdirectory(cgutils)

add_subdirectory(screen_aligned_triangles)
add_subdirectory(sky_triangle)


# 
# Deployment
# 

# Deploy generated headers
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME} DESTINATION include COMPONENT dev)

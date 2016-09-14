
# OVR_FOUND
# OVR_INCLUDE_DIR
# OVR_LIBRARY

# OVR_BINARY (win32 only)


find_path(OVR_INCLUDE_DIR OVR_CAPI.h

    PATHS
    $ENV{OVR_DIR}

    PATH_SUFFIXES
    /include

    DOC "The directory where OVR_CAPI.h resides")

if (X64)
    set(OVR_BUILD_DIR x64/Release)
else()
    set(OVR_BUILD_DIR Win32/Release)
endif()

IF (MSVC)
    # Visual Studio 2010
    IF(MSVC10)
        SET(OCULUS_MSVC_DIR "VS2010")
    ENDIF()
    # Visual Studio 2012
    IF(MSVC11)
        SET(OCULUS_MSVC_DIR "VS2012")
    ENDIF()
    # Visual Studio 2013
    IF(MSVC12)
        SET(OCULUS_MSVC_DIR "VS2013")
    ENDIF()
    # Visual Studio 2015
    IF(MSVC14)
        SET(OCULUS_MSVC_DIR "VS2015")
    ENDIF()
ENDIF()

find_library(OVR_LIBRARY NAMES LibOVR

    PATHS
    $ENV{OVR_DIR}

    # authors prefered choice for development
    /lib/Windows/${OVR_BUILD_DIR}/${OCULUS_MSVC_DIR}

    DOC "The OVR library")

# if(WIN32)

#     find_file(GLEW_BINARY NAMES glew32.dll glew32s.dll

#         HINTS
#         ${GLEW_INCLUDE_DIR}/..

#         PATHS
#         $ENV{OVR_DIR}

#         PATH_SUFFIXES
#         /bin
#         /bin/${OVR_BUILD_DIR}

#         DOC "The GLEW binary")

# endif()
    
find_package_handle_standard_args(OVR REQUIRED_VARS OVR_INCLUDE_DIR OVR_LIBRARY)
# mark_as_advanced(OVR_INCLUDE_DIR OVR_LIBRARY)

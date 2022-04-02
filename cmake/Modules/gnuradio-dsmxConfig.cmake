find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_DSMX gnuradio-dsmx)

FIND_PATH(
    GR_DSMX_INCLUDE_DIRS
    NAMES gnuradio/dsmx/api.h
    HINTS $ENV{DSMX_DIR}/include
        ${PC_DSMX_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_DSMX_LIBRARIES
    NAMES gnuradio-dsmx
    HINTS $ENV{DSMX_DIR}/lib
        ${PC_DSMX_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-dsmxTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_DSMX DEFAULT_MSG GR_DSMX_LIBRARIES GR_DSMX_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_DSMX_LIBRARIES GR_DSMX_INCLUDE_DIRS)

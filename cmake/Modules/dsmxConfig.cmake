INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_DSMX dsmx)

FIND_PATH(
    DSMX_INCLUDE_DIRS
    NAMES dsmx/api.h
    HINTS $ENV{DSMX_DIR}/include
        ${PC_DSMX_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    DSMX_LIBRARIES
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

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DSMX DEFAULT_MSG DSMX_LIBRARIES DSMX_INCLUDE_DIRS)
MARK_AS_ADVANCED(DSMX_LIBRARIES DSMX_INCLUDE_DIRS)


include(FindPackageHandleStandardArgs)

find_path(VPX_INCLUDE_DIR
  NAMES vpx/vp8.h
)

find_library(VPX_LIBRARY
    NAMES vpx
)

find_package_handle_standard_args(libvpx
  REQUIRED_VARS VPX_INCLUDE_DIR VPX_LIBRARY
)

if(libvpx_FOUND AND NOT TARGET WebM::vpx)
    add_library(WebM::vpx UNKNOWN IMPORTED)
    set_target_properties(WebM::vpx PROPERTIES
      IMPORTED_LOCATION "${VPX_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${VPX_INCLUDE_DIR}"
    )
endif()
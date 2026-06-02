get_filename_component(_nestegg_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

add_library(nestegg STATIC IMPORTED)
set_target_properties(nestegg PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${_nestegg_root}/include"
    IMPORTED_LOCATION "${_nestegg_root}/lib/nestegg.lib"
)

unset(_nestegg_root)

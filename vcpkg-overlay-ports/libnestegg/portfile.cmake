set(VCPKG_LIBRARY_LINKAGE static)

if(WIN32)
    set(PATCHES Makefile.patch)
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO mozilla/nestegg
        REF f7a0b7cedc893b6683cf15cb210b1656c086d964
        SHA512 93aaefab4975823e45da92c5874ea71544f436cde65cae6120f162440fbab4b975351a83a36fddb25665bb039e7cd6ee6d96400f86499a97ce65209752d244c8
        HEAD_REF master
        PATCHES ${PATCHES}
    )
else()
    vcpkg_from_github(
        OUT_SOURCE_PATH SOURCE_PATH
        REPO mozilla/nestegg
        REF f7a0b7cedc893b6683cf15cb210b1656c086d964
        SHA512 93aaefab4975823e45da92c5874ea71544f436cde65cae6120f162440fbab4b975351a83a36fddb25665bb039e7cd6ee6d96400f86499a97ce65209752d244c8
        HEAD_REF master
    )
endif()

vcpkg_make_configure(
  SOURCE_PATH ${SOURCE_PATH}
  AUTORECONF
  LANGUAGES "C"
)

vcpkg_make_install()
file(COPY ${CMAKE_CURRENT_LIST_DIR}/${PORT}Config.cmake DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT})
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
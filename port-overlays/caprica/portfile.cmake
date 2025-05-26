set(ENV{PATH} "C:\\Users\\Lynn\\bin;D:\\v;D:\\tools\\CMake\\bin;D:\\tools\\ninja-win;$ENV{PATH}")
set(ENV{VCPKG_ROOT} "D:/v")

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO lynnpye/caprica
  REF d74bb93760750dff30dcb2637e5b5308c32d43a2
  SHA512 ccbeaa5c3237d6dc826b5a2533a473b5534af7ece20075e85c5409d1937795af32f0a2b009fc7e2a8267681e84486255b3d50962ade9c604a0e0b818c07cde56
  HEAD_REF static
)

vcpkg_configure_cmake(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include/caprica/caprica/cmake"
    "${CURRENT_PACKAGES_DIR}/include/caprica/caprica/cmake")

vcpkg_cmake_config_fixup()


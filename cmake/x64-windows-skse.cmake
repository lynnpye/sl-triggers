set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)

if (${PORT} MATCHES "fully-dynamic-game-engine|skse|qt*")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
else ()
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

set($ENV{PATH} "C:/Users/Lynn/bin;D:/v;D:/tools/CMake/bin;D:/tools/ninja-win;$ENV{PATH}")
set($ENV{VCPKG_ROOT} "D:/v")

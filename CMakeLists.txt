cmake_minimum_required(VERSION 3.31.6)
project(wsr VERSION 0.0.1 LANGUAGES C CXX)
set(PROJECT_ADVERTISED_NAME wsRelay)
add_compile_definitions(PROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

add_subdirectory(utils)
add_subdirectory(websocket)
add_subdirectory(client)

include(CTest)
add_subdirectory(tests)

include(GNUInstallDirs)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_ADVERTISED_NAME}")

install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/base64/LICENSE" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_ADVERTISED_NAME}/wpa_supplicant and hostapd")
set(CPACK_PACKAGE_NAME "${PROJECT_ADVERTISED_NAME}")
include(CPack)
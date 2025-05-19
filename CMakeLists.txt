cmake_minimum_required(VERSION 3.24...3.30)

project(pong LANGUAGES CXX VERSION 0.0.1)

if(NOT DEFINED CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD STREQUAL "98")
    set(CMAKE_CXX_STANDARD 23)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/WX)
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CLANG_USED TRUE)
    endif()
    option(CLANG_USE_LIBCPP "Use libc++ for clang compilation" ${CLANG_USED})
    if(APPLE OR CLANG_USE_LIBCPP)
        message(STATUS "Use libc++")
        add_compile_options(-stdlib=libc++)
        add_link_options(-stdlib=libc++ -lc++abi)
    endif()

endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_subdirectory(src)

# add_library(pong::pong ALIAS pong)
# target_include_directories(pong INTERFACE
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#     $<INSTALL_INTERFACE:include>
# )

# option(PONG_INSTALL "Generate the install target" ${PONG_MASTER_PROJECT})
# if(PONG_INSTALL)
#     include(GNUInstallDirs)
#     include(CMakePackageConfigHelpers)

#     install(TARGETS pong EXPORT ${PROJECT_NAME}Targets)
#     install(DIRECTORY include/pong DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#     set(configFile ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/cmake/Config.cmake)
#     set(versionFile ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake)
#     set(configInstallDestination ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

#     configure_package_config_file(
#         ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
#         ${configFile}
#         INSTALL_DESTINATION ${configInstallDestination}
#     )
#     if(${CMAKE_VERSION} VERSION_GREATER 3.13)
#         write_basic_package_version_file("${versionFile}" COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
#     else()
#         write_basic_package_version_file("${versionFile}" COMPATIBILITY SameMajorVersion)
#     endif()
#     configure_file(
#         ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in
#         ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
#         @ONLY
#     )

#     install(FILES ${configFile} ${versionFile} DESTINATION ${configInstallDestination})
#     install(
#         EXPORT ${PROJECT_NAME}Targets
#         NAMESPACE "pong::"
#         DESTINATION ${configInstallDestination}
#     )
#     install(
#         FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
#         DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
#     )
# endif()

option(BUILD_PONG_TESTS "Build tests" ON)
if(BUILD_PONG_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

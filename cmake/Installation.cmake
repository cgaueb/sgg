include (GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets)

# install tree
install(EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
)

# build tree
export(EXPORT ${PROJECT_NAME}-targets
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_NAME}-targets.cmake
)

include(CMakePackageConfigHelpers)

# install tree
configure_package_config_file(cmake/${PROJECT_NAME}-config.cmake.in
    ${PROJECT_BINARY_DIR}/share/${PROJECT_NAME}-config.cmake
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
    PATH_VARS 
        CMAKE_INSTALL_DATADIR 
)

# build tree
configure_file(cmake/${PROJECT_NAME}-config.build.cmake.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    @ONLY
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    COMPATIBILITY SameMajorVersion
)

export(PACKAGE ${PROJECT_NAME})

install(FILES
    ${PROJECT_BINARY_DIR}/share/${PROJECT_NAME}-config.cmake
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
)

install(DIRECTORY 
    ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(FILES
    ${PROJECT_SOURCE_DIR}/cmake/FindSDL2.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindWrap_Freetype.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindWrap_glm.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindWrap_SDL2_mixer.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindSDL2_mixer.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindWrap_GLEW.cmake
    ${PROJECT_SOURCE_DIR}/cmake/FindWrap_SDL2.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/cmake/${PROJECT_NAME}
)


include(${PROJECT_SOURCE_DIR}/cmake/AddUninstallTarget.cmake)


# Install targets
install(FILES
    $<TARGET_RUNTIME_DLLS:FlowCompute>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT FlowComputeCore
)

# Catch transitive dependencies
if(DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/")
else()
    set(VCPKG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/")
endif()

install(DIRECTORY "${VCPKG_BIN_DIR}"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT FlowComputeCore
    FILES_MATCHING PATTERN "*.dll"
)

# Windows Deployment & IFW Setup
install(PROGRAMS "${CMAKE_SOURCE_DIR}/server/wsl_server"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT FlowComputeCore
)

install(FILES $<TARGET_RUNTIME_DLLS:FlowCompute>
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT FlowComputeCore
)

# CPack IFW Configuration
set(CPACK_GENERATOR "IFW")
set(CPACK_PACKAGE_NAME "FlowCompute")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FlowCompute")
set(CPACK_PACKAGE_VERSION "0.9.0")
set(CPACK_PACKAGE_VENDOR "FlowCompute LLC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source client for OpenFOAM.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

file(TO_CMAKE_PATH "$ENV{IFW_PATH}" CLEAN_IFW_PATH)
set(CPACK_IFW_ROOT "${CLEAN_IFW_PATH}")
set(CPACK_IFW_PACKAGE_TITLE "FlowCompute")
set(CPACK_IFW_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/images/flowcompute.ico")
set(CPACK_IFW_PACKAGE_STYLE_SHEET "${CMAKE_SOURCE_DIR}/installer_style.qss")
set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT "${CMAKE_SOURCE_DIR}/control_script.qs")
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Classic")
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH 700)
set(CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT 400)

include(CPack)
include(CPackIFW)

cpack_ifw_configure_component(FlowComputeCore
    DISPLAY_NAME "FlowCompute"
    DESCRIPTION "The core executable and configuration files."
    LICENSES "LGPLv3" "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    FORCED_INSTALLATION
)
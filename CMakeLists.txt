cmake_minimum_required(VERSION 3.19)
project(FlowCompute VERSION 0.9.0 LANGUAGES CXX C)

include(GNUInstallDirs)

set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Concurrent)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

qt_standard_project_setup()

# Tree-sitter library
add_library(tree_sitter OBJECT third_party/tree_sitter/src/lib.c)
target_include_directories(tree_sitter PUBLIC third_party/tree_sitter/include)

# Suppress warnings for C code
set(C_WARNING_SUPPRESSION $<$<CXX_COMPILER_ID:MSVC>:/w> $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
target_compile_options(tree_sitter PRIVATE ${C_WARNING_SUPPRESSION})

# Access libssh and zlib
find_package(ZLIB REQUIRED)
if(WIN32)
    find_package(libssh REQUIRED)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH REQUIRED IMPORTED_TARGET libssh)
endif()

# Find the Vulkan shader compiler
find_package(Vulkan REQUIRED)
if(TARGET Vulkan::glslangValidator)
    get_target_property(GLSL_VALIDATOR Vulkan::glslangValidator IMPORTED_LOCATION)
else()
    find_program(GLSL_VALIDATOR glslangValidator HINTS $ENV{VULKAN_SDK}/bin)
endif()

if(NOT GLSL_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found! Make sure the Vulkan SDK is in your system PATH.")
endif()

# Add subdirectories
add_subdirectory(dialogs)
add_subdirectory(editors)
add_subdirectory(geometry)
add_subdirectory(parser)
add_subdirectory(shaders)
add_subdirectory(systems)
add_subdirectory(views)
add_subdirectory(wizards)

qt_add_executable(FlowCompute
    main.cpp
    main_window.cpp
    main_window.h
    main_window_slots.cpp
    resources.qrc
    core_types.h

)

if(WIN32)
    target_sources(FlowCompute PRIVATE app_icon.rc)
endif()

# SPIR-V files will be generated at build time
set_source_files_properties(${COMPILED_SPIRV_FILES} PROPERTIES GENERATED ON)

# Bundle the .spv files as resources
qt_add_resources(FlowCompute "compiled_shaders"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    FILES ${COMPILED_SPIRV_FILES}
)

target_compile_definitions(FlowCompute PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
)

target_include_directories(FlowCompute PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
)

add_dependencies(FlowCompute CompileShaders)

target_link_libraries(FlowCompute PRIVATE
    Dialogs Editors Geometry Parser Systems Views Wizards
    Qt6::Widgets Qt6::Core
)

# Installation and deployment
install(TARGETS FlowCompute
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT FlowComputeCore
)

# Deploy Qt dependencies
qt_generate_deploy_app_script(
    TARGET FlowCompute
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
)

# Link the script to the component
install(SCRIPT ${deploy_script} COMPONENT FlowComputeCore)

if(WIN32)
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
endif()

install(PROGRAMS
    "${CMAKE_SOURCE_DIR}/server/wsl_server"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT FlowComputeCore
)

# CPack configuration
set(CPACK_GENERATOR "IFW")
set(CPACK_PACKAGE_NAME "FlowCompute")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FlowCompute")
set(CPACK_PACKAGE_VERSION "0.9.0")
set(CPACK_PACKAGE_VENDOR "FlowCompute LLC")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source client for OpenFOAM.")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# IFW Configuration
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
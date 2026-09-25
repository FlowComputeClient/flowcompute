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
set(C_WARNING_SUPPRESSION $<$<C_COMPILER_ID:MSVC>:/w> $<$<NOT:$<C_COMPILER_ID:MSVC>>:-w>)
target_compile_options(tree_sitter PRIVATE ${C_WARNING_SUPPRESSION})

# Access libssh and zlib
find_package(ZLIB REQUIRED)
if(WIN32)
    find_package(libssh REQUIRED)
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBSSH REQUIRED IMPORTED_TARGET libssh)
    if(TARGET PkgConfig::LIBSSH AND NOT TARGET libssh::libssh)
        add_library(libssh::libssh ALIAS PkgConfig::LIBSSH)
    endif()
endif()

# Find the Vulkan shader compiler
find_package(Vulkan REQUIRED)
if(TARGET Vulkan::glslangValidator)
    get_target_property(GLSL_VALIDATOR Vulkan::glslangValidator IMPORTED_LOCATION)
else()
    find_program(GLSL_VALIDATOR glslangValidator HINTS "$ENV{VULKAN_SDK}/bin")
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

# Common installation rule
install(TARGETS FlowCompute
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT FlowComputeCore
)

# Deploy Qt runtime dependencies
qt_generate_deploy_app_script(
    TARGET FlowCompute
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script} COMPONENT FlowComputeCore)

# Platform-specific packaging and dependency management
if(WIN32)
    include(cmake/PackagingWindows.cmake)
elseif(UNIX AND NOT APPLE)
    include(cmake/PackagingLinux.cmake)
endif()
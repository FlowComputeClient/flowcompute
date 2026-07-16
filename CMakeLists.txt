cmake_minimum_required(VERSION 3.19)
project(FlowCompute VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Concurrent)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

qt_standard_project_setup()

qt_add_executable(FlowCompute
    main.cpp
    main_window.cpp
    main_window.h
	main_window_slots.cpp
    resources.qrc
    core_types.h
    utils.h utils.cpp

    dialogs/surface/surface_dialog.h
    dialogs/surface/surface_dialog.cpp

    dialogs/preferences/preferences_dialog.h
	dialogs/preferences/preferences_dialog.cpp

    dialogs/run_mesh/run_mesh_dialog.h
    dialogs/run_mesh/run_mesh_dialog.cpp

    dialogs/run_solver/run_solver_dialog.h
    dialogs/run_solver/run_solver_dialog.cpp

    dialogs/selection/selection_dialog.h
    dialogs/selection/selection_dialog.cpp

    dialogs/utility_output/utility_output_dialog.h
    dialogs/utility_output/utility_output_dialog.cpp

    editors/tab_bar.h
	editors/tab_bar.cpp
    editors/tab_widget.h
    editors/tab_widget.cpp

    editors/text/text_editor.h
    editors/text/text_editor.cpp
    editors/text/tree_sitter_highlighter.h
    editors/text/tree_sitter_highlighter.cpp

    editors/graphical/table_delegate.h
    editors/graphical/table_delegate.cpp

    editors/graphical/vulkan/vulkan_window.h
    editors/graphical/vulkan/vulkan_window.cpp
    editors/graphical/vulkan/vulkan_renderer.h
    editors/graphical/vulkan/vulkan_renderer.cpp
    editors/graphical/vulkan/vulkan_renderer_resources.cpp

    editors/graphical/mesh/mesh_editor.h
    editors/graphical/mesh/mesh_editor.cpp
    editors/graphical/mesh/mesh_left_pane.h
    editors/graphical/mesh/mesh_left_pane.cpp

    editors/graphical/surface/surface_editor.cpp
    editors/graphical/surface/surface_editor.h
    editors/graphical/surface/surface_left_pane.h
    editors/graphical/surface/surface_left_pane.cpp

    editors/graphical/result/result_editor.h
    editors/graphical/result/result_editor.cpp
    editors/graphical/result/result_left_pane.h
    editors/graphical/result/result_left_pane.cpp
    editors/graphical/result/color_bar_widget.h
    editors/graphical/result/color_bar_widget.cpp

    geometry/graphic_data.h
    geometry/mesh/mesh_reader.h
    geometry/mesh/mesh_reader.cpp
    geometry/stl/stl_reader.h
    geometry/stl/stl_reader.cpp
    geometry/obj/obj_reader.h
    geometry/obj/obj_reader.cpp	

    parser/open_foam_dictionary.h
    parser/open_foam_dictionary.cpp
    parser/parser.c
    third_party/tree_sitter/src/lib.c

    systems/local_system.h
	systems/local_system.cpp
    systems/remote_system.h
    systems/remote_system.cpp
    systems/system_manager.h
	systems/system_manager.cpp
    systems/target_system.h
    systems/wsl_system.h
    systems/wsl_system.cpp

    views/console/console.h
    views/console/console.cpp

    views/navigator/node_data.h
    views/navigator/navigator_model.h
    views/navigator/navigator_model.cpp
    views/navigator/case_navigator.h
    views/navigator/case_navigator.cpp
    views/navigator/node_data.cpp

    wizards/new_case/wizard_new_case.h
    wizards/new_case/wizard_new_case.cpp
    wizards/new_case/page_10_intro.h
    wizards/new_case/page_10_intro.cpp
    wizards/new_case/page_20_tutorial.h
    wizards/new_case/page_20_tutorial.cpp
    wizards/new_case/page_30_interactive.h
    wizards/new_case/page_30_interactive.cpp
    wizards/new_case/page_40_project.cpp
    wizards/new_case/page_40_project.h
    wizards/new_case/template_strings.h

    wizards/mesh/mesh_structs.h
    wizards/mesh/mesh_io.h
    wizards/mesh/mesh_io.cpp
    wizards/mesh/wizard_mesh.h
    wizards/mesh/wizard_mesh.cpp
    wizards/mesh/page_10_geometry.h
    wizards/mesh/page_10_geometry.cpp
    wizards/mesh/page_20_blockmesh.h
    wizards/mesh/page_20_blockmesh.cpp
    wizards/mesh/page_30_blockmesh.h
    wizards/mesh/page_30_blockmesh.cpp
    wizards/mesh/page_40_surface_feature.h
    wizards/mesh/page_40_surface_feature.cpp
    wizards/mesh/page_50_castellation.h
    wizards/mesh/page_50_castellation.cpp
    wizards/mesh/page_60_snapcontrol.h
    wizards/mesh/page_60_snapcontrol.cpp
    wizards/mesh/page_70_layercontrol.h
    wizards/mesh/page_70_layercontrol.cpp

    wizards/solver/solver_structs.h
    wizards/solver/wizard_solver.h
    wizards/solver/wizard_solver.cpp
    wizards/solver/solver_io.h
    wizards/solver/solver_io.cpp
    wizards/solver/surface_widget.cpp
    wizards/solver/surface_widget.h
    wizards/solver/page_10_control.cpp
    wizards/solver/page_10_control.h
    wizards/solver/page_20_transient.h
    wizards/solver/page_20_transient.cpp
    wizards/solver/page_30_physics.cpp
    wizards/solver/page_30_physics.h
    wizards/solver/page_40_boundary.cpp
    wizards/solver/page_40_boundary.h
    wizards/solver/page_50_algorithm.cpp
    wizards/solver/page_50_algorithm.h
    wizards/solver/page_70_pimple.h
    wizards/solver/page_70_pimple.cpp
    wizards/solver/page_60_simple.h
    wizards/solver/page_60_simple.cpp
    wizards/solver/page_80_piso.h
    wizards/solver/page_80_piso.cpp
    wizards/solver/page_90_parallel.cpp
    wizards/solver/page_90_parallel.h
    wizards/solver/page_93_visualization.h
    wizards/solver/page_93_visualization.cpp
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/parser/parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tree_sitter/src/lib.c
    PROPERTIES COMPILE_FLAGS "-w"
)

target_include_directories(FlowCompute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(FlowCompute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tree_sitter/include)
target_include_directories(FlowCompute PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/parser)

find_package(ZLIB REQUIRED)
target_link_libraries(FlowCompute PRIVATE
    Qt::Core Qt::Widgets Qt::Network Qt6::Concurrent ZLIB::ZLIB)

install(TARGETS FlowCompute
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

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

# List the relative paths to all your shader source files
set(SHADER_SOURCES
    "shaders/axis/axis.vert"
    "shaders/axis/axis.frag"
    "shaders/model/model.vert"
    "shaders/model/model.frag"
    "shaders/mesh/mesh.vert"
    "shaders/mesh/mesh.frag"
    "shaders/color/color.vert"
    "shaders/color/color.frag"
)

set(SPIRV_FILES "")

# Create a command for each shader
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_DIR ${SHADER} DIRECTORY)
    get_filename_component(SHADER_EXT ${SHADER} LAST_EXT)
    string(SUBSTRING ${SHADER_EXT} 1 -1 OUT_NAME)
    set(SPIRV_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_DIR}/${OUT_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${GLSL_VALIDATOR} -V ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling Vulkan shader: ${SHADER} -> ${SHADER_DIR}/${OUT_NAME}.spv"
    )
    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach()

# Group all shader commands into a single target
set_property(TARGET FlowCompute APPEND PROPERTY AUTORCC_DEPENDS ${SPIRV_FILES})

qt_generate_deploy_app_script(
    TARGET FlowCompute
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

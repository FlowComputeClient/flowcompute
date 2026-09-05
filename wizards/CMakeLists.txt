add_library(Wizards STATIC
    mesh/wizard_mesh.h
    mesh/wizard_mesh.cpp
    mesh/page_10_geometry.h
    mesh/page_10_geometry.cpp
    mesh/page_20_blockmesh.h
    mesh/page_20_blockmesh.cpp
    mesh/page_30_blockmesh.h
    mesh/page_30_blockmesh.cpp
    mesh/page_40_surface_feature.h
    mesh/page_40_surface_feature.cpp
    mesh/page_50_castellation.h
    mesh/page_50_castellation.cpp
    mesh/page_60_snapcontrol.h
    mesh/page_60_snapcontrol.cpp
    mesh/page_70_layercontrol.h
    mesh/page_70_layercontrol.cpp

    new_case/wizard_new_case.h
    new_case/wizard_new_case.cpp
    new_case/page_10_intro.h
    new_case/page_10_intro.cpp
    new_case/page_15_remote.h
    new_case/page_15_remote.cpp
    new_case/page_20_tutorial.h
    new_case/page_20_tutorial.cpp
    new_case/page_30_interactive.h
    new_case/page_30_interactive.cpp
    new_case/page_40_project.cpp
    new_case/page_40_project.h
    new_case/template_strings.h

    open_case/wizard_open_case.h
    open_case/wizard_open_case.cpp
    open_case/page_20_casefolder.h
    open_case/page_20_casefolder.cpp

    post_processing/forces_dialog.h
    post_processing/forces_dialog.cpp
    post_processing/force_coeffs_dialog.h
    post_processing/force_coeffs_dialog.cpp
    post_processing/field_minmax_dialog.h
    post_processing/field_minmax_dialog.cpp
    post_processing/page_10_tasks.h
    post_processing/page_10_tasks.cpp
    post_processing/page_20_time_region.h
    post_processing/page_20_time_region.cpp
    post_processing/probes_dialog.h
    post_processing/probes_dialog.cpp
    post_processing/surfaces_dialog.h
    post_processing/surfaces_dialog.cpp
    post_processing/add_surface_dialog.h
    post_processing/add_surface_dialog.cpp
    post_processing/wizard_postprocessing.h
    post_processing/wizard_postprocessing.cpp
    post_processing/task_structs.h
    post_processing/yplus_dialog.h
    post_processing/yplus_dialog.cpp

    solver/wizard_solver.h
    solver/wizard_solver.cpp
    solver/page_10_control.cpp
    solver/page_10_control.h
    solver/page_20_transient.h
    solver/page_20_transient.cpp
    solver/page_30_physics.cpp
    solver/page_30_physics.h
    solver/page_40_boundary.cpp
    solver/page_40_boundary.h
    solver/page_50_algorithm.cpp
    solver/page_50_algorithm.h
    solver/page_70_pimple.h
    solver/page_70_pimple.cpp
    solver/page_60_simple.h
    solver/page_60_simple.cpp
    solver/page_80_piso.h
    solver/page_80_piso.cpp
    solver/page_90_parallel.cpp
    solver/page_90_parallel.h
)

# Link libraries
target_link_libraries(Wizards PRIVATE Qt6::Widgets Qt6::Concurrent)

# Link libssh
if(WIN32)
    target_link_libraries(Wizards PUBLIC ssh)
elseif(UNIX)
    target_link_libraries(Wizards PUBLIC PkgConfig::LIBSSH Parser)
endif()

# Include folders
target_include_directories(Wizards PUBLIC ${CMAKE_SOURCE_DIR})
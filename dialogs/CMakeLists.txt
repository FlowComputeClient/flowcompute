add_library(Dialogs STATIC
    about/about_dialog.h 
    about/about_dialog.cpp
    license/license_dialog.h 
    license/license_dialog.cpp
    login/login_dialog.h
    login/login_dialog.cpp
    new_dict/new_dict_dialog.h
    new_dict/new_dict_dialog.cpp
    preferences/preferences_dialog.h
    preferences/preferences_dialog.cpp
    run_mesh/run_mesh_dialog.h
    run_mesh/run_mesh_dialog.cpp
    run_solver/run_solver_dialog.h
    run_solver/run_solver_dialog.cpp
    selection/selection_dialog.h
    selection/selection_dialog.cpp
    utility_output/utility_output_dialog.h
    utility_output/utility_output_dialog.cpp
)

# Link libraries
target_link_libraries(Dialogs PRIVATE Qt6::Widgets ssh)

# Include folders
target_include_directories(Dialogs PUBLIC ${CMAKE_SOURCE_DIR})
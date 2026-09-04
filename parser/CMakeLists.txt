add_library(Parser STATIC
    parser.c
    open_foam_dictionary.h
    open_foam_dictionary.cpp
    block_mesh_dict.h
    block_mesh_dict.cpp
    boundary.h
    boundary.cpp
    common.h
    common.cpp
    control_dict.h
    control_dict.cpp
    decompose_par_dict.h
    decompose_par_dict.cpp
    field.h
    field.cpp
    function_object.h
    function_object.cpp
    fv_solution.h
    fv_solution.cpp
    snappy_hex_mesh_dict.h
    snappy_hex_mesh_dict.cpp
    surface_feature.h
    surface_feature.cpp
    surface_patch_dict.h
    surface_patch_dict.cpp
    transport_properties.h
    transport_properties.cpp
    turbulence_properties.h
    turbulence_properties.cpp
)

# Link libraries
target_link_libraries(Parser PRIVATE Qt6::Widgets tree_sitter)

# Include folders
target_include_directories(Parser PUBLIC ${CMAKE_SOURCE_DIR})
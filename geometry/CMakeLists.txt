add_library(Geometry STATIC
    graphic_data.h
    mesh/mesh_reader.h
    mesh/mesh_reader.cpp
    stl/stl_reader.h
    stl/stl_reader.cpp
    obj/obj_reader.h
    obj/obj_reader.cpp
)

# Link libraries
target_link_libraries(Geometry PRIVATE Qt6::Widgets)

# Include folders
target_include_directories(Geometry PUBLIC ${CMAKE_SOURCE_DIR})
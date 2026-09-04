add_library(Views STATIC
    console/console.h
    console/console.cpp
    navigator/node_data.h
    navigator/navigator_model.h
    navigator/navigator_model.cpp
    navigator/case_navigator.h
    navigator/case_navigator.cpp
    navigator/case_navigator_delegate.h
    navigator/case_navigator_delegate.cpp
    navigator/node_data.cpp
)

# Link libraries
target_link_libraries(Views PRIVATE Qt6::Widgets)

# Include folders
target_include_directories(Views PUBLIC ${CMAKE_SOURCE_DIR})
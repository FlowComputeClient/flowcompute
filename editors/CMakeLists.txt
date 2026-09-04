add_library(Editors STATIC
    graphical/patch_palette.h
    graphical/patch_palette.cpp
    graphical/table_delegate.h
    graphical/table_delegate.cpp
    graphical/mesh/mesh_editor.h
    graphical/mesh/mesh_editor.cpp
    graphical/mesh/mesh_left_pane.h
    graphical/mesh/mesh_left_pane.cpp
    graphical/result/result_editor.h
    graphical/result/result_editor.cpp
    graphical/result/result_left_pane.h
    graphical/result/result_left_pane.cpp
    graphical/result/color_bar_widget.h
    graphical/result/color_bar_widget.cpp
    graphical/surface/surface_editor.cpp
    graphical/surface/surface_editor.h
    graphical/surface/surface_left_pane.h
    graphical/surface/surface_left_pane.cpp
    graphical/vulkan/vulkan_window.h
    graphical/vulkan/vulkan_window.cpp
    graphical/vulkan/vulkan_renderer.h
    graphical/vulkan/vulkan_renderer.cpp
    graphical/vulkan/vulkan_renderer_resources.cpp
    text/file_changed_banner.h
    text/file_changed_banner.cpp
    text/text_editor.h
    text/text_editor.cpp
    text/text_widget.h
    text/text_widget.cpp
    text/tree_sitter_highlighter.h
    text/tree_sitter_highlighter.cpp
    tab_bar.h
    tab_bar.cpp
    tab_widget.h
    tab_widget.cpp
)

# Link libraries
target_link_libraries(Editors PUBLIC Qt6::Concurrent tree_sitter PRIVATE Qt6::Widgets)

# Include folders
target_include_directories(Editors PUBLIC ${CMAKE_SOURCE_DIR})

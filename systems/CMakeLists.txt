add_library(Systems STATIC
    local_system.h
    local_system.cpp
    local_system_mesh.cpp
    remote_system.h
    remote_system.cpp
    system_manager.h
    system_manager.cpp
    target_system.h
    wsl_system.h
    wsl_system.cpp
)

# Link libraries
target_link_libraries(Systems PUBLIC Qt6::Concurrent Qt6::Network PRIVATE Qt6::Widgets ZLIB::ZLIB)

# Link libssh
if(WIN32)
    target_link_libraries(Systems PUBLIC ssh)
elseif(UNIX)
    target_link_libraries(Systems PUBLIC PkgConfig::LIBSSH Dialogs)
endif()

# Include folders
target_include_directories(Systems PUBLIC ${CMAKE_SOURCE_DIR})
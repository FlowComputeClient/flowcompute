# Linux Desktop & AppImage Target Setup
include(GNUInstallDirs)

install(FILES "${CMAKE_SOURCE_DIR}/deploy/flowcompute.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    COMPONENT FlowComputeCore
)

install(FILES "${CMAKE_SOURCE_DIR}/images/flowcompute.png"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
    COMPONENT FlowComputeCore
)

find_program(LINUXDEPLOYQT_EXECUTABLE linuxdeployqt)

if(LINUXDEPLOYQT_EXECUTABLE)
    add_custom_target(appimage
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_BINARY_DIR}/AppDir"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/AppDir"
        COMMAND ${CMAKE_COMMAND} -E env DESTDIR="${CMAKE_BINARY_DIR}/AppDir"
                "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --prefix /usr
		COMMAND ${CMAKE_COMMAND} -E env 
                VERSION="${PROJECT_VERSION}" 
                OUTPUT="FlowCompute-${PROJECT_VERSION}.AppImage"
                "${LINUXDEPLOYQT_EXECUTABLE}"
                "${CMAKE_BINARY_DIR}/AppDir/usr/share/applications/flowcompute.desktop"
                -appimage
                -updateinformation="gh-releases-zsync|FlowComputeClient|flowcompute|latest|FlowCompute-${PROJECT_VERSION}.AppImage.zsync"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        DEPENDS FlowCompute
        COMMENT "Building FlowCompute AppImage..."
    )
endif()
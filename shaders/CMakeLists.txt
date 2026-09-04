set(SHADER_SOURCES
    "axis/axis.vert"
    "axis/axis.frag"
    "model/model.vert"
    "model/model.frag"
    "mesh/mesh.vert"
    "mesh/mesh.frag"
    "color/color.vert"
    "color/color.frag"
    "lines/lines.vert"
    "lines/lines.frag"
)

set(LOCAL_SPIRV_FILES "")

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_DIR ${SHADER} DIRECTORY)
    
    # Get the full name instead of just the extension
    get_filename_component(SHADER_NAME ${SHADER} NAME) 
    
    # Output to the build directory
    set(SPIRV_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_DIR}/${SHADER_NAME}.spv")
    
    # Set the output subdirectory
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_DIR}")
    
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${GLSL_VALIDATOR} -V ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling Vulkan shader: ${SHADER} -> ${SHADER_DIR}/${SHADER_NAME}.spv"
    )
    
    list(APPEND LOCAL_SPIRV_FILES ${SPIRV_FILE})
endforeach()

# Create target to force the commands to execute
add_custom_target(CompileShaders DEPENDS ${LOCAL_SPIRV_FILES})

# Export list to the parent scope
set(COMPILED_SPIRV_FILES ${LOCAL_SPIRV_FILES} PARENT_SCOPE)
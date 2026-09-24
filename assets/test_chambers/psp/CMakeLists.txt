#########################################
## Level generation for the PSP target ##
#########################################

# The N64's per level steps (Blender export, skeletool64 with
# export_level.lua) with --psp and the shared material list.

include("${ASSETS_DIR}/test_chambers/level_lists.cmake")

set(PSP_LEVELS_SRC_DIR "${ASSETS_DIR}/test_chambers")
set(PSP_LEVELS_FBX_DIR "${PROJECT_BINARY_DIR}/assets/test_chambers")
set(PSP_LEVELS_OUT_DIR "${GENERATED_CODE_DIR}/assets/test_chambers")

set(PSP_EXPORT_LEVEL "${TOOLS_DIR}/level_scripts/export_level.lua")

file(GLOB PSP_EXPORT_SCRIPTS "${TOOLS_DIR}/level_scripts/*.lua")

# Textures come from the N64 build's extraction.
set(PSP_LEVEL_EXTRACTED_TEXTURE
    "${PAK_MODIFIED_DIR}/materials/gamepadui/portal_logo.png")

function(_add_psp_level LEVEL_NAME LEVEL_OUTPUT_LIST ANIM_OUTPUT_LIST)
    set(LEVEL_BLEND "${PSP_LEVELS_SRC_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.blend")
    set(LEVEL_YAML  "${PSP_LEVELS_SRC_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.yaml")
    set(LEVEL_FBX   "${PSP_LEVELS_FBX_DIR}/${LEVEL_NAME}/${LEVEL_NAME}.fbx")

    add_custom_command(
        DEPENDS
            ${EXPORT_FBX} ${LEVEL_BLEND}
        OUTPUT
            ${LEVEL_FBX}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${LEVEL_FBX}>"
        COMMAND
            ${Blender_EXECUTABLE}
            ${LEVEL_BLEND} --background --python ${EXPORT_FBX} -- ${LEVEL_FBX}
        COMMENT
            "Exporting $<PATH:RELATIVE_PATH,${LEVEL_BLEND},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    set(OUTPUT_FILE_NO_EXTENSION "${PSP_LEVELS_OUT_DIR}/${LEVEL_NAME}/${LEVEL_NAME}")
    set(LEVEL_OUTPUT_FILES
        "${OUTPUT_FILE_NO_EXTENSION}.h"
        "${OUTPUT_FILE_NO_EXTENSION}_geo.c"
    )
    set(ANIM_OUTPUT_FILES "")

    if (LEVEL_NAME IN_LIST ANIMATED_LEVELS)
        list(APPEND ANIM_OUTPUT_FILES "${OUTPUT_FILE_NO_EXTENSION}_anim.c")
    endif()

    add_custom_command(
        DEPENDS
            skeletool64::skeletool64
            ${PSP_EXPORT_SCRIPTS}
            ${LEVEL_FBX}
            ${LEVEL_YAML}
            "${ASSETS_DIR}/materials/static.skm.yaml"
            ${PSP_LEVEL_EXTRACTED_TEXTURE}
        OUTPUT
            ${LEVEL_OUTPUT_FILES}
            ${ANIM_OUTPUT_FILES}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE_NO_EXTENSION}>"
        COMMAND
            $<TARGET_FILE:skeletool64::skeletool64>
            --script ${PSP_EXPORT_LEVEL}
            --psp
            --psp-shared-materials static
            --fixed-point-scale ${SCENE_SCALE}
            --model-scale ${MODEL_SCALE}
            --name ${LEVEL_NAME}/${LEVEL_NAME}
            -m "${ASSETS_DIR}/materials/static.skm.yaml"
            --output "${OUTPUT_FILE_NO_EXTENSION}.h"
            ${LEVEL_FBX}
        # Texture paths are relative to the project root.
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Generating PSP level ${LEVEL_NAME}"
        VERBATIM
    )

    list(APPEND ${LEVEL_OUTPUT_LIST} ${LEVEL_OUTPUT_FILES})
    list(APPEND ${ANIM_OUTPUT_LIST} ${ANIM_OUTPUT_FILES})
    return(PROPAGATE ${LEVEL_OUTPUT_LIST} ${ANIM_OUTPUT_LIST})
endfunction()

set(PSP_LEVEL_FILES "")
set(PSP_LEVEL_ANIM_FILES "")

foreach(LEVEL ${LEVELS})
    _add_psp_level(${LEVEL} PSP_LEVEL_FILES PSP_LEVEL_ANIM_FILES)
endforeach()

add_custom_target(levels_psp_generated
    DEPENDS ${PSP_LEVEL_FILES} ${PSP_LEVEL_ANIM_FILES}
)

# The level list: same generator and output on both machines.
set(PSP_LEVEL_HEADERS ${PSP_LEVEL_FILES})
list(FILTER PSP_LEVEL_HEADERS INCLUDE REGEX "\.h$")

set(PSP_LEVEL_LIST "${PSP_LEVELS_OUT_DIR}/level_list.h")

add_custom_command(
    DEPENDS
        levels_psp_generated ${PSP_LEVEL_HEADERS}
        "${TOOLS_DIR}/models/generate_level_list.js" ${MODEL_LIST_UTILS}
    OUTPUT
        ${PSP_LEVEL_LIST}
    COMMAND
        ${NodeJs_EXECUTABLE} "${TOOLS_DIR}/models/generate_level_list.js"
        --psp
        ${PSP_LEVEL_LIST} ${PSP_LEVEL_HEADERS}
    WORKING_DIRECTORY
        ${PSP_LEVELS_OUT_DIR}
    COMMENT
        "Generating PSP level list"
    VERBATIM
)

add_custom_target(generated_level_list_psp
    DEPENDS ${PSP_LEVEL_LIST}
)

set(PSP_LEVEL_SOURCES ${PSP_LEVEL_FILES} ${PSP_LEVEL_ANIM_FILES})
list(FILTER PSP_LEVEL_SOURCES INCLUDE REGEX "\.c$")

add_library(levels_psp INTERFACE)
add_dependencies(levels_psp levels_psp_generated generated_level_list_psp)
target_sources(levels_psp INTERFACE
    ${PSP_LEVEL_SOURCES}
)
# The geometry references the shared material list by symbol.
target_link_libraries(levels_psp INTERFACE
    materials_psp
)

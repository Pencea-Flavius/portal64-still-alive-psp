#########################################
## Model generation for the PSP target ##
#########################################

# The N64's per model steps (Blender export, skeletool64 with the .flags
# file) with --psp and the shared material list.

include("${ASSETS_DIR}/models/model_lists.cmake")

set(PSP_MODELS_SRC_DIR "${ASSETS_DIR}/models")
set(PSP_MODELS_FBX_DIR "${PROJECT_BINARY_DIR}/assets/models")
set(PSP_MODELS_OUT_DIR "${GENERATED_CODE_DIR}/assets/models")

# The material list the geometry references. Models naming objects.skm.yaml
# or chell.skm.yaml still carry those themselves.
set(PSP_SHARED_MATERIAL_LIST static)

# Every model depends on every material file.
file(GLOB PSP_MODEL_MATERIALS "${ASSETS_DIR}/materials/*.skm.yaml")

# Textures come from the N64 build's extraction; assets/materials/psp
# fails the configure if it hasn't run.
set(PSP_MODEL_EXTRACTED_TEXTURE
    "${PAK_MODIFIED_DIR}/materials/gamepadui/portal_logo.png")

function(_add_psp_model MODEL_NAME MODEL_OUTPUT_LIST ANIM_OUTPUT_LIST)
    set(MODEL_BLEND "${PSP_MODELS_SRC_DIR}/${MODEL_NAME}.blend")
    set(MODEL_FLAGS "${PSP_MODELS_SRC_DIR}/${MODEL_NAME}.flags")
    set(MODEL_FBX   "${PSP_MODELS_FBX_DIR}/${MODEL_NAME}.fbx")

    add_custom_command(
        DEPENDS
            ${EXPORT_FBX} ${MODEL_BLEND}
        OUTPUT
            ${MODEL_FBX}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${MODEL_FBX}>"
        COMMAND
            ${Blender_EXECUTABLE}
            ${MODEL_BLEND} --background --python ${EXPORT_FBX} -- ${MODEL_FBX}
        COMMENT
            "Exporting $<PATH:RELATIVE_PATH,${MODEL_BLEND},${PROJECT_SOURCE_DIR}>"
        VERBATIM
    )

    set(OUTPUT_FILE_NO_EXTENSION "${PSP_MODELS_OUT_DIR}/${MODEL_NAME}")
    set(MODEL_OUTPUT_FILES
        "${OUTPUT_FILE_NO_EXTENSION}.h"
        "${OUTPUT_FILE_NO_EXTENSION}_geo.c"
    )
    set(ANIM_OUTPUT_FILES "")

    if (MODEL_NAME IN_LIST KEYFRAMED_ANIMATED_MODELS)
        list(APPEND ANIM_OUTPUT_FILES "${OUTPUT_FILE_NO_EXTENSION}_anim.c")
    endif()

    add_custom_command(
        DEPENDS
            skeletool64::skeletool64
            ${RUN_COMMAND}
            ${MODEL_FBX}
            ${MODEL_FLAGS}
            ${PSP_MODEL_MATERIALS}
            ${PSP_MODEL_EXTRACTED_TEXTURE}
        OUTPUT
            ${MODEL_OUTPUT_FILES}
            ${ANIM_OUTPUT_FILES}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "$<PATH:GET_PARENT_PATH,${OUTPUT_FILE_NO_EXTENSION}>"
        COMMAND
            ${Python3_EXECUTABLE} ${RUN_COMMAND}
            $<TARGET_FILE:skeletool64::skeletool64>
            ${MODEL_FBX}
            --psp
            --psp-shared-materials ${PSP_SHARED_MATERIAL_LIST}
            --fixed-point-scale ${SCENE_SCALE}
            --model-scale ${MODEL_SCALE}
            --name ${MODEL_NAME}
            --output "${OUTPUT_FILE_NO_EXTENSION}.h"
            @${MODEL_FLAGS}
        # Paths in .flags and material files are relative to the project root.
        WORKING_DIRECTORY
            ${PROJECT_SOURCE_DIR}
        COMMENT
            "Generating PSP model ${MODEL_NAME}"
        VERBATIM
    )

    list(APPEND ${MODEL_OUTPUT_LIST} ${MODEL_OUTPUT_FILES})
    list(APPEND ${ANIM_OUTPUT_LIST} ${ANIM_OUTPUT_FILES})
    return(PROPAGATE ${MODEL_OUTPUT_LIST} ${ANIM_OUTPUT_LIST})
endfunction()

# Everything is linked on the PSP; the lists stay apart because the dynamic
# model list generator reads one.
set(PSP_STATIC_MODEL_FILES "")
set(PSP_DYNAMIC_MODEL_FILES "")
set(PSP_ANIM_FILES "")

foreach(MODEL ${MODELS})
    set(PSP_MODEL_FILES "")

    _add_psp_model(${MODEL} PSP_MODEL_FILES PSP_ANIM_FILES)

    if (MODEL IN_LIST DYNAMIC_MODELS)
        list(APPEND PSP_DYNAMIC_MODEL_FILES ${PSP_MODEL_FILES})
    else()
        list(APPEND PSP_STATIC_MODEL_FILES ${PSP_MODEL_FILES})
    endif()
endforeach()

add_custom_target(models_psp_generated
    DEPENDS
        ${PSP_STATIC_MODEL_FILES}
        ${PSP_DYNAMIC_MODEL_FILES}
        ${PSP_ANIM_FILES}
)

set(PSP_MODEL_SOURCES ${PSP_STATIC_MODEL_FILES} ${PSP_DYNAMIC_MODEL_FILES} ${PSP_ANIM_FILES})
list(FILTER PSP_MODEL_SOURCES INCLUDE REGEX "\.c$")

add_library(models_psp INTERFACE)
add_dependencies(models_psp models_psp_generated)
target_sources(models_psp INTERFACE
    ${PSP_MODEL_SOURCES}
)
# Whatever links the models needs the material list too.
target_link_libraries(models_psp INTERFACE
    materials_psp
)

###############################
## Dynamic model list        ##
###############################

# The N64's generator with --psp: each entry is the model and its name,
# for the models dynamic_asset_loader.c indexes.
set(PSP_DYNAMIC_MODEL_HEADERS ${PSP_DYNAMIC_MODEL_FILES})
list(FILTER PSP_DYNAMIC_MODEL_HEADERS INCLUDE REGEX "\.h$")

set(PSP_DYNAMIC_NON_ANIMATED_MODEL_HEADERS "")
set(PSP_DYNAMIC_ANIMATED_MODEL_HEADERS "")

foreach(GENERATED_HEADER ${PSP_DYNAMIC_MODEL_HEADERS})
    cmake_path(
        RELATIVE_PATH GENERATED_HEADER
        BASE_DIRECTORY "${PSP_MODELS_OUT_DIR}"
        OUTPUT_VARIABLE MODEL_NAME
    )
    cmake_path(
        REMOVE_EXTENSION MODEL_NAME
        OUTPUT_VARIABLE MODEL_NAME
    )

    if (MODEL_NAME IN_LIST ANIMATED_MODELS)
        list(APPEND PSP_DYNAMIC_ANIMATED_MODEL_HEADERS ${GENERATED_HEADER})
    else()
        list(APPEND PSP_DYNAMIC_NON_ANIMATED_MODEL_HEADERS ${GENERATED_HEADER})
    endif()
endforeach()

set(PSP_DYNAMIC_MODEL_LIST_H "${PSP_MODELS_OUT_DIR}/dynamic_model_list.h")
set(PSP_DYNAMIC_MODEL_LIST_C "${PSP_MODELS_OUT_DIR}/dynamic_model_list.c")

add_custom_command(
    DEPENDS
        models_psp_generated ${PSP_DYNAMIC_NON_ANIMATED_MODEL_HEADERS}
        ${GEN_DYNAMIC_MODEL_LIST} ${MODEL_LIST_UTILS}
    OUTPUT
        ${PSP_DYNAMIC_MODEL_LIST_H} ${PSP_DYNAMIC_MODEL_LIST_C}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_DYNAMIC_MODEL_LIST}
        --psp
        ${PSP_DYNAMIC_MODEL_LIST_H}
        ${PSP_DYNAMIC_NON_ANIMATED_MODEL_HEADERS}
    COMMENT
        "Generating PSP dynamic model list"
    VERBATIM
)

set(PSP_DYNAMIC_ANIM_MODEL_LIST_H "${PSP_MODELS_OUT_DIR}/dynamic_animated_model_list.h")
set(PSP_DYNAMIC_ANIM_MODEL_LIST_C "${PSP_MODELS_OUT_DIR}/dynamic_animated_model_list.c")

add_custom_command(
    DEPENDS
        models_psp_generated ${PSP_DYNAMIC_ANIMATED_MODEL_HEADERS}
        ${GEN_DYNAMIC_ANIM_MODEL_LIST} ${MODEL_LIST_UTILS}
    OUTPUT
        ${PSP_DYNAMIC_ANIM_MODEL_LIST_H} ${PSP_DYNAMIC_ANIM_MODEL_LIST_C}
    COMMAND
        ${NodeJs_EXECUTABLE} ${GEN_DYNAMIC_ANIM_MODEL_LIST}
        --psp
        ${PSP_DYNAMIC_ANIM_MODEL_LIST_H}
        ${PSP_DYNAMIC_ANIMATED_MODEL_HEADERS}
    COMMENT
        "Generating PSP dynamic animated model list"
    VERBATIM
)

add_custom_target(generated_model_lists_psp
    DEPENDS
        ${PSP_DYNAMIC_MODEL_LIST_H} ${PSP_DYNAMIC_MODEL_LIST_C}
        ${PSP_DYNAMIC_ANIM_MODEL_LIST_H} ${PSP_DYNAMIC_ANIM_MODEL_LIST_C}
)

add_library(model_lists_psp INTERFACE)
add_dependencies(model_lists_psp generated_model_lists_psp)
target_sources(model_lists_psp INTERFACE
    ${PSP_DYNAMIC_MODEL_LIST_C}
    ${PSP_DYNAMIC_ANIM_MODEL_LIST_C}
)
target_link_libraries(model_lists_psp INTERFACE
    models_psp
)

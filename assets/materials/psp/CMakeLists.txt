############################################
## Material generation for the PSP target ##
############################################

# ui.skm.yaml's textures are in the repository (except the Valve logo);
# static.skm.yaml and hud.skm.yaml need the texture extraction.

set(PSP_MATERIALS_DIR "${GENERATED_CODE_DIR}/assets/materials")

# The PSP's copy of ui.skm.yaml with PSP button prompts (see
# assets/images/psp/README.md). Paths are made absolute since it is written
# into the build tree.
file(READ "${ASSETS_DIR}/materials/ui.skm.yaml" UI_MATERIAL_YAML)
string(REPLACE "\"../../" "\"${PROJECT_SOURCE_DIR}/" UI_MATERIAL_YAML "${UI_MATERIAL_YAML}")
string(REPLACE "\"../" "\"${ASSETS_DIR}/" UI_MATERIAL_YAML "${UI_MATERIAL_YAML}")
string(REPLACE "${ASSETS_DIR}/images/button_icons.png" "${ASSETS_DIR}/images/psp/button_icons.png" UI_MATERIAL_YAML "${UI_MATERIAL_YAML}")
set(UI_MATERIAL_FILE "${CMAKE_CURRENT_BINARY_DIR}/ui.skm.yaml")
file(GENERATE OUTPUT "${UI_MATERIAL_FILE}" CONTENT "${UI_MATERIAL_YAML}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ASSETS_DIR}/materials/ui.skm.yaml")

set(UI_MATERIAL_TEXTURES
    "${ASSETS_DIR}/fonts/dejavu_sans/dejavu_sans_0.png"
    "${ASSETS_DIR}/fonts/dejavu_sans/dejavu_sans_1.png"
    "${ASSETS_DIR}/fonts/dejavu_sans/dejavu_sans_2.png"
    "${ASSETS_DIR}/fonts/dejavu_sans/dejavu_sans_3.png"
    "${ASSETS_DIR}/fonts/dejavu_sans/dejavu_sans_4.png"
    "${ASSETS_DIR}/fonts/liberation_mono/liberation_mono_0.png"
    "${ASSETS_DIR}/images/psp/button_icons.png"
    "${ASSETS_DIR}/images/credits_icons.png"
    "${ASSETS_DIR}/images/github_qr.png"
    "${ASSETS_DIR}/images/rounded_corners.png"
)

set(UI_MATERIAL_EXTRACTED_TEXTURE
    "${PAK_MODIFIED_DIR}/materials/gamepadui/portal_logo.png")

# Textures come from the N64 build's extraction.
if (NOT EXISTS "${UI_MATERIAL_EXTRACTED_TEXTURE}")
    message(FATAL_ERROR
        "Game textures have not been extracted. Build the N64 target once first, "
        "which unpacks them into portal_pak_modified/.")
endif()

add_custom_command(
    OUTPUT
        "${PSP_MATERIALS_DIR}/ui.h"
        "${PSP_MATERIALS_DIR}/ui_mat.c"
    COMMAND
        ${CMAKE_COMMAND} -E make_directory "${PSP_MATERIALS_DIR}"
    COMMAND
        $<TARGET_FILE:skeletool64::skeletool64>
            --psp
            --name ui
            -m "${UI_MATERIAL_FILE}"
            --material-output
            --output "${PSP_MATERIALS_DIR}/ui.h"
    DEPENDS
        skeletool64::skeletool64
        "${UI_MATERIAL_FILE}"
        ${UI_MATERIAL_TEXTURES}
        "${UI_MATERIAL_EXTRACTED_TEXTURE}"
    COMMENT
        "Generating PSP materials for ui.skm.yaml"
    VERBATIM
)

# Level, HUD and chapter image materials, all from extracted textures. PNG
# changes are tracked through the extraction (PAK_MODIFIED_DIR).
foreach(PSP_MATERIAL_SET static hud images)
    add_custom_command(
        OUTPUT
            "${PSP_MATERIALS_DIR}/${PSP_MATERIAL_SET}.h"
            "${PSP_MATERIALS_DIR}/${PSP_MATERIAL_SET}_mat.c"
        COMMAND
            ${CMAKE_COMMAND} -E make_directory "${PSP_MATERIALS_DIR}"
        COMMAND
            $<TARGET_FILE:skeletool64::skeletool64>
                --psp
                --name ${PSP_MATERIAL_SET}
                -m "${ASSETS_DIR}/materials/${PSP_MATERIAL_SET}.skm.yaml"
                --material-output
                --output "${PSP_MATERIALS_DIR}/${PSP_MATERIAL_SET}.h"
        DEPENDS
            skeletool64::skeletool64
            "${ASSETS_DIR}/materials/${PSP_MATERIAL_SET}.skm.yaml"
            "${UI_MATERIAL_EXTRACTED_TEXTURE}"
        # Texture paths are relative to the project root.
        WORKING_DIRECTORY
            "${PROJECT_SOURCE_DIR}"
        COMMENT
            "Generating PSP materials for ${PSP_MATERIAL_SET}.skm.yaml"
        VERBATIM
    )
endforeach()

add_custom_target(materials_psp_generated
    DEPENDS
        "${PSP_MATERIALS_DIR}/ui.h" "${PSP_MATERIALS_DIR}/ui_mat.c"
        "${PSP_MATERIALS_DIR}/static.h" "${PSP_MATERIALS_DIR}/static_mat.c"
        "${PSP_MATERIALS_DIR}/hud.h" "${PSP_MATERIALS_DIR}/hud_mat.c"
        "${PSP_MATERIALS_DIR}/images.h" "${PSP_MATERIALS_DIR}/images_mat.c"
)

add_library(materials_psp INTERFACE)
add_dependencies(materials_psp materials_psp_generated)
target_sources(materials_psp INTERFACE
    "${PSP_MATERIALS_DIR}/ui_mat.c"
    "${PSP_MATERIALS_DIR}/static_mat.c"
    "${PSP_MATERIALS_DIR}/hud_mat.c"
    "${PSP_MATERIALS_DIR}/images_mat.c"
)

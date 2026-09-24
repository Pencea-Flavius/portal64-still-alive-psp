# The renderer test runs models through the real generator.
set(PSP_TEST_MODEL_DIR "${CMAKE_CURRENT_BINARY_DIR}/codegen/graphics/psp")

# A real game model, to exercise more of the pipeline.
set(PSP_GAME_MODEL_BLEND "${PROJECT_SOURCE_DIR}/assets/models/cube/cube.blend")
set(PSP_GAME_MODEL_FBX   "${PSP_TEST_MODEL_DIR}/companion_cube.fbx")
set(PSP_GAME_MODEL_TEXTURE
    "${PROJECT_SOURCE_DIR}/portal_pak_modified/materials/models/props/metal_box.png")

# Textures come from the N64 build's extraction.
if (NOT EXISTS "${PSP_GAME_MODEL_TEXTURE}")
    message(FATAL_ERROR
        "Game textures have not been extracted. Build the N64 target once first, "
        "which unpacks them into portal_pak_modified/.")
endif()

add_custom_command(
    OUTPUT
        "${PSP_GAME_MODEL_FBX}"
    COMMAND
        ${CMAKE_COMMAND} -E make_directory "${PSP_TEST_MODEL_DIR}"
    COMMAND
        ${Blender_EXECUTABLE} "${PSP_GAME_MODEL_BLEND}"
            --background --python "${TOOLS_DIR}/models/export_fbx.py"
            -- "${PSP_GAME_MODEL_FBX}"
    DEPENDS
        "${PSP_GAME_MODEL_BLEND}"
        "${TOOLS_DIR}/models/export_fbx.py"
    COMMENT
        "Exporting companion cube"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PSP_TEST_MODEL_DIR}/companion_cube_geo.c"
        "${PSP_TEST_MODEL_DIR}/companion_cube.h"
    COMMAND
        $<TARGET_FILE:skeletool64::skeletool64>
            "${PSP_GAME_MODEL_FBX}"
            --psp
            --fixed-point-scale 128
            --model-scale 0.01
            --name companion_cube
            -m assets/materials/static.skm.yaml
            --default-material cube
            --output "${PSP_TEST_MODEL_DIR}/companion_cube.h"
    DEPENDS
        skeletool64::skeletool64
        "${PSP_GAME_MODEL_FBX}"
        "${PROJECT_SOURCE_DIR}/assets/materials/static.skm.yaml"
        "${PSP_GAME_MODEL_TEXTURE}"
    # Texture paths are relative to the project root.
    WORKING_DIRECTORY
        "${PROJECT_SOURCE_DIR}"
    COMMENT
        "Generating PSP companion cube"
    VERBATIM
)

add_custom_command(
    OUTPUT
        "${PSP_TEST_MODEL_DIR}/test_cube_geo.c"
        "${PSP_TEST_MODEL_DIR}/test_cube.h"
    COMMAND
        ${CMAKE_COMMAND} -E make_directory "${PSP_TEST_MODEL_DIR}"
    COMMAND
        $<TARGET_FILE:skeletool64::skeletool64>
            --psp
            --fixed-point-scale 128
            --name test_cube
            -m "${SRC_DIR}/graphics/psp/test_cube_materials.yaml"
            -o "${PSP_TEST_MODEL_DIR}/test_cube.c"
            "${SRC_DIR}/graphics/psp/test_cube.obj"
    DEPENDS
        skeletool64::skeletool64
        "${SRC_DIR}/graphics/psp/test_cube.obj"
        "${SRC_DIR}/graphics/psp/test_cube_materials.yaml"
        "${SRC_DIR}/graphics/psp/test_cube.png"
    COMMENT
        "Generating PSP test model"
    VERBATIM
)

add_executable(psp_boot_test
    "${PSP_TEST_MODEL_DIR}/test_cube_geo.c"
    "${PSP_TEST_MODEL_DIR}/companion_cube_geo.c"

    psp_boot_test.c

    boot_psp.c
    cartridge_psp.c
    controller_psp.c
    display_psp.c
    time_psp.c

    ../../font/psp/font_images.c

    # The render state builds matrices from transforms.
    ../../math/transform.c
    ../../math/matrix.c
    ../../math/quaternion.c
    ../../math/vector3.c
    ../../math/mathf.c

    ../../graphics/psp/psp_model_render.c
    ../../graphics/psp/psp_render.c
    ../../graphics/psp/render_state.c
    ../../graphics/psp/render_test_psp.c

    ultra64/os_message_psp.c
    ultra64/os_thread_psp.c
    ultra64/os_time_psp.c
)

target_include_directories(psp_boot_test PRIVATE
    "${SRC_DIR}"
    "${PSP_TEST_MODEL_DIR}"
    "$<PATH:GET_PARENT_PATH,${GENERATED_CODE_DIR}>"
    "${SRC_DIR}"

    # PSP only, like FindLibultra.cmake for the N64; resolves <ultra64.h>.
    "${CMAKE_CURRENT_SOURCE_DIR}/ultra64"
)

target_link_libraries(psp_boot_test PRIVATE
    platform
    materials_psp
)

# The version string is baked into either build.
add_dependencies(psp_boot_test version_header)

create_pbp_file(
    TARGET  psp_boot_test
    TITLE   "Portal 64 PSP"
    # Same as the game's, in the top level CMakeLists.txt.
    MEMSIZE 1
)

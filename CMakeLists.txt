cmake_minimum_required(VERSION 3.28)
project(portal64 C ASM)
set(C_STANDARD 17)
set(C_STANDARD_REQUIRED TRUE)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(AddExternalProjectExecutable)
include(ListOption)

list_option(AUDIO_LANGUAGES "english"
    "Game audio languages to include"
    "${SUPPORTED_AUDIO_LANGUAGES}"
)
list_option(TEXT_LANGUAGES "all"
    "Text languages to include"
    "${SUPPORTED_TEXT_LANGUAGES}"
)

# Programs used in build
find_package(Blender                    3.6 EXACT        REQUIRED)
find_package(Python3                                     REQUIRED)

# Git: the version string is baked into either build.
find_program(Git_EXECUTABLE             git              REQUIRED)

# Node: both targets generate model lists with it.
find_package(NodeJs                     18.3.0           REQUIRED)

if (N64)
    find_program(FFmpeg_EXECUTABLE      ffmpeg           REQUIRED)
    find_package(ImageMagickConvert     6.9.12.85...<7   REQUIRED)
    find_program(SFZ2N64_EXECUTABLE     sfz2n64          REQUIRED)
    find_program(Sox_EXECUTABLE         sox              REQUIRED)
    find_program(VPK_EXECUTABLE         vpk              REQUIRED)
    find_program(VTF2PNG_EXECUTABLE     vtf2png          REQUIRED)
endif()

# skeletool is a host tool, built as an external project, used by both targets.
add_external_project_executable(skeletool64)

# Source and asset file directories
set(ASM_DIR            "${PROJECT_SOURCE_DIR}/asm")
set(ASSETS_DIR         "${PROJECT_SOURCE_DIR}/assets")
set(LINKER_DIR         "${PROJECT_SOURCE_DIR}/linker")
set(SRC_DIR            "${PROJECT_SOURCE_DIR}/src")
set(TOOLS_DIR          "${PROJECT_SOURCE_DIR}/tools")
set(VPK_DIR            "${PROJECT_SOURCE_DIR}/vpk")

# Blender files use metres and exported FBX centimetres; MODEL_SCALE converts
# back. SCENE_SCALE is the render space scale. Shared by both targets.
set(MODEL_SCALE 0.01)
set(SCENE_SCALE 128)

set(EXPORT_FBX                  "${TOOLS_DIR}/models/export_fbx.py")
set(MODEL_LIST_UTILS            "${TOOLS_DIR}/models/model_list_utils.js")
set(GEN_DYNAMIC_MODEL_LIST      "${TOOLS_DIR}/models/generate_dynamic_model_list.js")
set(GEN_DYNAMIC_ANIM_MODEL_LIST "${TOOLS_DIR}/models/generate_dynamic_animated_model_list.js")
set(RUN_COMMAND                 "${TOOLS_DIR}/run_command.py")

# Extracted/generated file output directories
set(GENERATED_CODE_DIR "${PROJECT_BINARY_DIR}/codegen")
set(LINKER_SCRIPT_DIR  "${PROJECT_BINARY_DIR}/ld")
set(PAK_DIR            "${PROJECT_SOURCE_DIR}/portal_pak_dir")
set(PAK_MODIFIED_DIR   "${PROJECT_SOURCE_DIR}/portal_pak_modified")

#########################
## Version information ##
#########################

set(GEN_VERSION_HEADER "${PROJECT_SOURCE_DIR}/cmake/GenVersionHeader.cmake")
set(VERSION_H          "${GENERATED_CODE_DIR}/version.h")

add_custom_target(version_header
    DEPENDS
        ${GEN_VERSION_HEADER}
    BYPRODUCTS
        ${VERSION_H}
    COMMAND
        ${CMAKE_COMMAND}
        -D Git_EXECUTABLE=${Git_EXECUTABLE}
        -D OUTPUT_FILE=${VERSION_H}
        -P ${GEN_VERSION_HEADER}
    WORKING_DIRECTORY
        ${PROJECT_SOURCE_DIR}
    VERBATIM
)

add_library(platform INTERFACE)

# Drawing code is split per machine into n64/ and psp/ directories; only one
# is on the include path, chosen here, so shared code includes headers
# unqualified.
if (N64)
    find_package(Libultra REQUIRED)
    target_link_libraries(platform INTERFACE libultra::libultra)
    target_include_directories(platform INTERFACE
        "${SRC_DIR}/effects/n64"
        "${SRC_DIR}/font/n64"
        "${SRC_DIR}/graphics/n64"
        "${SRC_DIR}/levels/n64"
        "${SRC_DIR}/menu/n64"
        "${SRC_DIR}/physics/n64"
        "${SRC_DIR}/scene/n64"
        "${SRC_DIR}/sk64/n64"
    )
elseif (PSP)
    # Include and link directories are injected globally by pspdev.cmake
    target_link_libraries(platform INTERFACE
        pspdebug
        pspdisplay
        pspge
        pspctrl
        pspgu
        # The VFPU build of libpspgum: matrix work on the vector unit.
        pspgum_vfpu
        pspvfpu
        pspaudio
        psppower
        # Boots the Media Engine for the mixer (src/system/psp/audio_me.c).
        me-core
    )
    target_include_directories(platform INTERFACE
        "${SRC_DIR}/effects/psp"
        "${SRC_DIR}/font/psp"
        "${SRC_DIR}/graphics/psp"
        "${SRC_DIR}/levels/psp"
        "${SRC_DIR}/menu/psp"
        "${SRC_DIR}/physics/psp"
        "${SRC_DIR}/scene/psp"
        "${SRC_DIR}/sk64/psp"
    )
else()
    message(FATAL_ERROR "Unsupported target platform. Did you specify a toolchain file?")
endif()

if (N64)
    add_subdirectory(${ASSETS_DIR})
    add_subdirectory(${ASM_DIR})
    add_subdirectory(${SRC_DIR})
    add_subdirectory(${LINKER_DIR})
    add_subdirectory(${VPK_DIR})

    add_library(code_segment STATIC)
    target_link_libraries(code_segment PRIVATE
        engine
        font_layout_tables
        level_list
        materials_core
        model_lists
        models_static
        sound_lookup_tables
        string_lookup_tables
    )

    add_executable(portal)
    target_link_libraries(portal PRIVATE
        platform

        asm
        code_segment
        materials_images
        levels
        level_animations
        model_animations
        models_dynamic
        string_data_tables
    )

    add_dependencies(portal linker_script)
    target_linker_script(portal "$<TARGET_PROPERTY:linker_script,SCRIPT_FILE>")

    add_n64_rom(portal)
elseif (PSP)
    # pspdev sets no optimisation level. No -ffast-math or FMA contraction, so
    # shared physics and the portal cutter keep IEEE results.
    add_compile_options(-O2 -G0 -fno-strict-aliasing -ffp-contract=off)

    # String tables, the same on either machine.
    add_subdirectory(${ASSETS_DIR}/fonts)
    add_subdirectory(${ASSETS_DIR}/translations)
    add_subdirectory(${ASSETS_DIR}/materials/psp)
    add_subdirectory(${ASSETS_DIR}/models/psp)
    add_subdirectory(${ASSETS_DIR}/test_chambers/psp)
    add_subdirectory(${ASSETS_DIR}/sound/psp)

    add_subdirectory(${SRC_DIR})
    add_subdirectory(${SRC_DIR}/system/psp)

    # The game: one executable, as the PSP has no segments.
    add_executable(portal)
    target_link_libraries(portal PRIVATE
        platform

        engine
        font_layout_tables
        levels_psp
        materials_psp
        model_lists_psp
        models_psp
        sound_lookup_tables_psp
        string_lookup_tables
        string_data_tables
    )

    # XMB assets in assets/psp/xmb/. PIC1 is the main menu's scene; SND0 is the
    # radio loop, encoded with Sony's at3tool (ATRAC3plus), which unlike
    # atracdenc's ATRAC3 plays on hardware:
    #     at3tool -e -wholeloop looping_radio_mix.wav SND0.AT3
    set(XMB_DIR "${PROJECT_SOURCE_DIR}/assets/psp/xmb")

    create_pbp_file(
        TARGET  portal
        TITLE   "Portal"
        ICON_PATH "${XMB_DIR}/ICON0.PNG"
        PREVIEW_PATH "${XMB_DIR}/PIC0.PNG"
        BACKGROUND_PATH "${XMB_DIR}/PIC1.PNG"
        MUSIC_PATH "${XMB_DIR}/SND0.AT3"
        # 1 asks for all of the machine's memory (create_pbp_file defaults to 2,
        # limited, for the Vita); the game does not fit the limited partition.
        MEMSIZE 1
    )
endif()

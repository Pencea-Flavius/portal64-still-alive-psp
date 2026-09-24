#############################################
## Sound id tables for the PSP target      ##
#############################################

# Sound ids from the converted sounds' names and banks. The PSP build uses
# the N64 build's converted sounds.

include("${ASSETS_DIR}/sound/sound_lists.cmake")

set(PSP_AUDIO_OUT_DIR "${GENERATED_CODE_DIR}/assets/audio")
set(PSP_N64_SOUND_DIR "${PROJECT_SOURCE_DIR}/build/n64/assets/sound")

if (NOT EXISTS "${PSP_N64_SOUND_DIR}")
    message(FATAL_ERROR
        "Game sounds have not been converted. Build the N64 target once first, "
        "which converts them into build/n64/assets/sound/.")
endif()

# Same order as assets/sound/CMakeLists.txt, which the generator requires.
set(PSP_SOUND_NAMES "")

foreach(SOUND_SCRIPT ${SOUND_SCRIPTS})
    cmake_path(REMOVE_EXTENSION SOUND_SCRIPT OUTPUT_VARIABLE SOUND_NAME)
    list(APPEND PSP_SOUND_NAMES ${SOUND_NAME})
endforeach()

foreach(LANGUAGE ${AUDIO_LANGUAGES_LIST})
    foreach(SOUND_SCRIPT ${LOCALIZED_SOUND_SCRIPTS})
        string(TOUPPER "${LANGUAGE}_EXCLUDED_LOCALIZED_SOUND_SCRIPTS" EXCLUDED_SOUNDS)
        if (EXCLUDED_SOUNDS AND (${SOUND_SCRIPT} IN_LIST ${EXCLUDED_SOUNDS}))
            continue()
        endif()

        cmake_path(REMOVE_EXTENSION SOUND_SCRIPT OUTPUT_VARIABLE SOUND_NAME)
        cmake_path(GET SOUND_NAME FILENAME SOUND_FILENAME)
        cmake_path(
            REPLACE_FILENAME SOUND_NAME
            "${LANGUAGE}_${SOUND_FILENAME}"
            OUTPUT_VARIABLE LOCALIZED_NAME
        )

        list(APPEND PSP_SOUND_NAMES ${LOCALIZED_NAME})
    endforeach()
endforeach()

foreach(MUSIC_SCRIPT ${MUSIC_SCRIPTS})
    cmake_path(REMOVE_EXTENSION MUSIC_SCRIPT OUTPUT_VARIABLE MUSIC_NAME)
    list(APPEND PSP_SOUND_NAMES ${MUSIC_NAME})
endforeach()

# Sounds in a bank are not listed again.
set(PSP_SOUND_TABLE_INPUTS "")

foreach(INS_BANK ${INSTRUMENT_BANKS})
    list(APPEND PSP_SOUND_TABLE_INPUTS "${ASSETS_DIR}/sound/${INS_BANK}")
endforeach()

foreach(SOUND_NAME ${PSP_SOUND_NAMES})
    if (NOT "${SOUND_NAME}.ins" IN_LIST INSTRUMENT_BANKS)
        list(APPEND PSP_SOUND_TABLE_INPUTS "${SOUND_NAME}.aifc")
    endif()
endforeach()

set(PSP_CLIPS_H     "${PSP_AUDIO_OUT_DIR}/clips.h")
set(PSP_LANGUAGES_H "${PSP_AUDIO_OUT_DIR}/languages.h")
set(PSP_LANGUAGES_C "${PSP_AUDIO_OUT_DIR}/languages.c")

add_custom_command(
    DEPENDS
        "${TOOLS_DIR}/sound/generate_sound_ids.js"
        "${ASSETS_DIR}/sound/sound_lists.cmake"
    OUTPUT
        ${PSP_CLIPS_H} ${PSP_LANGUAGES_H} ${PSP_LANGUAGES_C}
    COMMAND
        ${CMAKE_COMMAND} -E make_directory "${PSP_AUDIO_OUT_DIR}"
    COMMAND
        ${NodeJs_EXECUTABLE} "${TOOLS_DIR}/sound/generate_sound_ids.js"
        --out-dir ${PSP_AUDIO_OUT_DIR} ${PSP_SOUND_TABLE_INPUTS}
    # Names are paths relative to the working directory.
    WORKING_DIRECTORY
        ${PSP_N64_SOUND_DIR}
    COMMENT
        "Generating PSP sound id tables"
    VERBATIM
)

# The sounds as raw PCM, next to EBOOT.PBP. See
# tools/sound/generate_psp_sound_pack.js.
set(PSP_SOUND_PACK "${CMAKE_BINARY_DIR}/sounds.psp")

add_custom_command(
    DEPENDS
        "${TOOLS_DIR}/sound/generate_psp_sound_pack.js"
        "${ASSETS_DIR}/sound/sound_lists.cmake"
    OUTPUT
        ${PSP_SOUND_PACK}
    COMMAND
        ${NodeJs_EXECUTABLE} "${TOOLS_DIR}/sound/generate_psp_sound_pack.js"
        --out ${PSP_SOUND_PACK}
        --pak-modified-sound-dir "${PAK_MODIFIED_DIR}/sound"
        ${PSP_SOUND_TABLE_INPUTS}
    WORKING_DIRECTORY
        ${PSP_N64_SOUND_DIR}
    COMMENT
        "Generating PSP sound pack"
    VERBATIM
)

add_custom_target(sound_tables_psp
    DEPENDS ${PSP_CLIPS_H} ${PSP_LANGUAGES_H} ${PSP_LANGUAGES_C} ${PSP_SOUND_PACK}
)

add_library(sound_lookup_tables_psp INTERFACE)
add_dependencies(sound_lookup_tables_psp sound_tables_psp)
target_sources(sound_lookup_tables_psp INTERFACE
    ${PSP_LANGUAGES_C}
)

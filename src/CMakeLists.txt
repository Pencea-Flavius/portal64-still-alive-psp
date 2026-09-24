###############
## Game code ##
###############

option(DEBUGGER      "Build with support for hardware debugging")
option(GFX_VALIDATOR "Build with display list validator")
option(RSP_PROFILER  "Build with RSP performance profiler")

add_library(engine INTERFACE)

target_include_directories(engine INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<PATH:GET_PARENT_PATH,${GENERATED_CODE_DIR}>"
)

target_link_libraries(engine INTERFACE
    platform
)

target_sources(engine INTERFACE
    audio/clips.c
    audio/soundplayer.c
    controls/controller_actions.c
    controls/rumble_pak_clip.c
    decor/decor_object.c
    decor/decor_object_list.c
    effects/effect_definitions.c
    effects/effects.c
    effects/particle_effect.c
    effects/portal_trail.c
    font/font.c
    graphics/color.c
    graphics/render_scene.c
    graphics/screen_clipper.c
    levels/credits.c
    levels/cutscene_runner.c
    levels/intro.c
    levels/levels.c
    levels/material_state.c
    levels/static_render.c
    locales/locales.c
    math/basis.c
    math/box2d.c
    math/box3d.c
    math/boxs16.c
    math/mathf.c
    math/matrix.c
    math/plane.c
    math/quaternion.c
    math/ray.c
    math/rotated_box.c
    math/transform.c
    math/vector2.c
    math/vector2s16.c
    math/vector3.c
    math/vector4.c
    menu/audio_options.c
    menu/cheat_codes.c
    menu/confirmation_dialog.c
    menu/controls.c
    menu/game_menu.c
    menu/gameplay_options.c
    menu/joystick_options.c
    menu/landing_menu.c
    menu/load_game.c
    menu/main_menu.c
    menu/menu.c
    menu/menu_builder.c
    menu/new_game_menu.c
    menu/options_menu.c
    menu/save_game_menu.c
    menu/savefile_list.c
    menu/tabs.c
    menu/text_manipulation.c
    menu/video_options.c
    physics/collision_box.c
    physics/collision_capsule.c
    physics/collision_cylinder.c
    physics/collision_object.c
    physics/collision_point.c
    physics/collision_quad.c
    physics/collision_scene.c
    physics/collision_sphere.c
    physics/collision_tetrahedron.c
    physics/contact_insertion.c
    physics/contact_solver.c
    physics/epa.c
    physics/gjk.c
    physics/line.c
    physics/compound_collider.c
    physics/mesh_collider.c
    physics/point_constraint.c
    physics/raycasting.c
    physics/rigid_body.c
    physics/world.c
    player/grab_rotation.c
    player/player.c
    player/player_rumble_clips.c
    savefile/checkpoint.c
    savefile/savefile.c
    savefile/scene_serialize.c
    savefile/serializer.c
    scene/ball.c
    scene/ball_catcher.c
    scene/ball_launcher.c
    scene/box_dropper.c
    scene/button.c
    scene/camera.c
    scene/clock.c
    scene/debug_scene.c
    scene/door.c
    scene/doorway_cover.c
    scene/dynamic_render_list.c
    scene/dynamic_scene.c
    scene/elevator.c
    scene/fizzler.c
    scene/hud.c
    scene/incinerator.c
    scene/laser.c
    scene/pedestal.c
    scene/point_light.c
    scene/portal.c
    scene/portal_gun.c
    scene/portal_render.c
    scene/portal_surface.c
    scene/portal_surface_generator.c
    scene/render_plan.c
    scene/scene.c
    scene/scene_animator.c
    scene/security_camera.c
    scene/signage.c
    scene/signals.c
    scene/switch.c
    scene/trigger_listener.c
    scene/turret.c
    sk64/skeletool_animator.c
    sk64/skeletool_armature.c
    strings/translations.c
    util/frame_time.c
    util/memory.c
    util/profile.c
    util/sort.c
    util/string.c
)

if (DEBUGGER)
    target_sources(engine INTERFACE
        debugger/debug.c
        debugger/usb.c
    )
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_DEBUGGER
    )
endif()

if (GFX_VALIDATOR)
    target_sources(engine INTERFACE
        gfxvalidator/command_printer.c
        gfxvalidator/error_printer.c
        gfxvalidator/validator.c
    )
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_GFX_VALIDATOR
    )
endif()

# The version header, for either machine.
add_dependencies(engine version_header)

###########################
## Library-specific code ##
###########################

# One of these two lists: each file is half of an n64/psp pair.

if (N64)

target_sources(engine INTERFACE
    decor/n64/decor_render.c
    effects/n64/particle_effect_render.c
    effects/n64/portal_trail_render.c
    font/n64/font_images.c
    font/n64/font_render.c
    graphics/n64/graphics.c
    graphics/n64/image.c
    graphics/n64/initgfx.c
    graphics/n64/render_scene_generate.c
    levels/n64/credits_render.c
    levels/n64/intro_render.c
    levels/n64/level_materials.c
    graphics/n64/render_state.c
    physics/n64/debug_renderer.c
    menu/n64/confirmation_dialog_render.c
    menu/n64/controls_render.c
    menu/n64/landing_menu_render.c
    menu/n64/main_menu_render.c
    menu/n64/menu_builder_render.c
    menu/n64/menu_render.c
    menu/n64/new_game_menu_render.c
    menu/n64/options_menu_render.c
    menu/n64/savefile_list_render.c
    scene/n64/ball_render.c
    scene/n64/camera_render.c
    scene/n64/clock_digits.c
    scene/n64/debug_scene_render.c
    scene/n64/doorway_cover_render.c
    scene/n64/fizzler_particles.c
    scene/n64/pedestal_render.c
    scene/n64/hud_render.c
    scene/n64/laser_render.c
    scene/n64/point_light_render.c
    scene/n64/portal_cover_render.c
    scene/n64/portal_gun_render.c
    scene/n64/portal_surface_gfx.c
    scene/n64/turret_render.c
    scene/n64/render_plan_execute.c
    scene/n64/scene_render.c
    scene/n64/signage_render.c
    scene/n64/scene_animator_transforms.c
    sk64/n64/skeletool_animation_data.c
    sk64/n64/skeletool_render.c

    system/libultra/audio_libultra.c
    system/libultra/boot_libultra.c
    system/libultra/cartridge_libultra.c
    system/libultra/controller_libultra.c
    system/libultra/display_libultra.c
    system/libultra/rsp_scheduler_libultra.c
    system/libultra/time_libultra.c

    # Unused upstream libultra rendering, kept building on the N64 only.
    graphics/debug_render.c
    materials/light.c
    materials/point_light_rendered.c
    materials/shadow_caster.c
    materials/subject.c
    scene/shadow_map.c
    scene/shadow_renderer.c

    levels/n64/level_load.c
    strings/n64/translations_load.c
    main.c
    player/n64/player_render.c
    util/n64/assert.c
    util/n64/dynamic_asset_loader.c
)

else()

target_sources(engine INTERFACE
    decor/psp/decor_render.c
    effects/psp/particle_effect_render.c
    effects/psp/portal_trail_render.c
    font/psp/font_images.c
    font/psp/font_render.c
    graphics/psp/graphics.c
    graphics/psp/psp_image.c
    graphics/psp/psp_model_render.c
    graphics/psp/psp_render.c
    graphics/psp/render_scene_generate.c
    graphics/psp/render_state.c
    levels/psp/credits_render.c
    levels/psp/intro_render.c
    levels/psp/level_materials.c
    menu/psp/confirmation_dialog_render.c
    menu/psp/controls_render.c
    menu/psp/landing_menu_render.c
    menu/psp/main_menu_render.c
    menu/psp/menu_builder_render.c
    menu/psp/menu_render.c
    menu/psp/new_game_menu_render.c
    menu/psp/options_menu_render.c
    menu/psp/savefile_list_render.c
    physics/psp/debug_renderer.c
    scene/psp/ball_render.c
    scene/psp/camera_render.c
    scene/psp/clock_digits.c
    scene/psp/debug_scene_render.c
    scene/psp/doorway_cover_render.c
    scene/psp/fizzler_particles.c
    scene/psp/hud_render.c
    scene/psp/laser_render.c
    scene/psp/pedestal_render.c
    scene/psp/portal_cover_render.c
    scene/psp/portal_gun_render.c
    scene/psp/portal_surface_gfx.c
    scene/psp/render_plan_execute.c
    scene/psp/scene_animator_transforms.c
    scene/psp/scene_render.c
    scene/psp/signage_render.c
    scene/psp/turret_render.c
    sk64/psp/skeletool_animation_data.c
    sk64/psp/skeletool_render.c

    system/psp/audio_me.c
    system/psp/audio_psp.c
    system/psp/psp_profile.c
    system/psp/boot_psp.c
    system/psp/cartridge_psp.c
    system/psp/controller_psp.c
    system/psp/display_psp.c
    system/psp/time_psp.c

    system/psp/ultra64/os_message_psp.c
    system/psp/ultra64/os_thread_psp.c
    system/psp/ultra64/os_time_psp.c

    levels/psp/level_load.c
    strings/psp/translations_load.c
    system/psp/main_psp.c
    player/psp/player_render.c
    util/psp/assert.c
    util/psp/dynamic_asset_loader.c
)

target_include_directories(engine INTERFACE
    # PSP only: makes <ultra64.h> resolve.
    "${CMAKE_CURRENT_SOURCE_DIR}/system/psp/ultra64"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/psp"
)

endif()

if (RSP_PROFILER)
    if (NOT DEBUGGER)
        message(FATAL_ERROR "Hardware debugging must be enabled to use the RSP profiler")
    endif()

    target_sources(engine INTERFACE
        system/libultra/rsp_profiler_libultra.c
    )
    target_compile_definitions(engine INTERFACE
        PORTAL64_WITH_RSP_PROFILER
    )
endif()

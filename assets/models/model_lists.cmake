# The set of models the game has, which is the same on either machine. The N64
# and PSP model pipelines both read this; only what they run per model differs.

# Models whose data is always loaded
set(STATIC_MODELS
    fleck_ash2
    grav_flare
    player/chell
    portal_gun/ball_trail
    portal_gun/v_portalgun
    portal_gun/w_portalgun
    portal/portal_blue
    portal/portal_blue_face
    portal/portal_blue_filled
    portal/portal_collider
    portal/portal_collider_vertical
    portal/portal_orange
    portal/portal_orange_face
    portal/portal_orange_filled
    props/round_elevator
    props/round_elevator_collision
    props/round_elevator_interior
    props/signage
    props_bts/glados_aperturedoor_collision
)

# Models whose data is loaded/unloaded as needed
set(DYNAMIC_MODELS
    cube/cube
    overlays/overlay_scrawlings002a
    overlays/overlay_scrawlings006a
    overlays/overlay_scrawlings006b
    overlays/overlay_scrawlings006c
    pedestal
    props/autoportal_frame/autoportal_frame
    props/box_dropper
    props/box_dropper_glass
    props/button
    props/combine_ball_catcher
    props/combine_ball_launcher
    props/cylinder_test
    props/door_01
    props/door_02
    props/food_can/food_can
    props/lab_chair
    props/lab_desk/lab_desk01
    props/lab_desk/lab_desk02
    props/lab_desk/lab_desk03
    props/lab_desk/lab_desk04
    props/lab_monitor
    props/light_rail_endcap
    props/milk_carton/milk_carton
    props/milk_carton/milk_carton_open
    props/pc_case_open/pc_case_open
    props/portal_cleanser
    props/radio
    props/saucepan/saucepan
    props/security_camera
    props/switch001
    props/turret_01
    props/turret_01_eye
    props/water_bottle/water_bottle
    props_bts/glados_aperturedoor
    props_junk/metalbucket01a
    signage/clock
    signage/clock_digits
)

# Models with predefined animations
set(KEYFRAMED_ANIMATED_MODELS
    pedestal
    player/chell
    portal_gun/v_portalgun
    props/box_dropper
    props/combine_ball_catcher
    props/combine_ball_launcher
    props/door_01
    props/door_02
    props/switch001
    props/turret_01
    props_bts/glados_aperturedoor
)

# All animated models, including those which are only procedurally animated
set(ANIMATED_MODELS
    ${KEYFRAMED_ANIMATED_MODELS}
    props/button
    props/security_camera
)

# All models
set(MODELS
    ${DYNAMIC_MODELS}
    ${STATIC_MODELS}
)

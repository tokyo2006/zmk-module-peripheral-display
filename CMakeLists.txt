# zmk-module-peripheral-display top-level CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)

zephyr_include_directories(include)

# All source files gated by ZMK_PERIPHERAL_DISPLAY
# (filled in by later tasks; left empty here so the module builds
#  even when the feature is disabled)
if(CONFIG_ZMK_PERIPHERAL_DISPLAY)
    # T3, T4
    target_sources(app PRIVATE src/peripheral_status.c)
    # T5
    target_sources(app PRIVATE src/peripheral_status_service.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)
    # T6
    target_sources(app PRIVATE src/peripheral_status_forward.c)
    target_sources(app PRIVATE src/peripheral_status_forward_debounce.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)
    # T7
    target_sources(app PRIVATE src/peripheral_status_receiver.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS)
    # T8
    target_sources(app PRIVATE src/peripheral_display.c)
    # T9 widgets
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_LAYER    app PRIVATE src/widgets/peripheral_layer_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_OUTPUT    app PRIVATE src/widgets/peripheral_output_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BATTERY   app PRIVATE src/widgets/peripheral_battery_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_MODIFIERS app PRIVATE src/widgets/peripheral_modifiers.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_HID_INDICATORS app PRIVATE src/widgets/peripheral_hid_indicators.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_WPM       app PRIVATE src/widgets/peripheral_wpm_status.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_CENTRAL_NAME app PRIVATE src/widgets/peripheral_central_name.c)
    target_sources_ifdef(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGET_BONGO_CAT app PRIVATE src/widgets/peripheral_bongo_cat.c src/widgets/peripheral_bongo_cat_images.c)
endif()

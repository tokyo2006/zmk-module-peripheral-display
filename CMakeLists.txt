# zmk-module-peripheral-display top-level CMakeLists.txt
cmake_minimum_required(VERSION 3.20.0)

zephyr_include_directories(include)

# All source files gated by ZMK_PERIPHERAL_DISPLAY
# (filled in by later tasks; left empty here so the module builds
#  even when the feature is disabled)
if(CONFIG_ZMK_PERIPHERAL_DISPLAY)
    # ASDC BLE L2CAP data channel (central -> peripheral), enabled only when a
    # zmk,peripheral-display-asdc node exists in the keyboard devicetree.
    if(CONFIG_ZMK_ARBITRARY_SPLIT_DATA_CHANNEL)
        target_sources(app PRIVATE src/asdc/asdc_core.c)
        if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
            target_sources(app PRIVATE src/asdc/asdc_ble_central.c)
        else()
            target_sources(app PRIVATE src/asdc/asdc_ble_peripheral.c)
        endif()
    endif()

    # shared status pack/unpack + shadow state
    target_sources(app PRIVATE src/peripheral_status.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_FORWARD)
    # central: subscribe ZMK events -> send status over ASDC
    target_sources(app PRIVATE src/peripheral_status_forward.c)
    target_sources(app PRIVATE src/peripheral_status_forward_debounce.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)
    # peripheral: receive relayed status event -> update shadow
    target_sources(app PRIVATE src/peripheral_status_receiver.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS)
    # T8
    target_sources(app PRIVATE src/peripheral_display.c)
    # shared 1-bit bitmap icons (modifiers + output status)
    target_sources(app PRIVATE src/widgets/peripheral_icons.c)
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

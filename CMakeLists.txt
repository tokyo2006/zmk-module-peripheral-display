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
endif()

if(CONFIG_ZMK_PERIPHERAL_STATUS_RECEIVE)
    # T7
    target_sources(app PRIVATE src/peripheral_status_receiver.c)
endif()

if(CONFIG_ZMK_PERIPHERAL_DISPLAY_WIDGETS)
    # T8
    target_sources(app PRIVATE src/peripheral_display.c)
    # T9 widgets added here
endif()

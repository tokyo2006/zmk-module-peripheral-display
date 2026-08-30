# Shield-local sources for peripheral_lcd_ls013
if(SHIELD_PERIPHERAL_LCD_LS013)
    target_sources(app PRIVATE src/custom_status_screen.c)
endif()

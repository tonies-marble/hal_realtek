# Copyright (c) 2024 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

zephyr_compile_options(-mcmse)

set(LIB_PATH ${ZEPHYR_CURRENT_MODULE_DIR}/zephyr/blobs/ameba/${CONFIG_SOC_SERIES}/lib)

if(CONFIG_WIFI_AMEBA AND NOT CONFIG_BUILD_ONLY_NO_BLOBS)
  zephyr_link_libraries(
    ${LIB_PATH}/lib_wifi_whc_ap.a
    ${LIB_PATH}/lib_wifi_api.a
    ${LIB_PATH}/lib_rtw_event.a
    ${LIB_PATH}/lib_wpa_lite.a
    ${LIB_PATH}/lib_wifi_com_sec.a
    ${LIB_PATH}/lib_coex_api.a
    ${LIB_PATH}/lib_wifi_rtk_app.a
  )
endif()
zephyr_include_directories(
  include
  source/cmsis
  source/misc
  source/swlib
  source/fwlib/include
  source/usrcfg/include
  source/app/monitor/include
)

# used by common
zephyr_library_sources(
  source/fwlib/ram_common/ameba_reset.c
  source/fwlib/ram_common/ameba_clk.c
  source/swlib/log.c
  source/swlib/log_zephyr.c
  source/fwlib/ram_common/ameba_adc.c
  source/fwlib/ram_common/ameba_loguart.c
  source/fwlib/ram_common/ameba_arch.c
  source/fwlib/ram_common/ameba_ipc_api.c
  source/fwlib/ram_common/ameba_ipc_ram.c
  source/fwlib/ram_common/ameba_clk_zephyr.c
)

# used by driver, need use zephyr_library_sources_ifdef
zephyr_library_sources_ifdef(CONFIG_GPIO_AMEBA source/fwlib/ram_common/ameba_gpio.c)
zephyr_library_sources_ifdef(CONFIG_INPUT_CTC_AMEBA source/fwlib/ram_common/ameba_captouch.c)
zephyr_library_sources_ifdef(CONFIG_I2S_AMEBA source/fwlib/ram_common/ameba_audio_clock.c)
zephyr_library_sources_ifdef(CONFIG_I2S_AMEBA source/fwlib/ram_common/ameba_pll.c)
zephyr_library_sources_ifdef(CONFIG_AUDIO_AMEBA_DMIC source/fwlib/ram_common/ameba_codec.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_AMEBA source/fwlib/ram_common/ameba_flash_ram.c)
zephyr_library_sources_ifdef(CONFIG_DMA_AMEBA source/fwlib/ram_common/ameba_gdma_ram.c)
zephyr_library_sources_ifdef(CONFIG_I2C_AMEBA source/fwlib/ram_common/ameba_i2c.c)
zephyr_library_sources_ifdef(CONFIG_LEDC_AMEBA source/fwlib/ram_common/ameba_ledc.c)
zephyr_library_sources_ifdef(CONFIG_RTC_AMEBA source/fwlib/ram_common/ameba_rtc.c)
zephyr_library_sources_ifdef(CONFIG_SPI_AMEBA source/fwlib/ram_common/ameba_spi.c)
zephyr_library_sources_ifdef(CONFIG_I2S_AMEBA source/fwlib/ram_common/ameba_sport.c)
if(CONFIG_COUNTER_TMR_AMEBA OR CONFIG_PWM_AMEBA)
  zephyr_library_sources(source/fwlib/ram_common/ameba_tim.c)
endif()
zephyr_library_sources_ifdef(CONFIG_UART_AMEBA source/fwlib/ram_common/ameba_uart.c)

zephyr_link_libraries(
    gcc
    # Zephyr on AmebaSmart runs as image2 (non-secure world). The upstream
    # SDK's image2 build links against the NS ROM-symbol map
    # (ameba_rom_symbol_acut.ld, aka c_LINK_ROM_SYMBOL_NS in
    # project_hp/CMakeLists.txt); the *_s map is only for image1/image3
    # (the secure bootloader). Using the S-side symbol table from
    # non-secure state causes SecureFault(INVEP) on the first ROM call
    # (e.g. SYSCFG_CHIPType_Get in soc_early_init_hook).
    -T${CMAKE_CURRENT_SOURCE_DIR}/ld/ameba_rom_symbol_acut.ld
    -T${CMAKE_CURRENT_SOURCE_DIR}/ld/ameba_rom_symbol_stdlib.ld
  )

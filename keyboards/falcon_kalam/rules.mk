# Keyboard-local linker script (ld/STM32F103x8_stm32duino_falcon_kalam.ld):
# identical to QMK's stm32duino layout except that the 4 KB wear-leveling
# EEPROM at the top of flash (stock: 2 KB) is excluded from the application
# region; see the ld header for the full list of differences.
MCU_LDSCRIPT = STM32F103x8_stm32duino_falcon_kalam

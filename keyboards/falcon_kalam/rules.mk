# Keyboard-local linker script (ld/STM32F103x8_stm32duino_falcon_kalam.ld):
# identical to QMK's stm32duino layout except that the 2 KB wear-leveling
# EEPROM at the top of flash is excluded from the application region.
MCU_LDSCRIPT = STM32F103x8_stm32duino_falcon_kalam

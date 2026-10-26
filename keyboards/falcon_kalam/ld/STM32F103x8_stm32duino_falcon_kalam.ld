/*
 * falcon_kalam -- GD32F103C8T6 (64 KB flash, 20 KB SRAM) behind the
 * STM32duino/Maple DFU bootloader.
 *
 * Derived from QMK's platforms/chibios/boards/STM32_F103_STM32DUINO/ld/
 * stm32duino_bootloader_common.ld (ChibiOS, Apache-2.0). The only change is
 * the length of flash0: the stock script gives the application everything up
 * to the end of the 64 KB part, which overlaps the wear-leveling EEPROM pages.
 * Here those pages are excluded so an oversized image fails at link time
 * instead of silently overlapping persistent storage.
 *
 *   0x08000000 - 0x08001FFF   8 KB   STM32duino/Maple bootloader (never written)
 *   0x08002000 - 0x0800EFFF  52 KB   application (this region)
 *   0x0800F000 - 0x0800FFFF   4 KB   wear-leveling EEPROM (WEAR_LEVELING_BACKING_SIZE)
 *
 * Stock used 2 KB at 0x0800F800 (1 KB of settings). This keyboard reserves
 * 4 KB (2 KB of settings) so the six HHKB mode layers fit next to VIA macros
 * (POLICY-HHKB, docs/policy.md).
 *
 * Keep FALCON_KALAM_EEPROM_BYTES in sync with WEAR_LEVELING_BACKING_SIZE and
 * WEAR_LEVELING_EFL_FLASH_SIZE in config.h / keyboard.json.
 */

falcon_kalam_flash_size    = 64k;
falcon_kalam_ram_size      = 20k;
falcon_kalam_boot_bytes    = 0x2000;
falcon_kalam_eeprom_bytes  = 4k;

MEMORY
{
    flash0  : org = 0x08002000, len = 64k - 0x2000 - 4k
    flash1  : org = 0x00000000, len = 0
    flash2  : org = 0x00000000, len = 0
    flash3  : org = 0x00000000, len = 0
    flash4  : org = 0x00000000, len = 0
    flash5  : org = 0x00000000, len = 0
    flash6  : org = 0x00000000, len = 0
    flash7  : org = 0x00000000, len = 0
    ram0    : org = 0x20000000, len = 20k
    ram1    : org = 0x00000000, len = 0
    ram2    : org = 0x00000000, len = 0
    ram3    : org = 0x00000000, len = 0
    ram4    : org = 0x00000000, len = 0
    ram5    : org = 0x00000000, len = 0
    ram6    : org = 0x00000000, len = 0
    ram7    : org = 0x00000000, len = 0
}

/* Exported for scripts/verify_firmware.py (read back from the ELF symbol table). */
__falcon_kalam_app_start__    = ORIGIN(flash0);
__falcon_kalam_app_limit__    = ORIGIN(flash0) + LENGTH(flash0);
__falcon_kalam_eeprom_start__ = 0x08000000 + falcon_kalam_flash_size - falcon_kalam_eeprom_bytes;

ASSERT(ORIGIN(flash0) == 0x08000000 + falcon_kalam_boot_bytes, "flash0 must start right after the bootloader")
ASSERT(__falcon_kalam_app_limit__ == __falcon_kalam_eeprom_start__, "flash0 must end where the EEPROM region begins")

/* For each data/text section two region are defined, a virtual region
   and a load region (_LMA suffix).*/

/* Flash region to be used for exception vectors.*/
REGION_ALIAS("VECTORS_FLASH", flash0);
REGION_ALIAS("VECTORS_FLASH_LMA", flash0);

/* Flash region to be used for constructors and destructors.*/
REGION_ALIAS("XTORS_FLASH", flash0);
REGION_ALIAS("XTORS_FLASH_LMA", flash0);

/* Flash region to be used for code text.*/
REGION_ALIAS("TEXT_FLASH", flash0);
REGION_ALIAS("TEXT_FLASH_LMA", flash0);

/* Flash region to be used for read only data.*/
REGION_ALIAS("RODATA_FLASH", flash0);
REGION_ALIAS("RODATA_FLASH_LMA", flash0);

/* Flash region to be used for various.*/
REGION_ALIAS("VARIOUS_FLASH", flash0);
REGION_ALIAS("VARIOUS_FLASH_LMA", flash0);

/* Flash region to be used for RAM(n) initialization data.*/
REGION_ALIAS("RAM_INIT_FLASH_LMA", flash0);

/* RAM region to be used for Main stack. This stack accommodates the processing
   of all exceptions and interrupts.*/
REGION_ALIAS("MAIN_STACK_RAM", ram0);

/* RAM region to be used for the process stack. This is the stack used by
   the main() function.*/
REGION_ALIAS("PROCESS_STACK_RAM", ram0);

/* RAM region to be used for data segment.*/
REGION_ALIAS("DATA_RAM", ram0);
REGION_ALIAS("DATA_RAM_LMA", flash0);

/* RAM region to be used for BSS segment.*/
REGION_ALIAS("BSS_RAM", ram0);

/* RAM region to be used for the default heap.*/
REGION_ALIAS("HEAP_RAM", ram0);

/* Generic rules inclusion.*/
INCLUDE rules.ld

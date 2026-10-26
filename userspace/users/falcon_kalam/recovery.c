// Copyright 2026 Leo Maxwell
// SPDX-License-Identifier: GPL-2.0-or-later
//
// POLICY-RECOVERY: a CPU fault resets the MCU into the bootloader instead of
// hanging (docs/policy.md). This unit has no power-on DFU window, so a hung
// image could otherwise only be recovered through the pads or SWD (R13).
//
// How it reaches DFU: QMK's STM32duino board code writes BKP_DR10 =
// RTC_BOOTLOADER_FLAG (0x424C) at every boot, and the bootloader stays in DFU
// when it finds that flag after a reset. The handlers write the flag again
// (in case something cleared it) and reset. They deliberately do nothing
// else: a fault inside a fault handler locks the Cortex-M3 up, which would
// be the very hang this avoids. So no LED update here (the LEDs keep their
// last colour).
//
// Not covered: chSysHalt() (its hook lives in the board's chconf.h, i.e. L1)
// and infinite loops without a fault (a watchdog is unsafe here: the IWDG
// survives the reset and the bootloader never feeds it, so it would cut DFU
// sessions short).

#include QMK_KEYBOARD_H
#include <stddef.h>
#include <hal.h>

// The handler below hard-codes these values in its instructions.
_Static_assert(BKP_BASE == 0x40006C00UL, "BKP base address");
_Static_assert(offsetof(BKP_TypeDef, DR10) == 0x28, "BKP_DR10 offset");
_Static_assert(RTC_BOOTLOADER_FLAG == 0x424C, "STM32duino stay-in-DFU flag");

// Stack-free on purpose: if the fault came from a broken stack, a push here
// would fault again and lock the CPU up. Immediates only (no literal pool):
//   BKP_DR10 (0x40006C28) = 0x424C, then SCB_AIRCR (0xE000ED0C) =
//   VECTKEY 0x05FA | SYSRESETREQ (bit 2), then wait for the reset.
__attribute__((naked, noreturn)) void falcon_kalam_fault_reset(void) {
    __asm__ volatile(
        "movw r0, #0x6C28   \n"
        "movt r0, #0x4000   \n"
        "movw r1, #0x424C   \n"
        "str  r1, [r0]      \n"
        "dsb                \n"
        "movw r0, #0xED0C   \n"
        "movt r0, #0xE000   \n"
        "movw r1, #0x0004   \n"
        "movt r1, #0x05FA   \n"
        "str  r1, [r0]      \n"
        "dsb                \n"
        "1: b 1b            \n");
}

// Override ChibiOS' weak vectors (vectors.S), which loop forever by default.
void HardFault_Handler(void) __attribute__((alias("falcon_kalam_fault_reset")));
void MemManage_Handler(void) __attribute__((alias("falcon_kalam_fault_reset")));
void BusFault_Handler(void) __attribute__((alias("falcon_kalam_fault_reset")));
void UsageFault_Handler(void) __attribute__((alias("falcon_kalam_fault_reset")));

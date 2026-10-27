# Dev variant (POLICY-VARIANTS): every debug feature that fits, for
# testing and diagnosis. Never the daily image. Shared policy: users/falcon_kalam/.
USER_NAME := falcon_kalam

FALCON_KALAM_FLASHREAD = yes           # POLICY-FLASHREAD: bootloader/option-byte/UID dump
KEYCODE_STRING_ENABLE = yes            # readable keycode names in console debug output
DEBUG_MATRIX_SCAN_RATE_ENABLE = yes    # prints the matrix scan rate while debug is on

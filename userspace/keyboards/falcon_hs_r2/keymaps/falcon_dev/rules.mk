# Dev variant (POLICY-VARIANTS): every debug feature that fits, for alpha
# testing and diagnosis. Never the daily image. Shared policy: users/falcon/.
USER_NAME := falcon

FALCON_FLASHREAD = yes                 # POLICY-FLASHREAD: bootloader/option-byte/UID dump
KEYCODE_STRING_ENABLE = yes            # readable keycode names in console debug output
DEBUG_MATRIX_SCAN_RATE_ENABLE = yes    # prints the matrix scan rate while debug is on

/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x1b000, LENGTH = 0x25000
  RAM (rwx) :  ORIGIN = 0x20002080, LENGTH = 0x5f80
  CFG_VAR (r) : ORIGIN = 0x3fc00, LENGTH = 0x04
}

SECTIONS
{
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM

  .cfgVer :
  {
    KEEP(*(.cfgVer))
  } > CFG_VAR
} INSERT AFTER .data;

INCLUDE "nrf5x_common.ld"
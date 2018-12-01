#include "cdef.h"

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

#define FW_CFG_SIGNATURE    0x0000
#define FW_CFG_ID           0x0001
#define FW_CFG_FILE_DIR     0x0019

typedef struct FWCfgFile {		/* an individual file entry, 64 bytes total */
      uint32 size;		/* size of referenced fw_cfg item, big-endian */
      uint16 select;		/* selector key of fw_cfg item, big-endian */
      uint16 reserved;
      uint8 name[56];		/* fw_cfg item name, NUL-terminated ascii */
} fwfile;

void
select_entity(uint32)
{
}


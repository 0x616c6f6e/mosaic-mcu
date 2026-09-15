#ifndef PLATFORM_FATFS_CONFIG_H
#define PLATFORM_FATFS_CONFIG_H

/** @brief FatFs configuration signature required by this upstream revision. */
#define FFCONF_DEF 80386

/** @brief Enable writable FatFs, formatting, and full file API support. */
#define FF_FS_READONLY       0
#define FF_FS_MINIMIZE       0
#define FF_USE_FIND          0
#define FF_USE_MKFS          1
#define FF_USE_FASTSEEK      0
#define FF_USE_EXPAND        0
#define FF_USE_CHMOD         0
#define FF_USE_LABEL         0
#define FF_USE_FORWARD       0
#define FF_USE_STRFUNC       0
#define FF_PRINT_LLI         0
#define FF_PRINT_FLOAT       0
#define FF_STRF_ENCODE       0

/** @brief FAT on-disk short-name code page (OEM US). */
#define FF_CODE_PAGE         437
/** @brief Enable long filenames with a maximum of 31 characters. */
#define FF_USE_LFN           1
#define FF_MAX_LFN           31
#define FF_LFN_UNICODE       0
#define FF_LFN_BUF           31
#define FF_SFN_BUF           12
#define FF_FS_RPATH          0
#define FF_PATH_DEPTH        10

/** @brief Bind exactly one external SPI NOR FAT volume. */
#define FF_VOLUMES           1
#define FF_STR_VOLUME_ID     0
/** @brief Optional textual volume name for the flash disk. */
#define FF_VOLUME_STRS       "FLASH"
#define FF_MULTI_PARTITION   0
/** @brief Fixed logical sector length for the SPI NOR disk in bytes. */
#define FF_MIN_SS            512
#define FF_MAX_SS            512
#define FF_LBA64             0
#define FF_MIN_GPT           0x10000000
#define FF_USE_TRIM          0

/** @brief Reuse the FatFs file object buffer to reduce RAM usage. */
#define FF_FS_TINY           1
#define FF_FS_EXFAT          0
/** @brief Use fallback FAT timestamps without a realtime clock. */
#define FF_FS_NORTC          1
#define FF_NORTC_MON         1
#define FF_NORTC_MDAY        1
/** @brief Fallback year for files created without an RTC. */
#define FF_NORTC_YEAR        2026
#define FF_FS_CRTIME         0
#define FF_FS_NOFSINFO       0
#define FF_FS_LOCK           0
/** @brief FatFs accesses are serialized by the calling firmware. */
#define FF_FS_REENTRANT      0
#define FF_FS_TIMEOUT        1000

#endif

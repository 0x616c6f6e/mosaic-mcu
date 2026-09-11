set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES WCH_TOOLCHAIN_ROOT)

get_filename_component(PLATFORM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(WCH_TOOLCHAIN_ROOT
    "${PLATFORM_ROOT}/tools/wch/toolchain/riscv-wch-elf-gcc-12.2.0"
    CACHE PATH "Root directory of the WCH RISC-V GCC12 toolchain"
)

if(NOT CMAKE_C_COMPILER)
    find_program(WCH_C_COMPILER
        NAMES riscv-wch-elf-gcc
        HINTS ${WCH_TOOLCHAIN_ROOT} ${WCH_TOOLCHAIN_ROOT}/bin
        REQUIRED
    )
    set(CMAKE_C_COMPILER ${WCH_C_COMPILER} CACHE FILEPATH "WCH C compiler")
endif()
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

if(NOT CMAKE_AR)
    find_program(WCH_AR NAMES riscv-wch-elf-ar
        HINTS ${WCH_TOOLCHAIN_ROOT} ${WCH_TOOLCHAIN_ROOT}/bin REQUIRED)
    set(CMAKE_AR ${WCH_AR} CACHE FILEPATH "WCH archiver")
endif()
if(NOT CMAKE_RANLIB)
    find_program(WCH_RANLIB NAMES riscv-wch-elf-ranlib
        HINTS ${WCH_TOOLCHAIN_ROOT} ${WCH_TOOLCHAIN_ROOT}/bin REQUIRED)
    set(CMAKE_RANLIB ${WCH_RANLIB} CACHE FILEPATH "WCH ranlib")
endif()
if(NOT CMAKE_OBJCOPY)
    find_program(WCH_OBJCOPY NAMES riscv-wch-elf-objcopy
        HINTS ${WCH_TOOLCHAIN_ROOT} ${WCH_TOOLCHAIN_ROOT}/bin REQUIRED)
    set(CMAKE_OBJCOPY ${WCH_OBJCOPY} CACHE FILEPATH "WCH objcopy")
endif()
if(NOT CMAKE_SIZE)
    find_program(WCH_SIZE NAMES riscv-wch-elf-size
        HINTS ${WCH_TOOLCHAIN_ROOT} ${WCH_TOOLCHAIN_ROOT}/bin REQUIRED)
    set(CMAKE_SIZE ${WCH_SIZE} CACHE FILEPATH "WCH size tool")
endif()

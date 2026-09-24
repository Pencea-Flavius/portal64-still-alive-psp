####################
## PSP Toolchain  ##
####################

if (NOT DEFINED ENV{PSPDEV})
    message(FATAL_ERROR "The PSPDEV environment variable must point at the pspdev installation")
endif()

# pspdev ships its own CMake toolchain. It sets the compilers, the include and
# link directories, PSP and PLATFORM_PSP, and pulls in create_pbp_file().
include("$ENV{PSPDEV}/psp/share/pspdev.cmake")

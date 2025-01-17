################################################################################
include(DownloadProject)

# Shortcut function
function(tetwild_download_project name)
    download_project(
        PROJ         ${name}
        SOURCE_DIR   ${TETWILD_EXTERNAL}/${name}
        DOWNLOAD_DIR ${TETWILD_EXTERNAL}/.cache/${name}
        ${ARGN}
    )
endfunction()

################################################################################

## libigl
function(tetwild_download_libigl)
    tetwild_download_project(libigl
        GIT_REPOSITORY https://github.com/libigl/libigl.git
        GIT_TAG        v2.5.0
    )
endfunction()

## geogram
function(tetwild_download_geogram)
    tetwild_download_project(geogram
        GIT_REPOSITORY https://github.com/Yixin-Hu/geogram/
        GIT_TAG        b613750341a6cdd31ae8df80ecfc26ac7ca1a6ad
    )
endfunction()

## fmt
function(tetwild_download_fmt)
    tetwild_download_project(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG        5.2.0
    )
endfunction()

## spdlog
function(tetwild_download_spdlog)
    tetwild_download_project(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.1.0
    )
endfunction()

## CLI11
function(tetwild_download_cli11)
    tetwild_download_project(cli11
        URL     https://github.com/CLIUtils/CLI11/archive/v1.6.1.tar.gz
        URL_MD5 48ef97262adb0b47a2f0a7edbda6e2aa
    )
endfunction()

## mmg
function(tetwild_download_mmg)
    tetwild_download_project(mmg
        GIT_REPOSITORY https://github.com/jdumas/mmg.git
        GIT_TAG        a9179cb77b32925011736bb3d862651b63cfaf1a
    )
endfunction()

## Sanitizers
function(tetwild_download_sanitizers)
    tetwild_download_project(sanitizers-cmake
        GIT_REPOSITORY https://github.com/arsenm/sanitizers-cmake.git
        GIT_TAG        6947cff3a9c9305eb9c16135dd81da3feb4bf87f
    )
endfunction()

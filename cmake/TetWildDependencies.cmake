################################################################################
# CMake download helpers
################################################################################

# download external dependencies
include(TetWildDownloadExternal)

################################################################################
# Required dependencies
################################################################################

# mmg
if(NOT TARGET mmg::mmg)
	tetwild_download_mmg()
	include(mmg)

	# option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
	# option(BUILD_TESTING "Enable/Disable continuous integration" OFF)
	# set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
	# set(OLD_BUILD_TESTING ${BUILD_TESTING} CACHE BOOL "" FORCE)
	# set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
	# set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
	# set(BUILD_MMG2D OFF CACHE BOOL "" FORCE)
	# set(BUILD_MMGS OFF CACHE BOOL "" FORCE)
	# set(BUILD_MMG3D OFF CACHE BOOL "" FORCE)
	# set(USE_SCOTCH OFF CACHE BOOL "" FORCE)
	# add_subdirectory(${TETWILD_EXTERNAL}/mmg)
	# set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
	# set(BUILD_TESTING ${OLD_BUILD_TESTING} CACHE BOOL "" FORCE)
	# add_library(mmg::mmg ALIAS libmmg_so)
	# add_library(mmg::mmgs ALIAS libmmgs_so)
	# add_library(mmg::mmg2d ALIAS libmmg2d_so)
	# add_library(mmg::mmg3d ALIAS libmmg3d_so)

	# foreach(target_name IN ITEMS libmmg_so libmmgs_so libmmg2d_so libmmg3d_so)
	# 	set_target_properties(${target_name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	# 	set_target_properties(${target_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	# 	set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
	# endforeach()

	# add_library(mmg_mmg INTERFACE)
	# add_library(mmg::mmg ALIAS mmg_mmg)

endif()
# geogram
if(NOT TARGET geogram)
	tetwild_download_geogram()
	include(geogram)
endif()

# Boost
if(TETWILD_WITH_HUNTER)
	hunter_add_package(Boost COMPONENTS thread system)
endif()

# fmt
if(NOT TARGET fmt::fmt)
	tetwild_download_fmt()
	add_subdirectory(${TETWILD_EXTERNAL}/fmt)
endif()

# spdlog
if(NOT TARGET spdlog::spdlog)
	tetwild_download_spdlog()
	add_library(spdlog INTERFACE)
	add_library(spdlog::spdlog ALIAS spdlog)
	target_include_directories(spdlog INTERFACE ${TETWILD_EXTERNAL}/spdlog/include)
	target_compile_definitions(spdlog INTERFACE -DSPDLOG_FMT_EXTERNAL)
	target_link_libraries(spdlog INTERFACE fmt::fmt)
endif()

# libigl
if(NOT TARGET igl::core)
	##tetwild_download_libigl()
	find_package(LIBIGL REQUIRED)
endif()

# pymesh loaders
add_subdirectory(${TETWILD_EXTERNAL}/pymesh)

# CL11
if(NOT TARGET CLI11::CLI11)
	tetwild_download_cli11()
	add_subdirectory(${TETWILD_EXTERNAL}/cli11)
endif()

#######################################################################
#
# Generic Compiz Fusion plugin cmake module
#
# Copyright : (C) 2008 by Dennis Kasprzyk
# E-mail    : onestone@opencompositing.org
#
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
#######################################################################
#
# This module provides the following macro:
#
# compiz_plugin (<plugin name>
#                       [PKGDEPS dep1 dep2 ...]
#                       [PLUGINDEPS plugin1 plugin2 ...]
#                       [LDFLAGSADD flag1 flag2 ...]
#                       [CFLAGSADD flag1 flag2 ...]
#                       [LIBRARIES lib1 lib2 ...]
#                       [LIBDIRS dir1 dir2 ...]
#                       [INCDIRS dir1 dir2 ...])
#
# PKGDEPS    = pkgconfig dependencies
# PLUGINDEPS = compiz plugin dependencies
# LDFLAGSADD = flags added to the link command
# CFLAGSADD  = flags added to the compile command
# LIBRARIES  = libraries added to link command
# LIBDIRS    = additional link directories
# INCDIRS    = additional include directories
#
# The following variables will be used by this macro:
#
# BUILD_GLOBAL=true Environment variable to install a plugin
#                   into the compiz directories
#
# COMPIZ_PLUGIN_INSTALL_TYPE = (package | compiz | local (default))
#     package = Install into ${CMAKE_INSTALL_PREFIX}
#     compiz  = Install into compiz prefix (BUILD_GLOBAL=true)
#     local   = Install into home directory
#
# COMPIZ_I18N_DIR      = Translation file directory
#
# COMPIZ_DISABLE_SCHEMAS_INSTALL  = Disables gconf schema installation with gconftool
# COMPIZ_INSTALL_GCONF_SCHEMA_DIR = Installation path of the gconf schema file
#
# VERSION = package version that is added to a plugin pkg-version file
#
#######################################################################

include (CompizCommon)
include (CompizBcop)

if (COMPIZ_GCONF_SCHEMAS_SUPPORT)
    include (CompizGconf)
endif ()

# determinate installation directories
macro (_prepare_directories)

    if ("${COMPIZ_PLUGIN_INSTALL_TYPE}" STREQUAL "package")
	set (PLUGIN_BUILDTYPE global)
	set (PLUGIN_PREFIX    ${CMAKE_INSTALL_PREFIX})
	set (PLUGIN_LIBDIR    ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/compiz)
	set (PLUGIN_INCDIR    ${CMAKE_INSTALL_PREFIX}/include)
	set (PLUGIN_PKGDIR    ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig)
	set (PLUGIN_XMLDIR    ${CMAKE_INSTALL_PREFIX}/share/compiz)
	set (PLUGIN_IMAGEDIR  ${CMAKE_INSTALL_PREFIX}/share/compiz)
	set (PLUGIN_DATADIR   ${CMAKE_INSTALL_PREFIX}/share/compiz)
	if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
            set (PLUGIN_SCHEMADIR "${CMAKE_INSTALL_PREFIX}/share/gconf/schemas")
        else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	    set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)

    elseif ("${COMPIZ_PLUGIN_INSTALL_TYPE}" STREQUAL "compiz" OR
	    "$ENV{BUILD_GLOBAL}" STREQUAL "true")
	set (PLUGIN_BUILDTYPE global)
	set (PLUGIN_PREFIX    ${COMPIZ_PREFIX})
	set (PLUGIN_LIBDIR    ${COMPIZ_LIBDIR}/compiz)
	set (PLUGIN_INCDIR    ${COMPIZ_INCLUDEDIR})
	set (PLUGIN_PKGDIR    ${COMPIZ_LIBDIR}/pkgconfig)
	set (PLUGIN_XMLDIR    ${COMPIZ_PREFIX}/share/compiz)
	set (PLUGIN_IMAGEDIR  ${COMPIZ_PREFIX}/share/compiz)
	set (PLUGIN_DATADIR   ${COMPIZ_PREFIX}/share/compiz)
	if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
            set (PLUGIN_SCHEMADIR "${COMPIZ_PREFIX}/share/gconf/schemas")
        else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	    set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	
	if (NOT "${CMAKE_BUILD_TYPE}")
	    compiz_set (CMAKE_BUILD_TYPE debug)
	endif (NOT "${CMAKE_BUILD_TYPE}")	
    else ("${COMPIZ_PLUGIN_INSTALL_TYPE}" STREQUAL "compiz" OR
	  "$ENV{BUILD_GLOBAL}" STREQUAL "true")
	set (PLUGIN_BUILDTYPE local)
	set (PLUGIN_PREFIX    $ENV{HOME}/.compiz)
	set (PLUGIN_LIBDIR    $ENV{HOME}/.compiz/plugins)
	set (PLUGIN_XMLDIR    $ENV{HOME}/.compiz/metadata)
	set (PLUGIN_IMAGEDIR  $ENV{HOME}/.compiz/images)
	set (PLUGIN_DATADIR   $ENV{HOME}/.compiz/data)

	if (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
            set (PLUGIN_SCHEMADIR "$ENV{HOME}/.gconf/schemas")
        else (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	    set (PLUGIN_SCHEMADIR "${COMPIZ_INSTALL_GCONF_SCHEMA_DIR}")
	endif (NOT COMPIZ_INSTALL_GCONF_SCHEMA_DIR)
	
	if (NOT "${CMAKE_BUILD_TYPE}")
	    compiz_set (CMAKE_BUILD_TYPE debug)
	endif (NOT "${CMAKE_BUILD_TYPE}")
    endif ("${COMPIZ_PLUGIN_INSTALL_TYPE}" STREQUAL "package")
endmacro (_prepare_directories)

# parse plugin macro parameter
macro (_get_plugin_parameters _prefix)
    set (_current_var _foo)
    set (_supported_var PKGDEPS PLUGINDEPS LDFLAGSADD CFLAGSADD LIBRARIES LIBDIRS INCDIRS)
    foreach (_val ${_supported_var})
	set (${_prefix}_${_val})
    endforeach (_val)
    foreach (_val ${ARGN})
	set (_found FALSE)
	foreach (_find ${_supported_var})
	    if ("${_find}" STREQUAL "${_val}")
		set (_found TRUE)
	    endif ("${_find}" STREQUAL "${_val}")
	endforeach (_find)
	
	if (_found)
	    set (_current_var ${_prefix}_${_val})
	else (_found)
	    list (APPEND ${_current_var} ${_val})
	endif (_found)
    endforeach (_val)
endmacro (_get_plugin_parameters)

# check pkgconfig dependencies
macro (_check_plugin_pkg_deps _prefix)
    set (${_prefix}_HAS_PKG_DEPS TRUE)
    foreach (_val ${ARGN})
        string (REGEX REPLACE "[<>=\\.]" "_" _name ${_val})
	string (TOUPPER ${_name} _name)

	compiz_pkg_check_modules (_${_name} ${_val})

	if (_${_name}_FOUND)
	    list (APPEND ${_prefix}_PKG_LIBDIRS "${_${_name}_LIBRARY_DIRS}")
	    list (APPEND ${_prefix}_PKG_LIBRARIES "${_${_name}_LIBRARIES}")
	    list (APPEND ${_prefix}_PKG_INCDIRS "${_${_name}_INCLUDE_DIRS}")
	else ()
	    set (${_prefix}_HAS_PKG_DEPS FALSE)
	    compiz_set (COMPIZ_${_prefix}_MISSING_DEPS "${COMPIZ_${_prefix}_MISSING_DEPS} ${_val}")
	    set(__pkg_config_checked__${_name} 0 CACHE INTERNAL "" FORCE)
	endif ()
    endforeach ()
endmacro ()

# check plugin dependencies
macro (_check_plugin_plugin_deps _prefix)
    set (${_prefix}_HAS_PLUGIN_DEPS TRUE)
    foreach (_val ${ARGN})
	string (TOUPPER ${_val} _name)

	find_file (
	    _plugin_dep_${_val}
	    compiz-${_val}.pc.in
	    PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../${_val}
	    NO_DEFAULT_PATH
	)
	
	if (_plugin_dep_${_val})
	    file (RELATIVE_PATH _relative ${CMAKE_CURRENT_SOURCE_DIR} ${_plugin_dep_${_val}})
	    get_filename_component (_plugin_inc_dir ${_relative} PATH)
	    list (APPEND ${_prefix}_LOCAL_INCDIRS "${_plugin_inc_dir}/include")
	    list (APPEND ${_prefix}_LOCAL_LIBRARIES ${_val})
	else ()
	    # fallback to pkgconfig
	    compiz_pkg_check_modules (_${_name} compiz-${_val})
	    if (_${_name}_FOUND)
		list (APPEND ${_prefix}_PKG_LIBDIRS "${_${_name}_LIBRARY_DIRS}")
		list (APPEND ${_prefix}_PKG_LIBRARIES "${_${_name}_LIBRARIES}")
	        list (APPEND ${_prefix}_PKG_INCDIRS "${_${_name}_INCLUDE_DIRS}")
	    else ()
		set (${_prefix}_HAS_PLUGIN_DEPS FALSE)
		_COMPIZ_set (COMPIZ_${_prefix}_MISSING_DEPS "${COMPIZ_${_prefix}_MISSING_DEPS} compiz-${_val}")
	    endif ()
	endif ()

	compiz_set (_plugin_dep_${_val} "${_plugin_dep_${_val}}")

    endforeach ()
endmacro ()




# main function
function (_build_compiz_plugin plugin)
    string (TOUPPER ${plugin} _PLUGIN)

    if (COMPIZ_PLUGIN_INSTALL_TYPE)
	set (
	    COMPIZ_PLUGIN_INSTALL_TYPE ${COMPIZ_PLUGIN_INSTALL_TYPE} CACHE STRING
	    "Where a plugin should be installed \(package | compiz | local\)"
	)
    else (COMPIZ_PLUGIN_INSTALL_TYPE)
	set (
	    COMPIZ_PLUGIN_INSTALL_TYPE "local" CACHE STRING
	    "Where a plugin should be installed \(package | compiz | local\)"
	)
    endif (COMPIZ_PLUGIN_INSTALL_TYPE)

    _get_plugin_parameters (${_PLUGIN} ${ARGN})
    _prepare_directories ()

    find_file (
	_${plugin}_xml_in ${plugin}.xml.in
	PATHS ${CMAKE_CURRENT_SOURCE_DIR}
	NO_DEFAULT_PATH
    )
    if (_${plugin}_xml_in)
	set (_${plugin}_xml ${_${plugin}_xml_in})
    else ()
	find_file (
	    _${plugin}_xml ${plugin}.xml
	    PATHS ${CMAKE_CURRENT_SOURCE_DIR} }
	    NO_DEFAULT_PATH
        )
    endif ()

    set (${_PLUGIN}_HAS_PKG_DEPS)
    set (${_PLUGIN}_HAS_PLUGIN_DEPS)

    # check dependencies
    compiz_unset (COMPIZ_${_PLUGIN}_MISSING_DEPS)
    _check_plugin_plugin_deps (${_PLUGIN} ${${_PLUGIN}_PLUGINDEPS})
    _check_plugin_pkg_deps (${_PLUGIN} ${${_PLUGIN}_PKGDEPS})

    if (${_PLUGIN}_HAS_PKG_DEPS AND ${_PLUGIN}_HAS_PLUGIN_DEPS)

	compiz_set (COMPIZ_${_PLUGIN}_BUILD TRUE PARENT_SCOPE)

	if (NOT EXISTS ${CMAKE_BINARY_DIR}/generated)
	    file (MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
	endif (NOT EXISTS ${CMAKE_BINARY_DIR}/generated)
	
	if (_${plugin}_xml_in)
	    # translate xml
	    compiz_translate_xml ( ${_${plugin}_xml_in} "${CMAKE_BINARY_DIR}/generated/${plugin}.xml")
	    set (_translation_sources "${CMAKE_BINARY_DIR}/generated/${plugin}.xml")
	    set (_translated_xml ${CMAKE_BINARY_DIR}/generated/${plugin}.xml)
	else ()
	    if (_${plugin}_xml)
		set (_translated_xml ${_${plugin}_xml})
	    endif ()
	endif ()
	
	if (_${plugin}_xml)
	    # do we need bcop for our plugin
	    compiz_plugin_needs_bcop (${_${plugin}_xml} _needs_bcop)
	    if (_needs_bcop)
		# initialize everything we need for bcop
		compiz_add_bcop_targets (${plugin} ${_${plugin}_xml} _bcop_sources)
	    endif ()
	endif ()
	
	if (_translated_xml)
	    if (COMPIZ_GCONF_SCHEMAS_SUPPORT)
	        # generate gconf schema
	        compiz_gconf_schema (${_translated_xml} "${CMAKE_BINARY_DIR}/generated/compiz-${plugin}.schemas" ${PLUGIN_SCHEMADIR})
	        set (_schema_sources "${CMAKE_BINARY_DIR}/generated/compiz-${plugin}.schemas")
	    endif ()

	    # install xml
	    install (
		FILES ${_translated_xml}
		DESTINATION ${PLUGIN_XMLDIR}
	    )
	endif (_translated_xml)
	
	find_file (
	    _${plugin}_pkg compiz-${plugin}.pc.in
	    PATHS ${CMAKE_CURRENT_SOURCE_DIR}
	    NO_DEFAULT_PATH
	)

	# generate pkgconfig file and install it and the plugin header file
	if (_${plugin}_pkg AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/${plugin})
	    if ("${PLUGIN_BUILDTYPE}" STREQUAL "local")
		message (STATUS "[WARNING] The plugin ${plugin} might be needed by other plugins. Install it systemwide.")
	    else ()
		set (prefix ${PLUGIN_PREFIX})
		set (libdir ${PLUGIN_LIBDIR})
		set (includedir ${PLUGIN_INCDIR})
		if (NOT VERSION)
		    set (VERSION 0.0.1-git)
		endif (NOT VERSION)

		compiz_configure_file (
		    ${_${plugin}_pkg}
		    ${CMAKE_BINARY_DIR}/generated/compiz-${plugin}.pc
		    COMPIZ_REQUIRES
		    COMPIZ_CFLAGS
		)
		
		install (
		    FILES ${CMAKE_BINARY_DIR}/generated/compiz-${plugin}.pc
		    DESTINATION ${PLUGIN_PKGDIR}
		)
		install (
		    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/${plugin}
		    DESTINATION ${PLUGIN_INCDIR}/compiz
		)
	    endif ()
	endif ()

	# install plugin data files
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
	    install (
		DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data
		DESTINATION ${PLUGIN_DATADIR}
	    )
	endif ()

	# install plugin image files
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/images)
	    install (
		DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/images
		DESTINATION ${PLUGIN_IMAGEDIR}
	    )
	endif ()
	
	# find files for build
	file (GLOB _h_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
	file (GLOB _h_ins_files "${CMAKE_CURRENT_SOURCE_DIR}/include/${plugin}/*.h")
	file (GLOB _cpp_files "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

#	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs")

#	set (_cflags "-Wall -Wpointer-arith  -fno-strict-aliasing")
	

	add_definitions (-DPREFIX='\"${PLUGIN_PREFIX}\"'
			 -DIMAGEDIR='\"${PLUGIN_IMAGEDIR}\"'
			 -DDATADIR='\"${PLUGIN_DATADIR}\"')

	include_directories (
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_BINARY_DIR}/generated
            ${${_PLUGIN}_LOCAL_INCDIRS}
            ${${_PLUGIN}_PKG_INCDIRS}
            ${${_PLUGIN}_INCDIRS}
            ${COMPIZ_INCLUDE_DIRS}
	)

	link_directories (
            ${COMPIZ_LINK_DIRS}
            ${${_PLUGIN}_PKG_LIBDIRS}
            ${${_PLUGIN}_LIBDIRS}
            ${PLUGIN_LIBDIR}
            ${COMPIZ_LIBDIR}/compiz
	)

	add_library (
            ${plugin} SHARED ${_cpp_files}
			     ${_h_files}
			     ${_h_ins_files}
			     ${_bcop_sources}
			     ${_translation_sources}
			     ${_schema_sources}
	)

	set_target_properties (
	    ${plugin} PROPERTIES
	    INSTALL_RPATH_USE_LINK_PATH 1
            BUILD_WITH_INSTALL_RPATH 1
            CMAKE_SKIP_BUILD_RPATH 0
	    COMPILE_FLAGS "${${_PLUGIN}_CFLAGSADD}"
	    LINK_FLAGS "${${_PLUGIN}_LDFLAGSADD}"
	)

	target_link_libraries (
	    ${plugin} ${COMPIZ_LIBRARIES}
		      ${${_PLUGIN}_LOCAL_LIBRARIES}
		      ${${_PLUGIN}_PKG_LIBRARIES}
		      ${${_PLUGIN}_LIBRARIES}
	)

	install (
	    TARGETS ${plugin}
	    LIBRARY DESTINATION ${PLUGIN_LIBDIR}
	)

	compiz_add_uninstall ()

    else ()
	message (STATUS "[WARNING] One or more dependencies for compiz plugin ${plugin} not found. Skipping plugin.")
	message (STATUS "Missing dependencies :${COMPIZ_${_PLUGIN}_MISSING_DEPS}")
	_COMPIZ_set (COMPIZ_${_PLUGIN}_BUILD FALSE)
    endif ()
endfunction ()

macro (compiz_plugin plugin)
    string (TOUPPER ${plugin} _PLUGIN)

    option (
	COMPIZ_DISABLE_PLUGIN_${_PLUGIN}
	"Disable building of plugin \"${plugin}\""
	OFF
    )

    if (NOT COMPIZ_DISABLE_PLUGIN_${_PLUGIN})
	_build_compiz_plugin (${plugin} ${ARGN})
    endif (NOT COMPIZ_DISABLE_PLUGIN_${_PLUGIN})
endmacro ()

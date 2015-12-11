include (FindPkgConfig)

add_subdirectory (gsettings)

set (_deps
     glib-2.0>=2.20.0
     gio-2.0>=2.25.0)

compiz_pkg_check_modules (COMPIZCONFIG_MATE_INTEGRATION ${_deps})

if (COMPIZCONFIG_MATE_INTEGRATION_FOUND)

    include_directories (${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include
			 ${CMAKE_SOURCE_DIR}/compizconfig/integration/mate/include
			 ${COMPIZCONFIG_MATE_INTEGRATION_INCLUDE_DIRS})

    link_directories (${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig
		      ${COMPIZCONFIG_MATE_INTEGRATION_LIBRARY_DIRS})

    add_library (compizconfig_mate_integration_constants STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integration_constants.c)

    add_library (compizconfig_mate_integration STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integration.c)

    target_link_libraries (compizconfig_mate_integration
			   compizconfig_mate_integration_constants
			   compizconfig_mate_integrated_setting
			   compizconfig
			   ${COMPIZCONFIG_MATE_INTEGRATION_LIBRARIES})

    add_library (compizconfig_mate_integrated_setting STATIC
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integrated_setting.c)

    target_link_libraries (compizconfig_mate_integrated_setting
			   compizconfig_mate_integration_constants
			   compizconfig
			${COMPIZCONFIG_MATE_INTEGRATION_LIBRARIES})

    if (COMPIZ_BUILD_TESTING)
	add_subdirectory (tests)
    endif (COMPIZ_BUILD_TESTING)

endif (COMPIZCONFIG_MATE_INTEGRATION_FOUND)

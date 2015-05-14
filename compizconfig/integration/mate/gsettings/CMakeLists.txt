include (FindPkgConfig)

if (USE_GSETTINGS)

    set (_deps
	glib-2.0>=2.20.0
	gio-2.0>=2.25.0)

    compiz_pkg_check_modules (COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION ${_deps})

    if (COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_FOUND)

	add_definitions (-DUSE_GCONF)

	include_directories (${CMAKE_SOURCE_DIR}/compizconfig/libcompizconfig/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/gsettings/gsettings_backend_shared
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/mate/include
			     ${CMAKE_SOURCE_DIR}/compizconfig/integration/mate/gsettings/include
			     ${COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_INCLUDE_DIRS})

	link_directories (${CMAKE_BINARY_DIR}/compizconfig/libcompizconfig
			  ${CMAKE_BINARY_DIR}/compizconfig/gsettings/gsettings_backend_shared
			  ${CMAKE_BINARY_DIR}/compizconfig/integration/mate
			  ${COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_LIBRARY_DIRS})

	add_library (compizconfig_mate_integration_gsettings_integrated_setting STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integration_gsettings_integrated_setting.c)

	target_link_libraries (compizconfig_mate_integration_gsettings_integrated_setting
			       compizconfig_mate_integrated_setting
			       compizconfig_mate_integration_constants
			       compizconfig
                               compizconfig_gsettings_util
			       ${COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_LIBRARIES})

	add_library (compizconfig_mate_integration_gsettings_setting_factory STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integration_gsettings_integrated_setting_factory.c)

	target_link_libraries (compizconfig_mate_integration_gsettings_setting_factory
			       compizconfig_gsettings_wrapper_factory_interface
			       compizconfig_mate_integration_constants
			       compizconfig_mate_integrated_setting
			       compizconfig_mate_integration_gsettings_integrated_setting
			       compizconfig
                               compizconfig_gsettings_util
			       ${COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_LIBRARIES})

	add_library (compizconfig_mate_integration_gsettings_wrapper_factory STATIC
		     ${CMAKE_CURRENT_SOURCE_DIR}/src/ccs_mate_integration_gsettings_wrapper_factory.c)

	target_link_libraries (compizconfig_mate_integration_gsettings_setting_factory
			       compizconfig_gsettings_wrapper_factory_interface
			       compizconfig
			       ${COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_LIBRARIES})

	if (COMPIZ_BUILD_TESTING)
	    add_subdirectory (tests)
	endif (COMPIZ_BUILD_TESTING)

    endif (COMPIZCONFIG_MATE_GSETTINGS_INTEGRATION_FOUND)

endif (USE_GSETTINGS)

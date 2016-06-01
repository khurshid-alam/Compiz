include (CompizGSettings)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}
		     ${CMAKE_CURRENT_SOURCE_DIR}/../
                     ${CMAKE_CURRENT_BINARY_DIR}
                     ${CMAKE_SOURCE_DIR}/tests/shared
                     ${CMAKE_SOURCE_DIR}/tests/shared/glib)

pkg_check_modules (COMPIZ_TEST_GTK_WINDOW_DECORATOR
		   glib-2.0>=2.28
		   gio-2.0>=2.25.0)

if (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)

    link_directories (${CMAKE_CURRENT_BINARY_DIR}
		      ${CMAKE_CURRENT_BINARY_DIR}/..)

    add_executable (compiz_test_gwd_settings
		    ${CMAKE_CURRENT_SOURCE_DIR}/test_gwd_settings.cpp)

    set (COMPIZ_TEST_GWD_SETTINGS_ADDITIONAL_LIBRARIES
	 gtk_window_decorator_settings)

    set (COMPIZ_TEST_GWD_SETTINGS_COVERAGE_TARGETS
	 gtk_window_decorator_settings)

    set (COMPIZ_TEST_GWD_SETTINGS_ADDITIONAL_LIBRARIES
         ${COMPIZ_TEST_GWD_SETTINGS_ADDITIONAL_LIBRARIES}
         gtk_window_decorator_settings_storage_gsettings)

    set (COMPIZ_TEST_GWD_SETTINGS_COVERAGE_TARGETS
         ${COMPIZ_TEST_GWD_SETTINGS_COVERAGE_TARGETS}
         gtk_window_decorator_settings_storage_gsettings)

    target_link_libraries (compiz_test_gwd_settings
			   ${COMPIZ_TEST_GWD_SETTINGS_ADDITIONAL_LIBRARIES}
			   decoration
			   ${COMPIZ_TEST_GTK_WINDOW_DECORATOR_LIBRARIES}
			   ${GTEST_BOTH_LIBRARIES}
			   ${GMOCK_LIBRARY}
		       	   ${GMOCK_MAIN_LIBRARY})

    compiz_discover_tests (compiz_test_gwd_settings
			   COVERAGE
			   ${COMPIZ_TEST_GWD_SETTINGS_COVERAGE_TARGETS})

endif (COMPIZ_TEST_GTK_WINDOW_DECORATOR_FOUND)

function (_print_configure_results)
    compiz_print_configure_header ("Compiz")
    compiz_color_message ("\n${_escape}[4mOptional features:${_escape}[0m\n")

    compiz_print_result_message ("GLESv2" USE_GLES)
    compiz_print_result_message ("gtk window decorator" USE_GTK)
    compiz_print_result_message ("metacity theme support" USE_METACITY)
    compiz_print_result_message ("gnome" USE_GNOME)
    compiz_print_result_message ("kde4 window decorator" USE_KDE4)

    compiz_print_result_message ("protocol buffers" USE_PROTOBUF)
    compiz_print_result_message ("file system change notifications" HAVE_INOTIFY)

    compiz_print_configure_footer ()
    compiz_print_plugin_stats ("${CMAKE_SOURCE_DIR}/plugins")
    compiz_print_configure_footer ()
endfunction ()

function (_check_compiz_cmake_macro)
    install (FILES
	     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindCompiz.cmake
	     ${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindOpenGLES2.cmake
	     DESTINATION
	     ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/)
    add_custom_target (findcompiz_install
	${CMAKE_COMMAND} -E make_directory ${CMAKE_ROOT}/Modules &&
	${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/FindCompiz.cmake ${CMAKE_ROOT}/Modules &&
	${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/cmake/FindOpenGLES2.cmake ${CMAKE_ROOT}/Modules
    )

    install (FILES
	     ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig/libcompizconfig/cmake/FindCompizConfig.cmake
	     DESTINATION
	     ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}/)
    add_custom_target (
	findcompizconfig_install
	${CMAKE_COMMAND} -E make_directory ${CMAKE_ROOT}/Modules &&
	${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/compizconfig/libcompizconfig/cmake/FindCompizConfig.cmake ${CMAKE_ROOT}/Modules
    )
endfunction ()

# add install prefix to pkgconfig search path if needed
string (REGEX REPLACE "([\\+\\(\\)\\^\\\$\\.\\-\\*\\?\\|])" "\\\\\\1" PKGCONFIG_REGEX ${CMAKE_INSTALL_PREFIX})
set (PKGCONFIG_REGEX ".*${PKGCONFIG_REGEX}/lib/pkgconfig:${PKGCONFIG_REGEX}/share/pkgconfig.*")

if (NOT "$ENV{PKG_CONFIG_PATH}" MATCHES "${PKGCONFIG_REGEX}")
    if ("" STREQUAL "$ENV{PKG_CONFIG_PATH}")
	set (ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig")
    else ()
	set (ENV{PKG_CONFIG_PATH}
	    "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig:${CMAKE_INSTALL_PREFIX}/share/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif ()
endif ()

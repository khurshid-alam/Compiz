set (_files
    bcop.xslt
)

compiz_configure_file (compiz_gsettings_schemas.xslt.in compiz_gsettings_schemas.xslt)

list (APPEND _files ${CMAKE_CURRENT_BINARY_DIR}/compiz_gsettings_schemas.xslt)

install (
    FILES ${_files}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/compiz/xslt
)

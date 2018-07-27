find_package (Compiz REQUIRED)

include (CompizPlugin)

include_directories (${COMPIZ_XORG_GTEST_COMMUNICATOR_INCLUDE_DIR})

set (COMPIZ_PLUGIN_PACK_BUILD 1)
compiz_plugin (testhelper NOINSTALL LIBRARIES ${COMPIZ_XORG_GTEST_COMMUNICATOR_LIBRARY})

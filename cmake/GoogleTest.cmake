set (GOOGLETEST_DIR "/usr/src/googletest" CACHE PATH "Path to Google Test")

if (EXISTS "${GOOGLETEST_DIR}")

    find_package (Threads REQUIRED)

    set (GMOCK_SRC_DIR "${GOOGLETEST_DIR}/googlemock" CACHE PATH "Path to Google Mock Sources")
    set (GMOCK_INCLUDE_DIR "${GMOCK_SRC_DIR}/include" CACHE PATH "Path to Google Mock Headers")
    set (GTEST_INCLUDE_DIR "${GOOGLETEST_DIR}/googletest/include" CACHE PATH "Path to Google Test Headers")
    set (GMOCK_BIN_DIR "${CMAKE_BINARY_DIR}/gmock")

    include (ExternalProject)
    ExternalProject_Add (GMock
                         SOURCE_DIR "${GMOCK_SRC_DIR}"
                         BINARY_DIR "${GMOCK_BIN_DIR}"
                         STAMP_DIR "${GMOCK_BIN_DIR}/stamp"
                         TMP_DIR "${GMOCK_BIN_DIR}/tmp"
                         INSTALL_COMMAND "")

    add_library (gtest INTERFACE)
    target_include_directories (gtest INTERFACE ${GTEST_INCLUDE_DIR})
    target_link_libraries (gtest INTERFACE "${GMOCK_BIN_DIR}/gtest/libgtest.a" ${CMAKE_THREAD_LIBS_INIT})
    add_dependencies (gtest GMock)

    add_library (gtest_main INTERFACE)
    target_include_directories (gtest_main INTERFACE ${GTEST_INCLUDE_DIR})
    target_link_libraries (gtest_main INTERFACE "${GMOCK_BIN_DIR}/gtest/libgtest_main.a" gtest)

    add_library (gmock INTERFACE)
    target_include_directories (gmock INTERFACE ${GMOCK_INCLUDE_DIR})
    target_link_libraries (gmock INTERFACE "${GMOCK_BIN_DIR}/libgmock.a" gtest)

    add_library (gmock_main INTERFACE)
    target_include_directories (gmock_main INTERFACE ${GMOCK_INCLUDE_DIR})
    target_link_libraries (gmock_main INTERFACE "${GMOCK_BIN_DIR}/libgmock_main.a" gmock)

    set (GMOCK_LIBRARY gmock)
    set (GMOCK_MAIN_LIBRARY gmock_main)
    set (GTEST_BOTH_LIBRARIES gtest gtest_main)

    add_definitions (-DGTEST_USE_OWN_TR1_TUPLE=0)
    include_directories (${GMOCK_INCLUDE_DIR}
                         ${GTEST_INCLUDE_DIR})

    set (GTEST_FOUND TRUE)
    set (GOOGLE_TEST_AND_MOCK_FOUND TRUE)

else ()

    message ("Google Test not found - cannot build tests!")

    set (GTEST_FOUND FALSE)
    set (GOOGLE_TEST_AND_MOCK_FOUND FALSE)

endif ()

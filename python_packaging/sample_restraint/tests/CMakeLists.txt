if(DOWNLOAD_GOOGLETEST)
    # Download and unpack googletest at configure time
    configure_file(CMakeLists.gtest.in googletest-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                    RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if(result)
        message(WARNING "CMake step for googletest failed: ${result}")
        #    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
                    RESULT_VARIABLE result
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
    if(result)
        message(WARNING "Build step for googletest failed: ${result}")
        #    message(FATAL_ERROR "Build step for googletest failed: ${result}")
    endif()

    # Add googletest directly to our build. This defines
    # the gtest and gtest_main targets.
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                     ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                     EXCLUDE_FROM_ALL)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GTest::Main ALIAS gtest_main)
else()
    if(GMXAPI_EXTENSION_MASTER_PROJECT)
        find_package(GTest REQUIRED)
    else() # building as part of a GROMACS build...
        if(TARGET gtest)
            add_library(GTest::GTest ALIAS gtest)
            add_library(GTest::Main ALIAS gtest_main)
        else()
            find_package(GTest)
        endif()
        if(NOT TARGET GTest::GTest)
            message(FATAL_ERROR "GTest::GTest target should have been supplied by the parent project.")
        endif()
        if(NOT TARGET GTest::Main)
            message(FATAL_ERROR "GTest::Main target should have been supplied by the parent project.")
        endif()
    endif()
endif() # DOWNLOAD_GOOGLETEST

#
# Set up our tests
#


# Copy test files
if (GMXAPI_EXTENSION_MASTER_PROJECT)
    # TODO: Deduplicate and leverage CMake dependencies.
    # We also build a test file in the spc_water_box pytest test fixture, but we can
    # presumably extract both of those from the GROMACS installation, or at least
    # through the gmxapi Python package resources.
    # Ref: https://gitlab.com/gromacs/gromacs/-/issues/2961
    file(DOWNLOAD
         https://github.com/kassonlab/sample_restraint/raw/master/tests/data/topol.tpr
         ${CMAKE_CURRENT_BINARY_DIR}/topol.tpr
         STATUS _download_status
         LOG _download_log)
    list(GET _download_status 0 _status)
    if(${_status} GREATER 0)
        message(WARNING "Could not download test data: ${_download_log}")
    endif()
    unset(_status)
    add_custom_target(gmxapi_extension_spc2_water_box
                      COMMAND cmake -E echo
                      SOURCES ${CMAKE_CURRENT_BINARY_DIR}/topol.tpr
                      )
else()
    # We are in the GROMACS build tree.
    # We just need a simple TPR input file. The 6 atom spc water box will suffice.
    if (NOT TARGET gmx)
        message(FATAL_ERROR "Trying to use gmx wrapper binary, but gmx target not defined.")
    endif()
    set(_mdp ${CMAKE_CURRENT_BINARY_DIR}/grompp.mdp)
    file(WRITE ${_mdp} "integrator = md\n")
    file(APPEND ${_mdp} "nsteps = 6")
    set(_gro ${CMAKE_SOURCE_DIR}/src/testutils/simulationdatabase/spc2.gro)
    set(_top ${CMAKE_SOURCE_DIR}/src/testutils/simulationdatabase/spc2.top)
    add_custom_target(gmxapi_extension_spc2_water_box
                      BYPRODUCTS topol.tpr
                      COMMAND gmx -quiet grompp -f ${_mdp} -c ${_gro} -p ${_top}
                      DEPENDS gmx ${_mdp}
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating input file for sample_restraint tests.")
    unset(_mdp)
    unset(_gro)
    unset(_top)
endif ()

# Note: Expects topol.tpr to be in CURRENT_BINARY_DUR
configure_file(testingconfiguration.in.h testingconfiguration.h)

# Test that the library can access its dependencies and build.
add_executable(gmxapi_extension_library-test test_binding.cpp)
add_dependencies(gmxapi_extension_library-test gmxapi_extension_spc2_water_box)
target_include_directories(gmxapi_extension_library-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(gmxapi_extension_library-test Gromacs::libgromacs Gromacs::gmxapi GTest::Main)
gtest_add_tests(TARGET gmxapi_extension_library-test
                TEST_LIST BasicPlugin)

# Test the C++ force evaluation for the restrained-ensemble biasing potential.
add_executable(gmxapi_extension_histogram-test test_histogram.cpp)
add_dependencies(gmxapi_extension_histogram-test gmxapi_extension_spc2_water_box)
target_include_directories(gmxapi_extension_histogram-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(gmxapi_extension_histogram-test PROPERTIES SKIP_BUILD_RPATH FALSE)
target_link_libraries(gmxapi_extension_histogram-test gmxapi_extension_ensemblepotential Gromacs::gmxapi
                      GTest::Main)
gtest_add_tests(TARGET gmxapi_extension_histogram-test
                TEST_LIST EnsembleHistogramPotentialPlugin)

# Test the flat-bottom bounding potential built in to the ensemble restraint.
add_executable(gmxapi_extension_bounding-test test_bounding_restraint.cpp)
add_dependencies(gmxapi_extension_bounding-test gmxapi_extension_spc2_water_box)
target_include_directories(gmxapi_extension_bounding-test PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(gmxapi_extension_bounding-test PROPERTIES SKIP_BUILD_RPATH FALSE)
target_link_libraries(gmxapi_extension_bounding-test gmxapi_extension_ensemblepotential Gromacs::gmxapi
                      GTest::Main)
gtest_add_tests(TARGET gmxapi_extension_bounding-test
                TEST_LIST EnsembleBoundingPotentialPlugin)

if (NOT GMXAPI_EXTENSION_MASTER_PROJECT)
    include(CMakeGROMACS.txt)
endif ()

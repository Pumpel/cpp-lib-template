cmake_minimum_required(VERSION 3.14)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif ()

project(
        "FooLib" # This will exposed as the variable PROJECT_NAME.
        VERSION 0.1.0 # Used for installation and defines variables PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR,
        #PROJECT_VERSION_PATCH, and PROJECT_VERSION_TWEAK.
        LANGUAGES CXX
)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif ()

# Standard CMake modules

include(CTest)                          # Must be called before adding tests but after calling project(). This automatically calls enable_testing() and configures ctest targets when using Make/Ninja
include(CMakeDependentOption)           # This is a really useful scripts that creates options that depends on other options. It can even be used with generator expressions !
include(GNUInstallDirs)                 # This will define the default values for installation directories (all platforms even if named GNU)
include(InstallRequiredSystemLibraries) # Tell CMake that the `install` target needs to install required system libraries (eg: Windows SDK)
include(CMakePackageConfigHelpers)      # Helper to create relocatable packages

# Custom modules and scripts

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake") # Make our cmake scripts available

include(LTO)
include(Warnings)
include(CopyDllsForDebug)
include(Coverage)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(${PROJECT_NAME}_USE_DOXYGEN "Add a doxygen target to generate the documentation" ON)
option(${PROJECT_NAME}_USE_ADDITIONAL_SOURCEFILE "Use the additional source file" ON)
option(${PROJECT_NAME}_INSTALL "" ON)

# Use own option for tests, in case the library is added through add_subdirectory
cmake_dependent_option(${PROJECT_NAME}_BUILD_TESTS
                       "Enable ${PROJECT_NAME} project tests targets" ON # By default we want tests if CTest is enabled
                       "BUILD_TESTING" OFF # Stay coherent with CTest variables
                       )

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Building a shared library, do not export all symbols by default
# gcc (and hence clang) are wrong about this.
#
# For more information, see https://gcc.gnu.org/wiki/Visibility and https://www.youtube.com/embed/m0DwB4OvDXk
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# Check for LTO support (needs to be after project(...) )
find_lto(CXX)

find_package(fmt REQUIRED)

set(TARGET_NAME ${PROJECT_NAME})

set(TARGET_HEADER
    include/foo.h
    )

set(TARGET_SOURCES
    source/foo.cpp
    source/foo-impl.h
    )

add_library(${TARGET_NAME}
            ${TARGET_SOURCES}
            ${TARGET_HEADER}
            )

target_compile_features(${TARGET_NAME} PRIVATE cxx_std_14)

# CMake scripts extensions
target_set_warnings(${TARGET_NAME} ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_enable_lto(${TARGET_NAME} optimized) #enable lto if available for non-debug configurations

target_include_directories(${TARGET_NAME}
                           PUBLIC # The folder must be used in the include path for any target using this library
                           $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           # Due to the way installation work, we only want this path set when building, not once installed
                           )

# The following properties are useful when you want to have an 'install' target
set_target_properties(${TARGET_NAME}
                      PROPERTIES
                      # PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/foo.h
                      # Headers listed here will automatically be copied when installing.
                      # Note that directories hierarchy is not preserved.
                      DEBUG_POSTFIX d
                      )

# We tell CMake what are the target dependencies
target_link_libraries(${TARGET_NAME}
                      PRIVATE # fmt is only needed to build, not to use this library
                      fmt::fmt
                      )

add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})


# Tests
if (${PROJECT_NAME}_BUILD_TESTS)
    # Let the user add options to the test runner if needed
    set(TEST_RUNNER_PARAMS "--force-colors=true" CACHE STRING "Options to add to our test runners commands")

    # In a real project you most likely want to exclude test folders
    # list(APPEND CUSTOM_COVERAGE_EXCLUDE "/test/")

    #    add_subdirectory(tests)

    # Setup some custom variables and add them to the CTestCustom.cmake.in template to have custom ctest settings
    # For example, exclude some directories from the coverage reports such as third-parties and tests
    configure_file(
            ${CMAKE_CURRENT_LIST_DIR}/cmake/CTestCustom.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake
            @ONLY
    )
endif ()


# Doxygen
if (${PROJECT_NAME}_USE_DOXYGEN AND CMAKE_VERSION VERSION_GREATER_EQUAL 3.9)
    find_package(Doxygen
                 OPTIONAL_COMPONENTS dot mscgen dia
                 )
    if (DOXYGEN_FOUND)
        set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
        doxygen_add_docs(
                doc
                README.md source
                COMMENT "Generate man pages"
        )
    endif ()
endif ()


# Packaging
if (${PROJECT_NAME}_INSTALL)
    # https://cmake.org/cmake/help/latest/module/CPack.html#variables-common-to-all-cpack-generators
    set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_LIST_DIR}/README.md)
    include(CPack)

    # Let users choose where to install the cmake package descriptions
    # For that we make use of the CMake Cache
    set(${PROJECT_NAME}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "Path to install ${PROJECT_NAME} Config*.cmake files to.")
    set(${PROJECT_NAME}_MODULE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake" CACHE STRING "Path to install ${PROJECT_NAME}'s .cmake  module files to.")

    # Use version checking helper provided by CMake so that users can safely use a version number in their find_package calls
    write_basic_package_version_file(
            ${PROJECT_NAME}ConfigVersion.cmake # The name of the version file needed by find_package.
            VERSION ${PROJECT_VERSION}         # The version of the project, already set by the `project` command at the top of this file
            COMPATIBILITY SameMajorVersion     # We use semantic versioning, backward compatibility is only guaranteed for a same major version
    )


    # We will need our own file if we have our own dependencies or want some special behavior when the user calls find_package
    # otherwise we could simply install the exports as the ${PROJECT_NAME}Config.cmake
    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/${TARGET_NAME}Config.cmake.in # This is your template file
            ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake          # This is the resulting file
            INSTALL_DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}    # This is where the file will be installed
            # List of paths that needs to be relocated once installed
            # For example if the variable containing the path is named MY_PATH, all instances of @PACKAGE_MY_PATH@ in the template will be replaced by the relocated version of the path
            # This is mostly only needed when you want to install cmake modules or have an unusual layout that cmake is not aware of.
            PATH_VARS ${PROJECT_NAME}_MODULE_INSTALL_DIR # This will be exposed as @PACKAGE_CppTemplateLib_MODULE_INSTALL_DIR@ in the template file
            # Imported targets do not require the following macros
            NO_SET_AND_CHECK_MACRO
            NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )


    # The following will export the targets under the name ${PROJECT_NAME}_Targets, not install them yet
    # It will then need a call to `install(EXPORT)`
    install(
            TARGETS
            ${TARGET_NAME}
            # fmt # All targets compiled by this project must be exported
            EXPORT ${PROJECT_NAME}_Targets
            # Following is only needed pre-cmake3.14
            #    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            #    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            #    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            # If you want to split between runtime and dev for examples, take a look at COMPONENT, NAMELINK_COMPONENT etc
            # More info in Craig Scott's talk "Deep CMake for library authors" https://www.youtube.com/watch?v=m0DwB4OvDXk
            INCLUDES
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # Install all the exported targets under the ${PROJECT_NAME}_Targets name.
    install(
            EXPORT ${PROJECT_NAME}_Targets
            NAMESPACE ${PROJECT_NAME}::        # Link targets with transitive properties and not only the library
            FILE ${PROJECT_NAME}Targets.cmake  # This is the file that needs to be included from your *Config.cmake.
            DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
    )


    # So far we only installed the exported targets, now install the package config files.
    # If you do not list headers in the PUBLIC_HEADER property, you will need to copy them using `install(FILES)` or `install(DIRECTORY)` too.
    # In that case, you can use CMAKE_INSTALL_INCLUDEDIR as the base destination path.
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
            DESTINATION
            ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
            )

    install(FILES ${TARGET_HEADER} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

endif ()


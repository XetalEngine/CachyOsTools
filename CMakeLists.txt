cmake_minimum_required(VERSION 3.16)

project(CachyOsTools VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network DBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network DBus)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WINDOW_ENUM_X11 REQUIRED IMPORTED_TARGET xcb xcb-res)
pkg_check_modules(WINDOW_ENUM_WAYLAND REQUIRED IMPORTED_TARGET wayland-client)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
set(WINDOW_ENUM_PROTOCOL_DIR "${CMAKE_CURRENT_BINARY_DIR}/window-enum-protocols")
file(MAKE_DIRECTORY "${WINDOW_ENUM_PROTOCOL_DIR}")
foreach(protocol wlr-foreign-toplevel-management-unstable-v1 ext-foreign-toplevel-list-v1)
    set(xml "${CMAKE_CURRENT_SOURCE_DIR}/window_enum/protocols/${protocol}.xml")
    set(header "${WINDOW_ENUM_PROTOCOL_DIR}/${protocol}-client.h")
    set(code "${WINDOW_ENUM_PROTOCOL_DIR}/${protocol}-protocol.c")
    add_custom_command(OUTPUT "${header}" "${code}"
        COMMAND "${WAYLAND_SCANNER}" client-header "${xml}" "${header}"
        COMMAND "${WAYLAND_SCANNER}" private-code "${xml}" "${code}"
        DEPENDS "${xml}" VERBATIM)
    list(APPEND WINDOW_ENUM_PROTOCOLS "${header}" "${code}")
endforeach()

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        # Header files containing implementations
        dual_gpu_kvm.h
        single_gpu_kvm.h
        device_manager.h
        users_groups.h
        restore_points.h
        firewall_manager.h
        kernel_manager.h
        env_editor.h
        system_properties.h
        drive.h
        mainwindow_d.h
        systembackup.h
        systemrestore.h
        systemlogs.h
        systemconfigs.h
        tweaks.h
        kde_tweaks.h
        kde_fixes.h
        shell.h
        executecommands.h
        current_system_iso_creator.h
        services.h
        includes.h
        uninstalltab.h
        tabupgrades.h
        thememanager.h
        dashboard.h
        isoexclusions.h
        cleaner.h
        script_helpers.h
        pacman_doctor.h
        sensors_power.h
        network_shares.h
        autostart.h
        window_enum.h
        window_enum.cpp
        window_enum_controller.cpp
        window_enum_backend.h
        window_enum_backend.cpp
        window_enum_x11.cpp
        window_enum_wayland.cpp
        ${WINDOW_ENUM_PROTOCOLS}
        resources.qrc
        # Additional files (currently empty but may be implemented)
        drivetools.h
        drivetools.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CachyOsTools
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET CachyOsTools APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(CachyOsTools SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(CachyOsTools
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(CachyOsTools PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::DBus)
target_link_libraries(CachyOsTools PRIVATE PkgConfig::WINDOW_ENUM_X11 PkgConfig::WINDOW_ENUM_WAYLAND)
target_include_directories(CachyOsTools PRIVATE "${WINDOW_ENUM_PROTOCOL_DIR}")

option(BUILD_WINDOW_ENUM_TESTS "Build isolated window enumeration tests" OFF)
if(BUILD_WINDOW_ENUM_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(window_enum_tests tests/window_enum_tests.cpp window_enum.h
        window_enum.cpp window_enum_controller.cpp window_enum_backend.cpp window_enum_x11.cpp resources.qrc)
    target_include_directories(window_enum_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(window_enum_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Test PkgConfig::WINDOW_ENUM_X11)
    add_test(NAME window_enum_x11 COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_window_enum_tests.py" $<TARGET_FILE:window_enum_tests>)
    pkg_check_modules(WINDOW_ENUM_SERVER REQUIRED IMPORTED_TARGET wayland-server)
    foreach(protocol wlr-foreign-toplevel-management-unstable-v1 ext-foreign-toplevel-list-v1)
        set(header "${WINDOW_ENUM_PROTOCOL_DIR}/${protocol}-server.h")
        add_custom_command(OUTPUT "${header}"
            COMMAND "${WAYLAND_SCANNER}" server-header
                "${CMAKE_CURRENT_SOURCE_DIR}/window_enum/protocols/${protocol}.xml" "${header}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/window_enum/protocols/${protocol}.xml" VERBATIM)
        list(APPEND WINDOW_ENUM_SERVER_HEADERS "${header}")
    endforeach()
    add_executable(window_enum_wayland_fixture tests/window_enum_wayland_fixture.cpp
        ${WINDOW_ENUM_PROTOCOLS} ${WINDOW_ENUM_SERVER_HEADERS})
    target_include_directories(window_enum_wayland_fixture PRIVATE "${WINDOW_ENUM_PROTOCOL_DIR}")
    target_link_libraries(window_enum_wayland_fixture PRIVATE PkgConfig::WINDOW_ENUM_SERVER)
    add_test(NAME window_enum_wayland COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_window_enum_wayland_tests.py"
        $<TARGET_FILE:CachyOsTools> $<TARGET_FILE:window_enum_wayland_fixture>)
endif()

# GUI translations (pt_BR, de). Skipped gracefully when LinguistTools is not
# installed — the app then simply runs in English.
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 QUIET COMPONENTS LinguistTools)
    if(Qt6LinguistTools_FOUND)
        qt_add_translations(CachyOsTools TS_FILES
            translations/cachyostools_pt_BR.ts
            translations/cachyostools_de.ts)
    endif()
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.CachyOsTools)
endif()
set_target_properties(CachyOsTools PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS CachyOsTools
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CachyOsTools)
endif()

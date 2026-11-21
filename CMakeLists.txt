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

include(GNUInstallDirs)
add_compile_definitions(CACHYOSTOOLS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CACHYOSTOOLS_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/cachyostools")

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        isoexclusionsdialog.ui
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
        modern_interface.h
        drive_tree.cpp drive_tree.h volume_commands.h
        ui_components/panel_widgets.h
        ui_components/hyper_fusion_banner.h
        ui_components/presentation.cpp ui_components/presentation.h
        ui_components/network_page.cpp ui_components/network_page.h ui_components/network_status.h
        ui_components/grub_editor.cpp ui_components/grub_editor.h
        ui_components/grub_appearance.cpp ui_components/grub_appearance.h
        ui_components/disk_visualizer.cpp ui_components/disk_visualizer.h ui_components/disk_layout.h
        window_enum_hyprland.cpp window_enum_hyprland.h
        drivelabel.h
        drivelabel.cpp
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
        themepresets.h
        preferences.h
        preferencesdialog.h
        preferencesdialog.cpp
        dashboard.h
        isoexclusions.h
        cleaner.h
        script_helpers.h
        file_manager.h
        package_install.h
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
target_include_directories(CachyOsTools PRIVATE "${WINDOW_ENUM_PROTOCOL_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/ui_components")

option(BUILD_WINDOW_ENUM_TESTS "Build isolated window enumeration tests" OFF)
if(BUILD_WINDOW_ENUM_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(window_enum_tests tests/window_enum_tests.cpp window_enum.h
        window_enum.cpp window_enum_controller.cpp window_enum_backend.cpp window_enum_x11.cpp window_enum_hyprland.cpp resources.qrc)
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

option(BUILD_PREFERENCES_TESTS "Build preferences dialog and persistence tests" OFF)
if(BUILD_PREFERENCES_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(preferences_dialog_tests tests/preferences_dialog_tests.cpp
        preferencesdialog.cpp preferencesdialog.h themepresets.h)
    target_include_directories(preferences_dialog_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(preferences_dialog_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME preferences_dialog COMMAND preferences_dialog_tests)
    set_tests_properties(preferences_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

option(BUILD_DRIVE_LABEL_TESTS "Build isolated volume label tests" OFF)
if(BUILD_DRIVE_LABEL_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_executable(drive_label_tests tests/drive_label_tests.cpp drivelabel.cpp drivelabel.h)
    target_include_directories(drive_label_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(drive_label_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME drive_label COMMAND drive_label_tests)
    set_tests_properties(drive_label PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

option(BUILD_FILE_MANAGER_TESTS "Build isolated folder-opening tests" OFF)
if(BUILD_FILE_MANAGER_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    find_program(DBUS_RUN_SESSION dbus-run-session REQUIRED)
    add_executable(file_manager_tests tests/file_manager_tests.cpp file_manager.h)
    target_include_directories(file_manager_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(file_manager_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Test)
    add_test(NAME file_manager COMMAND "${DBUS_RUN_SESSION}"
        "--config-file=${CMAKE_CURRENT_SOURCE_DIR}/tests/file_manager_bus.conf"
        -- $<TARGET_FILE:file_manager_tests>)
    set_tests_properties(file_manager PROPERTIES
        ENVIRONMENT "CACHY_FILE_MANAGER_TEST_BUS=1;QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME="
        TIMEOUT 30)
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

install(TARGETS CachyOsTools
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CachyOsTools)
endif()

install(DIRECTORY tools/ DESTINATION ${CMAKE_INSTALL_DATADIR}/cachyostools/tools FILES_MATCHING PATTERN "*.py" PATTERN "__pycache__" EXCLUDE)
install(DIRECTORY assets/fonts/ DESTINATION ${CMAKE_INSTALL_DATADIR}/cachyostools/assets/fonts)

option(BUILD_MODERN_UI_TESTS "Build standalone GRUB, drive, network, disk and Hyprland tests" OFF)
if(BUILD_MODERN_UI_TESTS)
    enable_testing()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    foreach(test drive_tree network_page grub_editor disk_visualizer hyprland)
        if(test STREQUAL "drive_tree")
            set(extra_sources drive_tree.cpp drivelabel.cpp)
        elseif(test STREQUAL "network_page")
            set(extra_sources ui_components/network_page.cpp)
        elseif(test STREQUAL "grub_editor")
            set(extra_sources ui_components/grub_editor.cpp ui_components/grub_appearance.cpp)
        elseif(test STREQUAL "disk_visualizer")
            set(extra_sources ui_components/disk_visualizer.cpp)
        else()
            set(extra_sources window_enum_hyprland.cpp window_enum.cpp window_enum_controller.cpp window_enum_backend.cpp)
        endif()
        add_executable(${test}_tests tests/${test}_tests.cpp ${extra_sources}
            ui_components/presentation.cpp ui_components/panel_widgets.h)
        target_include_directories(${test}_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/ui_components")
        target_link_libraries(${test}_tests PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Test)
        add_test(NAME ${test} COMMAND ${test}_tests)
        set_tests_properties(${test} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME=" TIMEOUT 120)
    endforeach()
    set(SMOKE_SOURCES ${PROJECT_SOURCES})
    list(REMOVE_ITEM SMOKE_SOURCES main.cpp)
    add_executable(mainwindow_smoke tests/mainwindow_smoke.cpp ${SMOKE_SOURCES})
    target_include_directories(mainwindow_smoke PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/ui_components" "${WINDOW_ENUM_PROTOCOL_DIR}")
    target_link_libraries(mainwindow_smoke PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network
        Qt${QT_VERSION_MAJOR}::DBus Qt${QT_VERSION_MAJOR}::Test PkgConfig::WINDOW_ENUM_X11 PkgConfig::WINDOW_ENUM_WAYLAND)
    add_test(NAME mainwindow_smoke COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_mainwindow_smoke.py" $<TARGET_FILE:mainwindow_smoke>)
    set_tests_properties(mainwindow_smoke PROPERTIES TIMEOUT 120)
    foreach(helper grub_editor grub_appearance disk_scan)
        add_test(NAME ${helper}_helper COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_${helper}.py")
        set_tests_properties(${helper}_helper PROPERTIES TIMEOUT 240)
    endforeach()
endif()

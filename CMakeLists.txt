cmake_minimum_required(VERSION 3.16)
project(cutefish-launcher)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    DBus
    Quick
    QuickControls2
    LinguistTools
)

find_package(KF6WindowSystem REQUIRED)

# Source files
set(SRCS
    src/desktopproperties.cpp
    src/iconthemeimageprovider.cpp
    src/launcher.cpp
    src/launchermodel.cpp
    src/appitem.cpp
    src/main.cpp
    src/pagemodel.cpp
    src/ucunits.cpp
    src/listmodelmanager.cpp
    src/iconitem.cpp
    src/processprovider.cpp
    src/appmanager.cpp
)

# QML resources
set(RESOURCES
    qml.qrc
)

# ------------------------------------------------------------
# Qt6 D-Bus adaptor generation
# ------------------------------------------------------------
set(DBUS_XML ${CMAKE_CURRENT_SOURCE_DIR}/src/com.cutefish.Launcher.xml)
set(DBUS_PARENT_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/launcher.h)

qt6_add_dbus_adaptor(DBUS_SRCS
    ${DBUS_XML}
    ${DBUS_PARENT_HEADER}
)

message(STATUS "DBus adaptor generated: ${DBUS_SRCS}")

# ------------------------------------------------------------

add_executable(${PROJECT_NAME}
    ${SRCS}
    ${DBUS_SRCS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::DBus
        Qt6::Quick
        Qt6::QuickControls2
        KF6::WindowSystem
)

# -----------

cmake_minimum_required(VERSION 3.20)

project(NomoIconTheme)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui GuiPrivate Svg REQUIRED)

add_qtc_plugin(NomoIconTheme
  LONG_DESCRIPTION_MD README.md
  PLUGIN_DEPENDS
    QtCreator::Core
  DEPENDS
    Qt::Gui Qt::GuiPrivate Qt::Svg
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    plugin.cpp
    icontheme.qrc
    README.md
)

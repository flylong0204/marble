PROJECT( CycleStreetsPlugin )

INCLUDE_DIRECTORIES(
 ${CMAKE_CURRENT_SOURCE_DIR}
 ${CMAKE_CURRENT_BINARY_DIR}
 ${QT_INCLUDE_DIR}
)

INCLUDE_DIRECTORIES(
  ${Qt5WebKitWidgets_INCLUDE_DIRS}
  ${Qt5Xml_INCLUDE_DIRS}
)

set( cyclestreets_SRCS CycleStreetsRunner.cpp CycleStreetsPlugin.cpp )

set( cyclestreets_UI CycleStreetsConfigWidget.ui )

qt_wrap_ui( cyclestreets_SRCS ${cyclestreets_UI} )

marble_add_plugin( CycleStreetsPlugin ${cyclestreets_SRCS} )

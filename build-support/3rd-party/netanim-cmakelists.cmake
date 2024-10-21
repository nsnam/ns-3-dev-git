# Copyright (c) 2017-2021 Universidade de Brasília
#
# SPDX-License-Identifier: GPL-2.0-only
#
# Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

find_package(Qt4 COMPONENTS QtGui QUIET)
find_package(Qt5 COMPONENTS Core Widgets PrintSupport Gui QUIET)

add_definitions(-Wno-error=misleading-indentation)

if((NOT ${Qt4_FOUND}) AND (NOT ${Qt5_FOUND}))
  message(FATAL_ERROR "You need Qt installed to build NetAnim")
endif()

# Qt 4 requires these inclusions
if(NOT ${Qt5_found})
  include(${QT_USE_FILE})
  add_definitions(${QT_DEFINITIONS})
  include_directories(${QT_INCLUDES})
endif()

# Used by qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(source_files
    animatormode.cpp
    animatorscene.cpp
    animatorview.cpp
    animlink.cpp
    animnode.cpp
    animpacket.cpp
    animpropertybrowser.cpp
    animresource.cpp
    animxmlparser.cpp
    countertablesscene.cpp
    flowmonstatsscene.cpp
    flowmonxmlparser.cpp
    graphpacket.cpp
    interfacestatsscene.cpp
    log.cpp
    logqt.cpp
    main.cpp
    mode.cpp
    netanim.cpp
    packetsmode.cpp
    packetsscene.cpp
    packetsview.cpp
    qcustomplot.cpp
    qtpropertybrowser/src/fileedit.cpp
    qtpropertybrowser/src/fileeditfactory.cpp
    qtpropertybrowser/src/filepathmanager.cpp
    qtpropertybrowser/src/qtbuttonpropertybrowser.cpp
    qtpropertybrowser/src/qteditorfactory.cpp
    qtpropertybrowser/src/qtgroupboxpropertybrowser.cpp
    qtpropertybrowser/src/qtpropertybrowser.cpp
    qtpropertybrowser/src/qtpropertybrowserutils.cpp
    qtpropertybrowser/src/qtpropertymanager.cpp
    qtpropertybrowser/src/qttreepropertybrowser.cpp
    qtpropertybrowser/src/qtvariantproperty.cpp
    resizeableitem.cpp
    routingstatsscene.cpp
    routingxmlparser.cpp
    statsmode.cpp
    statsview.cpp
    table.cpp
    textbubble.cpp
)

set(header_files
    abort.h
    animatorconstants.h
    animatormode.h
    animatorscene.h
    animatorview.h
    animevent.h
    animlink.h
    animnode.h
    animpacket.h
    animpropertybrowser.h
    animresource.h
    animxmlparser.h
    common.h
    countertablesscene.h
    flowmonstatsscene.h
    flowmonxmlparser.h
    graphpacket.h
    interfacestatsscene.h
    logqt.h
    mode.h
    netanim.h
    packetsmode.h
    packetsscene.h
    packetsview.h
    qcustomplot.h
    qtpropertybrowser/src/QtAbstractEditorFactoryBase
    qtpropertybrowser/src/QtAbstractPropertyBrowser
    qtpropertybrowser/src/QtAbstractPropertyManager
    qtpropertybrowser/src/QtBoolPropertyManager
    qtpropertybrowser/src/QtBrowserItem
    qtpropertybrowser/src/QtButtonPropertyBrowser
    qtpropertybrowser/src/QtCharEditorFactory
    qtpropertybrowser/src/QtCharPropertyManager
    qtpropertybrowser/src/QtCheckBoxFactory
    qtpropertybrowser/src/QtColorEditorFactory
    qtpropertybrowser/src/QtColorPropertyManager
    qtpropertybrowser/src/QtCursorEditorFactory
    qtpropertybrowser/src/QtCursorPropertyManager
    qtpropertybrowser/src/QtDateEditFactory
    qtpropertybrowser/src/QtDatePropertyManager
    qtpropertybrowser/src/QtDateTimeEditFactory
    qtpropertybrowser/src/QtDateTimePropertyManager
    qtpropertybrowser/src/QtDoublePropertyManager
    qtpropertybrowser/src/QtDoubleSpinBoxFactory
    qtpropertybrowser/src/QtEnumEditorFactory
    qtpropertybrowser/src/QtEnumPropertyManager
    qtpropertybrowser/src/QtFlagPropertyManager
    qtpropertybrowser/src/QtFontEditorFactory
    qtpropertybrowser/src/QtFontPropertyManager
    qtpropertybrowser/src/QtGroupBoxPropertyBrowser
    qtpropertybrowser/src/QtGroupPropertyManager
    qtpropertybrowser/src/QtIntPropertyManager
    qtpropertybrowser/src/QtKeySequenceEditorFactory
    qtpropertybrowser/src/QtKeySequencePropertyManager
    qtpropertybrowser/src/QtLineEditFactory
    qtpropertybrowser/src/QtLocalePropertyManager
    qtpropertybrowser/src/QtPointFPropertyManager
    qtpropertybrowser/src/QtPointPropertyManager
    qtpropertybrowser/src/QtProperty
    qtpropertybrowser/src/QtRectFPropertyManager
    qtpropertybrowser/src/QtRectPropertyManager
    qtpropertybrowser/src/QtScrollBarFactory
    qtpropertybrowser/src/QtSizeFPropertyManager
    qtpropertybrowser/src/QtSizePolicyPropertyManager
    qtpropertybrowser/src/QtSizePropertyManager
    qtpropertybrowser/src/QtSliderFactory
    qtpropertybrowser/src/QtSpinBoxFactory
    qtpropertybrowser/src/QtStringPropertyManager
    qtpropertybrowser/src/QtTimeEditFactory
    qtpropertybrowser/src/QtTimePropertyManager
    qtpropertybrowser/src/QtTreePropertyBrowser
    qtpropertybrowser/src/QtVariantEditorFactory
    qtpropertybrowser/src/QtVariantProperty
    qtpropertybrowser/src/QtVariantPropertyManager
    qtpropertybrowser/src/fileedit.h
    qtpropertybrowser/src/fileeditfactory.h
    qtpropertybrowser/src/filepathmanager.h
    qtpropertybrowser/src/qtbuttonpropertybrowser.h
    qtpropertybrowser/src/qteditorfactory.h
    qtpropertybrowser/src/qtgroupboxpropertybrowser.h
    qtpropertybrowser/src/qtpropertybrowser.h
    qtpropertybrowser/src/qtpropertybrowserutils_p.h
    qtpropertybrowser/src/qtpropertymanager.h
    qtpropertybrowser/src/qttreepropertybrowser.h
    qtpropertybrowser/src/qtvariantproperty.h
    resizeableitem.h
    routingstatsscene.h
    routingxmlparser.h
    statisticsconstants.h
    statsmode.h
    statsview.h
    table.h
    textbubble.h
    timevalue.h
)

set(resource_files resources.qrc qtpropertybrowser/src/qtpropertybrowser.qrc)

add_executable(netanim_visualizer ${source_files} ${resource_files})

if(Qt4_FOUND)
  target_link_libraries(netanim_visualizer PUBLIC ${libcore} Qt4::QtGui)
else()
  target_link_libraries(
    netanim_visualizer PUBLIC ${libcore} Qt5::Widgets Qt5::Core
                              Qt5::PrintSupport Qt5::Gui
  )
endif()

target_include_directories(netanim_visualizer PUBLIC qtpropertybrowser/src)
set_runtime_outputdirectory(
  netanim_visualizer ${CMAKE_OUTPUT_DIRECTORY}/bin/ ""
)

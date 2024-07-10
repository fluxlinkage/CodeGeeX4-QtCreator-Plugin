cmake_minimum_required(VERSION 3.10)

project(CodeGeeX4)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(QtCreator REQUIRED COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
set(QtX Qt${QT_VERSION_MAJOR})

# Add a CMake option that enables building your plugin with tests.
# You don't want your released plugin binaries to contain tests,
# so make that default to 'NO'.
# Enable tests by passing -DWITH_TESTS=ON to CMake.
option(WITH_TESTS "Builds with tests" NO)

if(WITH_TESTS)
  # Look for QtTest
  find_package(${QtX} REQUIRED COMPONENTS Test)

  # Tell CMake functions like add_qtc_plugin about the QtTest component.
  set(IMPLICIT_DEPENDS Qt::Test)

  # Enable ctest for auto tests.
  enable_testing()
endif()

add_qtc_plugin(CodeGeeX4
  PLUGIN_DEPENDS
    QtCreator::Core
    QtCreator::LanguageClient
    QtCreator::ProjectExplorer
    QtCreator::TextEditor
  DEPENDS
    ${QtX}::Widgets
    ${QtX}::Network
    QtCreator::ExtensionSystem
    QtCreator::Utils
  SOURCES
    codegeex4client.cpp
    codegeex4client.h
    codegeex4clientinterface.cpp
    codegeex4clientinterface.h
    codegeex4constants.h
    codegeex4hoverhandler.cpp
    codegeex4hoverhandler.h
    codegeex4icons.h
    #codegeex4optionspage.cpp
    #codegeex4optionspage.h
    codegeex4plugin.cpp
    codegeex4projectpanel.cpp
    codegeex4projectpanel.h
    codegeex4settings.cpp
    codegeex4settings.h
    codegeex4suggestion.cpp
    codegeex4suggestion.h
    codegeex4tr.h
    requests/getcompletions.h
    codegeex4.qrc
)

qt_add_translations(CodeGeeX4 TS_FILES CodeGeeX4_zh_CN.ts
  SOURCES
    codegeex4hoverhandler.cpp
    codegeex4plugin.cpp
    codegeex4settings.cpp
)
# qt_add_lupdate(CodeGeeX4 TS_FILES CodeGeeX4_zh_CN.ts
#     SOURCES
#     codegeex4hoverhandler.cpp
#     codegeex4plugin.cpp
#     codegeex4settings.cpp
# )
install(FILES ${PROJECT_BINARY_DIR}/CodeGeeX4_zh_CN.qm DESTINATION share/qtcreator/translations)

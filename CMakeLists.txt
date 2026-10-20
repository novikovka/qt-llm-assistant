cmake_minimum_required(VERSION 3.16)

project(QtLlmAssistant VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Widgets — окна и элементы интерфейса, Network — HTTP-запросы к llama-server
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Стандартные настройки Qt: включает AUTOMOC (обработку Q_OBJECT) и т. п.
qt_standard_project_setup()

qt_add_executable(qt-llm-assistant
    main.cpp
    mainwindow.h
    mainwindow.cpp
    llmclient.h
    llmclient.cpp
)

target_link_libraries(qt-llm-assistant PRIVATE
    Qt6::Widgets
    Qt6::Network
)

list(APPEND ddui_table_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/model.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/style.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/view.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/view.cpp
)
set(ddui_table_SOURCES ${ddui_table_SOURCES} PARENT_SCOPE)

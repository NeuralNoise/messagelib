include(ECMAddTests)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(templateeditor_gui_SRCS templateeditor_gui.cpp)
add_executable(templateeditor_gui ${templateeditor_gui_SRCS})
target_link_libraries(templateeditor_gui
  KF5::KIOCore
  KF5::TemplateParser
  KF5::PimTextEdit
  KF5::I18n
  KF5::MessageViewer
)

set(templateconfigurewidget_gui_SRCS templateconfigurewidget_gui.cpp)
add_executable(templateconfigurewidget_gui ${templateconfigurewidget_gui_SRCS})
target_link_libraries(templateconfigurewidget_gui
  KF5::KIOCore
  KF5::TemplateParser
  KF5::I18n
  KF5::PimTextEdit
  KF5::MessageViewer
)

set(templateparseremailrequester_gui_SRCS templateparseremailrequester.cpp)
add_executable(templateparseremailrequester_gui ${templateparseremailrequester_gui_SRCS})
target_link_libraries(templateparseremailrequester_gui
  KF5::TemplateParser
  KF5::I18n
  KF5::MessageViewer
  KF5::PimTextEdit
)


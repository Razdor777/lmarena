target_sources(Dexko PRIVATE
        "imgui.cpp"
        "imgui_draw.cpp"
        "imgui_impl_dx11.cpp"
        "imgui_impl_win32.cpp"
        "imgui_tables.cpp"
        "imgui_widgets.cpp"
        "stb_image.h"
)

target_include_directories(Dexko PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

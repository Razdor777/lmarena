# Dexko renders through D3D11 + the Win32 platform only. There is no ClickGui
# (so no imgui_toggle) and no demo window, so those files are deliberately left
# out of the build — compiling them used to add about a megabyte to the DLL for
# nothing. Add them back here if the feature ever returns.
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

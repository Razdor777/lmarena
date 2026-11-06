# ==== ported from the 1.21.44 master ====
# target был Dexko — переименован в Solstice вместе с проектом.
# imgui_freetype.cpp НЕ собираем: он требует ft2build.h + lunasvg, которых в дереве нет.
# Флаг ImGuiFreeTypeBuilderFlags_NoHinting из pch работать не будет, но он и не обязателен —
# ImGui просто игнорирует FontBuilderFlags без IMGUI_ENABLE_FREETYPE.
target_sources(Solstice PRIVATE
        "imgui.cpp"
        "imgui_draw.cpp"
        "imgui_impl_dx11.cpp"
        "imgui_impl_win32.cpp"
        "imgui_tables.cpp"
        "imgui_widgets.cpp"
        "stb_image.h"
)

target_include_directories(Solstice PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

#pragma once
#include <Utils/Structs.hpp>
//
//



class RenderUtils {
public:
    static inline FrameTransform transform;

    static void drawOutlinedAABB(const AABB& aabb, bool filled, const ImColor& color = ImColor(255, 255, 255, 255));
    static bool worldToScreen(glm::vec3 pos, ImVec2& screenPos);
};
//
// Created by vastrakai on 9/25/2024.
//

#include "ScriptingGui.hpp"

#include <Features/FeatureManager.hpp>
#include <Utils/FontHelper.hpp>
#include <Utils/MiscUtils/ColorUtils.hpp>
#include <Utils/MiscUtils/ImRenderUtils.hpp>
#include <Utils/MiscUtils/MathUtils.hpp>

void ScriptingGui::render(float inScale, float animation, ImVec2 screen, float blurStrength)
{
    FontHelper::pushPrefFont();

    auto drawList = ImGui::GetBackgroundDrawList();
    drawList->AddRectFilled(ImVec2(0, 0), ImVec2(screen.x, screen.y), IM_COL32(0, 0, 0, 255 * animation * 0.38f));
    ImRenderUtils::addBlur(ImVec4(0.f, 0.f, screen.x, screen.y),
                           animation * blurStrength, 0);

    ImColor shadowRectColor = ColorUtils::getThemedColor(0);
    shadowRectColor.Value.w = 0.5f * animation; // Adjust rect alpha with animation

    float firstheight = (screen.y - screen.y / 3);

    firstheight = MathUtils::lerp(screen.y, firstheight, inScale);
    ImRenderUtils::fillGradientOpaqueRectangle(
        ImVec4(0, firstheight, screen.x, screen.y),
        shadowRectColor, shadowRectColor, 0.4f * inScale, 0.0f);


    ImVec2 windowSize = ImVec2(800.0f * inScale, 600.0f * inScale);  // Scale window size
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGui::SetNextWindowBgAlpha(0.8f * inScale);  // Control opacity using animation

    ImGui::Begin("Scripting");

    ImGui::Text("Scripts:");
    auto& sm = gFeatureManager->mScriptManager;
    if (!sm)
    {
        ImGui::TextDisabled("Script manager is not running.");
        FontHelper::popPrefFont();
        ImGui::End();
        return;
    }

    if (sm->mLoadingScripts)
    {
        ImGui::Text("Loading scripts...");
        FontHelper::popPrefFont();
        ImGui::End();
        return;
    }

    auto& scripts = sm->getScripts();
    ImGui::SameLine();
    ImGui::TextDisabled("(%zu)", scripts.size());

    if (ImGui::Button("Reload all"))
        sm->reloadAll();

    ImGui::Separator();

    int i = 0;
    for (auto& script : scripts)
    {
        if (!script || script->mType != ScriptType::Module) continue;
        i++;

        bool enabled = script->mEnabled;
        const std::string label = "##enabled" + std::to_string(i);
        if (ImGui::Checkbox(label.c_str(), &enabled))
            sm->setScriptEnabled(script, enabled);

        ImGui::SameLine();
        ImGui::TextUnformatted(script->mName.c_str());
        ImGui::SameLine();

        std::string desc = "- " + script->mDescription;
        ImGui::TextDisabled("%s", desc.c_str());

        if (!script->mAuthor.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImColor(1.0f, 0.84f, 0.0f, 1.0f), "made by");
            ImGui::SameLine();
            ImGui::TextColored(ImColor(0.0f, 0.5f, 1.0f, 1.0f), "%s", script->mAuthor.c_str());
        }
    }

    if (i == 0)
        ImGui::TextDisabled("No module scripts found in Scripts\\Modules");

    FontHelper::popPrefFont();
    ImGui::End();
}

//
//

#include "LevelRenderer.hpp"

#include <libhat/access.hpp>
#include <SDK/OffsetProvider.hpp>

glm::vec3* LevelRendererPlayer::getCameraPos()
{
    return &hat::member_at<glm::vec3>(this, OffsetProvider::LevelRendererPlayer_mCameraPos);
}

float* LevelRendererPlayer::getFovX()
{
    return &hat::member_at<float>(this, OffsetProvider::LevelRendererPlayer_mFovX);
}

float* LevelRendererPlayer::getFovY()
{
    return &hat::member_at<float>(this, OffsetProvider::LevelRendererPlayer_mFovY);
}

void LevelRendererPlayer::setFovX(float v)
{
    hat::member_at<float>(this, OffsetProvider::LevelRendererPlayer_mFovX) = v;
}

void LevelRendererPlayer::setFovY(float v)
{
    hat::member_at<float>(this, OffsetProvider::LevelRendererPlayer_mFovY) = v;
}

LevelRendererPlayer* LevelRenderer::getRendererPlayer()
{
    return hat::member_at<LevelRendererPlayer*>(this, OffsetProvider::LevelRenderer_mRendererPlayer);
}

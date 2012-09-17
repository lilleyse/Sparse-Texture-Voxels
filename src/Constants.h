#pragma once

const unsigned int POSITION_ATTR(0);
const unsigned int DEBUG_TRANSFORM_ATTR(1);
const unsigned int DEBUG_COLOR_ATTR(2);
const unsigned int PER_FRAME_UBO_BINDING(0);

struct PerFrameUBO
{
	glm::mat4 viewProjection;
};
#pragma once

#include "glm_config.h"


/**
 * Calculate normal vector for a triangle using its vertices (v1, v2, v3)
 *
 * Note: order of vertices is important, it must have the same order as during rendering
 * otherwise the normal will point to the inverse of the "true" plane normal.
 */
glm::vec3 calculateNormal(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3) {
    const glm::vec3 v1v2 = v1 - v2;
    const glm::vec3 v2v3 = v2 - v3;

    const glm::vec3 cross = glm::cross(v1v2, v2v3);
    return glm::normalize(cross);
}

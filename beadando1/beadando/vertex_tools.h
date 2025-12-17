#pragma once

#include "glm_config.h"
#include "vertex.h"

/**
 * Calculate normal vector for a triangle using its vertices (v1, v2, v3)
 *
 * Note: order of vertices is important, it must have the same order as during rendering
 * otherwise the normal will point to the inverse of the "true" plane normal.
 */
inline glm::vec3 calculateNormal(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2)
{
  return glm::normalize(glm::cross(v1 - v0, v2 - v0));
}


/**
 * Calculate normal vector for every vertex.
 */
inline void generateVertexNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& index_list){

  // Accumulate face normals (over every triangle)
  for (size_t i = 0; i + 2 < index_list.size(); i += 3) {
    // The triangle's vertices
    Vertex& a = vertices[index_list[i]];
    Vertex& b = vertices[index_list[i + 1]];
    Vertex& c = vertices[index_list[i + 2]];

    // calculate faceNormal
    glm::vec3 faceNormal = calculateNormal(
        {a.x, a.y, a.z},
        {b.x, b.y, b.z},
        {c.x, c.y, c.z}
    );

    //accumulating normals for each vertex
    a.n1 += faceNormal.x;
    a.n2 += faceNormal.y;
    a.n3 += faceNormal.z;

    b.n1 += faceNormal.x;
    b.n2 += faceNormal.y;
    b.n3 += faceNormal.z;

    c.n1 += faceNormal.x;
    c.n2 += faceNormal.y;
    c.n3 += faceNormal.z;
  }

  // Normalize vertex normals
  for (auto& v : vertices) {
    glm::vec3 n(v.n1, v.n2, v.n3);
    if (glm::length(n) > 0.00001f) { //ezt a feltételt chatgpt ajánlotta normalizálás miatti bugok ellen, én elhittem neki a magyarázatot
      n = glm::normalize(n);
      v.n1 = n.x;
      v.n2 = n.y;
      v.n3 = n.z;
    }
  }
}

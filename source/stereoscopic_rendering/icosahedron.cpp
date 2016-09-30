
#include "icosahedron.h"

#include <iterator>
#include <algorithm>

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>

#include <glbinding/gl41core/enum.h>
#include <glbinding/gl41core/functions.h>
#include <glbinding/gl41core/boolean.h>


using namespace glm;
using namespace gl41core;

std::array<vec3, 12> Icosahedron::vertices()
{
    static const float t = (1.f + glm::sqrt(5.f)) * 0.5f; // 2.118
    static const float i = glm::inversesqrt(t * t + 1.f);  // 0.427
    static const float a = t * i;                     // 0.904

    // icosahedron vertices (normalized) form three orthogonal golden rectangles
    // http://en.wikipedia.org/wiki/Icosahedron#Cartesian_coordinates

    return std::array<vec3, 12>{{
        vec3(-i, a, 0) 
    ,   vec3( i, a, 0)
    ,   vec3(-i,-a, 0)
    ,   vec3( i,-a, 0)
    ,   vec3( 0,-i, a)
    ,   vec3( 0, i, a)
    ,   vec3( 0,-i,-a)
    ,   vec3( 0, i,-a)
    ,   vec3( a, 0,-i)
    ,   vec3( a, 0, i)
    ,   vec3(-a, 0,-i)
    ,   vec3(-a, 0, i)
    }};
}

std::array<Icosahedron::Face, 20> Icosahedron::indices()
{
    return std::array<Face, 20>{{
        Face{{  0, 11,  5 }}
    ,   Face{{  0,  5,  1 }}
    ,   Face{{  0,  1,  7 }}
    ,   Face{{  0,  7, 10 }}
    ,   Face{{  0, 10, 11 }}

    ,   Face{{  1,  5,  9 }}
    ,   Face{{  5, 11,  4 }}
    ,   Face{{ 11, 10,  2 }}
    ,   Face{{ 10,  7,  6 }}
    ,   Face{{  7,  1,  8 }}

    ,   Face{{  3,  9,  4 }}
    ,   Face{{  3,  4,  2 }}
    ,   Face{{  3,  2,  6 }}
    ,   Face{{  3,  6,  8 }}
    ,   Face{{  3,  8,  9 }}

    ,   Face{{  4,  9,  5 }}
    ,   Face{{  2,  4, 11 }}
    ,   Face{{  6,  2, 10 }}
    ,   Face{{  8,  6,  7 }}
    ,   Face{{  9,  8,  1 }}
    }};
}

Icosahedron::Icosahedron(const GLsizei iterations, const GLint positionLocation, const GLint normalLocation)
: m_vao(0)
, m_vertices(0)
, m_indices(0)
{
    auto v(vertices());
    auto i(indices());

    std::vector<vec3> vertices(v.begin(), v.end());
    std::vector<Face> indices(i.begin(), i.end());

    refine(vertices, indices, static_cast<char>(glm::clamp(iterations, 0, 8)));

    glGenBuffers(1, &m_vertices);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_indices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Face) * indices.size(), indices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_size = static_cast<GLsizei>(indices.size() * std::tuple_size<Face>::value);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indices);

    if (positionLocation >= 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
		glEnableVertexAttribArray(positionLocation);
    }

    if (normalLocation >= 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
        glVertexAttribPointer(normalLocation, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), nullptr);
        glEnableVertexAttribArray(normalLocation);
    }

	glBindVertexArray(0);
}

Icosahedron::~Icosahedron()
{
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vertices);
	glDeleteBuffers(1, &m_indices);
}

void Icosahedron::draw()
{
    draw(GL_TRIANGLES);
}

void Icosahedron::draw(const GLenum mode)
{
	glBindVertexArray(m_vao);
	glDrawElements(mode, m_size, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);
}

void Icosahedron::refine(
    std::vector<vec3> & vertices
,   std::vector<Face> & indices
,   const unsigned char levels)
{
    std::unordered_map<uint, GLushort> cache;

    for(int i = 0; i < levels; ++i)
    {
        const int size(static_cast<int>(indices.size()));

        for(int f = 0; f < size; ++f)
        {
            Face & face = indices[f];

            const GLushort a(face[0]);
            const GLushort b(face[1]);
            const GLushort c(face[2]);

            const GLushort ab(split(a, b, vertices, cache));
            const GLushort bc(split(b, c, vertices, cache));
            const GLushort ca(split(c, a, vertices, cache));

            face = {{ ab, bc, ca }};

            indices.emplace_back(Face{{ a, ab, ca }});
            indices.emplace_back(Face{{ b, bc, ab }});
            indices.emplace_back(Face{{ c, ca, bc }});
        }
    }
}

GLushort Icosahedron::split(
    const GLushort a
,   const GLushort b
,   std::vector<vec3> & points
,   std::unordered_map<uint, GLushort> & cache)
{
    const bool aSmaller(a < b);

    const uint smaller(aSmaller ? a : b);
    const uint greater(aSmaller ? b : a);
    const uint hash((smaller << 16) + greater);

    auto h(cache.find(hash));
    if(cache.end() != h)
        return h->second;

    points.push_back(normalize((points[a] + points[b]) * .5f));

    const GLushort i = static_cast<GLushort>(points.size() - 1);

    cache[hash] = i;

    return i;
}

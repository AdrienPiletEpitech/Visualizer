#include <GL/glew.h>

#pragma warning(push, 0)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#pragma warning(pop, 0)

#include <glutils.hpp>
#include <camera.hpp>
#include <renderer.hpp>
#include <Vertex.hpp>
#include <OBJLoader.hpp>



BEGIN_VISUALIZER_NAMESPACE

bool Renderer::Initialize()
{

    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals; // Won't be used at the moment.
    bool res = loadOBJ("../../res/palm.obj", vertices, uvs, normals);

    GL_CALL(glCreateBuffers, 1, &m_UBO);
    GL_CALL(glNamedBufferStorage, m_UBO, sizeof(glm::mat4), glm::value_ptr(m_Camera->GetViewProjectionMatrix()), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);

    GL_CALL(glCreateBuffers, 1, &m_VBO);
    GL_CALL(glNamedBufferStorage, m_VBO, sizeof(glm::vec3) * vertices.size(), vertices.data(), 0);

    GL_CALL(glCreateVertexArrays, 1, &m_VAO);
    GL_CALL(glBindVertexArray, m_VAO);

    GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, m_VBO);
    GL_CALL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, m_IBO);

    GL_CALL(glEnableVertexAttribArray, 0);
    GL_CALL(glVertexAttribPointer, 0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    GL_CALL(glEnableVertexAttribArray, 1);
    GL_CALL(glVertexAttribPointer, 1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<GLvoid*>(sizeof(glm::vec3)));




    GL_CALL(glBindVertexArray, 0);

    GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, 0);
    GL_CALL(glBindBuffer, GL_ELEMENT_ARRAY_BUFFER, 0);

    GL_CALL(glDisableVertexAttribArray, 0);
    GL_CALL(glDisableVertexAttribArray, 1);

    GLuint vShader = GL_CALL(glCreateShader, GL_VERTEX_SHADER);
    GLuint fShader = GL_CALL(glCreateShader, GL_FRAGMENT_SHADER);

    m_ShaderProgram = glCreateProgram();

    GL_CALL(glAttachShader, m_ShaderProgram, vShader);
    GL_CALL(glAttachShader, m_ShaderProgram, fShader);

    {
        char const* const vertexShader =
R"(#version 450 core

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inColor;

layout(location = 0) smooth out vec3 color;

layout(std140, binding = 0) uniform Matrix
{
    mat4 modelViewProjection;
};

void main()
{
    color = inColor;
    gl_Position = modelViewProjection*vec4(inWorldPos, 1.);
}
)";

        GL_CALL(glShaderSource, vShader, 1, &vertexShader, nullptr);

        GL_CALL(glCompileShader, vShader);

        {
            GLint length = 0;

            GL_CALL(glGetShaderiv, vShader, GL_INFO_LOG_LENGTH, &length);

            if (length > 1)
            {
                std::string log(length, '\0');

                GL_CALL(glGetShaderInfoLog, vShader, length, nullptr, log.data());

                std::cerr << "Vertex shader log:\n" << log << '\n';
            }
        }

        char const* const fragmentShader =
R"(#version 450 core

layout(location = 0) out vec4 outColor;

layout(location = 0) smooth in vec3 color;

void main()
{
    outColor = vec4(color, 1.0);
}
)";

        GL_CALL(glShaderSource, fShader, 1, &fragmentShader, nullptr);

        GL_CALL(glCompileShader, fShader);

        {
            GLint length = 0;

            GL_CALL(glGetShaderiv, fShader, GL_INFO_LOG_LENGTH, &length);

            if (length > 1)
            {
                std::string log(length, '\0');

                GL_CALL(glGetShaderInfoLog, fShader, length, nullptr, log.data());

                std::cerr << "Vertex shader log:\n" << log << '\n';
            }
        }
    }

    GL_CALL(glLinkProgram, m_ShaderProgram);

    {
        GLint length = 0;

        GL_CALL(glGetProgramiv, m_ShaderProgram, GL_INFO_LOG_LENGTH, &length);

        if (length > 1)
        {
            std::string log(length, '\0');

            GL_CALL(glGetProgramInfoLog, m_ShaderProgram, length, nullptr, log.data());

            std::cerr << "Shader program log:\n" << log << '\n';
        }
    }

    GL_CALL(glDetachShader, m_ShaderProgram, vShader);
    GL_CALL(glDetachShader, m_ShaderProgram, fShader);

    GL_CALL(glDeleteShader, vShader);
    GL_CALL(glDeleteShader, fShader);

    m_UBOData = GL_CALL_REINTERPRET_CAST_RETURN_VALUE(glm::mat4*, glMapNamedBufferRange, m_UBO, 0, sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);

    return true;
}

void Renderer::Render()
{
    GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GL_CALL(glUseProgram, m_ShaderProgram);

    GL_CALL(glBindBufferBase, GL_UNIFORM_BUFFER, 0, m_UBO);
    GL_CALL(glBindVertexArray, m_VAO);
    GL_CALL(glDrawArrays, GL_TRIANGLES, 0, vertices.size());
    GL_CALL(glBindVertexArray, 0);
    GL_CALL(glBindBufferBase, GL_UNIFORM_BUFFER, 0, 0);

    GL_CALL(glUseProgram, 0);
}

void Renderer::Cleanup()
{
    m_UBOData = nullptr;

    GL_CALL(glUnmapNamedBuffer, m_UBO);

    GL_CALL(glDeleteBuffers, 1, &m_VBO);
    GL_CALL(glDeleteBuffers, 1, &m_IBO);
    GL_CALL(glDeleteBuffers, 1, &m_UBO);

    GL_CALL(glDeleteVertexArrays, 1, &m_VAO);

    GL_CALL(glDeleteProgram, m_ShaderProgram);

}

void Renderer::UpdateViewport(uint32_t width, uint32_t height)
{
    m_ViewportWidth = width;
    m_ViewportHeight = height;

    glViewport(0, 0, m_ViewportWidth, m_ViewportHeight);
    m_Camera->ComputeProjection(m_ViewportWidth, m_ViewportHeight);

    std::memcpy(m_UBOData, glm::value_ptr(m_Camera->GetViewProjectionMatrix()), sizeof(glm::mat4));
    GL_CALL(glFlushMappedNamedBufferRange, m_UBO, 0, sizeof(glm::mat4));
}

void Renderer::UpdateCamera()
{
    std::memcpy(m_UBOData, glm::value_ptr(m_Camera->GetViewProjectionMatrix()), sizeof(glm::mat4));
    GL_CALL(glFlushMappedNamedBufferRange, m_UBO, 0, sizeof(glm::mat4));
}

END_VISUALIZER_NAMESPACE

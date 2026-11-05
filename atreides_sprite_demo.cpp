// atreides_walk_demo.cpp
// OpenGL 3.3 + GLFW + GLAD 2 + stb_image
// Displays the 12-frame WALK cycle from atreides_sprite_sheet.png.
// Keep this file, stb_image.h, and atreides_sprite_sheet.png in the same folder.

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

constexpr int WINDOW_W = 1000;
constexpr int WINDOW_H = 750;

GLFWwindow* window = nullptr;
GLuint program = 0, vao = 0, vbo = 0, ebo = 0, textureID = 0;
int imageW = 0, imageH = 0;
int frame = 0;
double lastFrameTime = 0.0;
bool isPlaying = true;
constexpr int FRAME_COUNT = 12;
constexpr double FPS = 10.0;

// Coordinates measured from the uploaded 2048 x 682 presentation sheet.
// WALK row: x=155..1019, y=340..489. There are 12 cells.
constexpr float SHEET_X = 155.0f;
constexpr float SHEET_Y = 340.0f;
constexpr float CELL_W = 72.0f;
constexpr float CELL_H = 149.0f;
constexpr float INSET = 3.0f;

const char* vertexSource = R"(
#version 330 core
layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aUV;
out vec2 uv;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    uv = aUV;
})";

const char* fragmentSource = R"(
#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D sprite;
void main() {
    color = texture(sprite, uv);
})";

GLuint compileShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    GLint ok = GL_FALSE;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::cerr << "Shader error:\n" << log << '\n';
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool createProgram() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vs || !fs) return false;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program link error:\n" << log << '\n';
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return ok == GL_TRUE;
}

bool loadTexture(const char* path) {
    stbi_set_flip_vertically_on_load(false);
    int channels = 0;
    unsigned char* pixels = stbi_load(path, &imageW, &imageH, &channels, 4);
    if (!pixels) {
        std::cerr << "Could not load " << path << ": "
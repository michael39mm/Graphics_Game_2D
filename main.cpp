#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Global variables
GLFWwindow* Window = nullptr;

int width = 800;
int height = 600;

GLuint ShaderProgram = 0;
GLuint VAO = 0;
GLuint VBO = 0;
GLuint EBO = 0;

// Vertex shader
const char* VertexShaderCode = R"(
#version 330 core

layout (location = 0) in vec3 Vpos;
layout (location = 1) in vec3 Vcolor;

out vec3 vertexColor;

void main() {
    gl_Position = vec4(Vpos, 1.0);
    vertexColor = Vcolor;
}
)";

// Fragment shader
const char* FragmentShaderCode = R"(
#version 330 core
in vec3 vertexColor;
out vec4 colors;

void main() {
    colors = vec4(vertexColor,1.0);
}
)";

// Window resize callback
void framebufferSizeCallback(
    GLFWwindow* window,
    int newWidth,
    int newHeight
) {
    width = newWidth;
    height = newHeight;

    glViewport(0, 0, width, height);
}

// Initialize GLFW, window, and GLAD
bool initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(width, height, "2D GAME", nullptr, nullptr);

    if (!Window) {
        std::cerr << "Window creation error\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(Window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(Window);
        Window = nullptr;
        glfwTerminate();
        return false;
    }

    glfwGetFramebufferSize(Window, &width, &height);
    glViewport(0, 0, width, height);

    glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);

    return true;
}

// Compile one shader
GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char log[512];

        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << '\n';

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Compile shaders, link program, and initialize VAO/VBO/EBO
bool pipeline() {
    // Compile shaders
    GLuint VertexShader =
        compileShader(GL_VERTEX_SHADER, VertexShaderCode);

    GLuint FragmentShader =
        compileShader(GL_FRAGMENT_SHADER, FragmentShaderCode);

    if (VertexShader == 0 || FragmentShader == 0) {
        if (VertexShader != 0)
            glDeleteShader(VertexShader);

        if (FragmentShader != 0)
            glDeleteShader(FragmentShader);

        return false;
    }

    // Link shaders
    ShaderProgram = glCreateProgram();

    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);

    GLint success;
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char log[512];

        glGetProgramInfoLog(ShaderProgram, 512, nullptr, log);
        std::cerr << "Shader link error:\n" << log << '\n';

        glDeleteShader(VertexShader);
        glDeleteShader(FragmentShader);
        glDeleteProgram(ShaderProgram);

        ShaderProgram = 0;
        return false;
    }

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    // Create VAO, VBO, and EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Reserve space for a maximum of 4 vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * 6 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    // Rectangle indices
    unsigned int indices[] = {
        0, 1, 2,
        2, 1, 3
    };

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
    );

    // Configure the vertex position attribute
    // Position attribute: x, y, z
glVertexAttribPointer(
    0,
    3,
    GL_FLOAT,
    GL_FALSE,
    6 * sizeof(float),
    (void*)0
);
glEnableVertexAttribArray(0);

// Color attribute: r, g, b
glVertexAttribPointer(
    1,
    3,
    GL_FLOAT,
    GL_FALSE,
    6 * sizeof(float),
    (void*)(3 * sizeof(float))
);
glEnableVertexAttribArray(1);

glBindVertexArray(0);

    return true;
}

// Draw a triangle
void drawTriangle(float r,float g,float b) {
    float vertices[] = {
        -0.8f, -0.5f, 0.0f,r,g,b,
        -0.2f, -0.5f, 0.0f,r,g,b,
        -0.5f,  0.3f, 0.0f,r,g,b
    };

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Replace the VBO's current vertex data
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    // Draw 3 vertices as one triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
}

// Draw a rectangle using its center position
void drawRectangle(float rectWidth, float rectHeight,
                   float posX, float posY,float r,float g,float b) {

    float left   = posX - rectWidth / 2.0f;
    float right  = posX + rectWidth / 2.0f;
    float top    = posY + rectHeight / 2.0f;
    float bottom = posY - rectHeight / 2.0f;

    float vertices[] = {
        left,  top,    0.0f,r,g,b,  
        left,  bottom, 0.0f, r,g,b, 
        right, top,    0.0f, r,g,b, 
        right, bottom, 0.0f ,r,g,b 
    };

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Replace the VBO's current vertex data
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    // Draw the rectangle using the 6 indices in the EBO
    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);
}

// Release resources
void cleanup() {
    if (EBO != 0)
        glDeleteBuffers(1, &EBO);

    if (VBO != 0)
        glDeleteBuffers(1, &VBO);

    if (VAO != 0)
        glDeleteVertexArrays(1, &VAO);

    if (ShaderProgram != 0)
        glDeleteProgram(ShaderProgram);

    if (Window != nullptr) {
        glfwDestroyWindow(Window);
        Window = nullptr;
    }

    glfwTerminate();
}



void drawRoadMarkings(float laneY)
{
    // Draw 8 small white dashes along the road
    for (int i = 0; i < 8; i++)
    {
        float x = -0.78f + i * 0.22f;

        drawRectangle(
            0.10f, 0.012f,  // Dash width and height
            x, laneY,       // Position
            0.90f, 0.90f, 0.90f
        );
    }
}



void drawCitySkyline()
{
    // Distant buildings: muted blue
    drawRectangle(0.16f, 0.25f, -0.82f, 0.84f,
                  0.35f, 0.48f, 0.66f);

    drawRectangle(0.12f, 0.38f, -0.63f, 0.90f,
                  0.40f, 0.52f, 0.68f);

    drawRectangle(0.18f, 0.30f, -0.43f, 0.86f,
                  0.43f, 0.53f, 0.70f);

    drawRectangle(0.13f, 0.44f, -0.21f, 0.93f,
                  0.36f, 0.47f, 0.63f);

    drawRectangle(0.20f, 0.28f,  0.03f, 0.85f,
                  0.44f, 0.54f, 0.70f);

    drawRectangle(0.14f, 0.36f,  0.25f, 0.89f,
                  0.36f, 0.48f, 0.65f);

    drawRectangle(0.18f, 0.25f,  0.47f, 0.84f,
                  0.43f, 0.53f, 0.69f);

    drawRectangle(0.14f, 0.42f,  0.68f, 0.92f,
                  0.34f, 0.45f, 0.62f);

    drawRectangle(0.18f, 0.30f,  0.88f, 0.86f,
                  0.42f, 0.51f, 0.66f);
}


void drawCityWindows()
{
    // Windows on the left buildings
    drawRectangle(0.025f, 0.035f, -0.82f, 0.85f,
                  1.0f, 0.85f, 0.35f);

    drawRectangle(0.025f, 0.035f, -0.82f, 0.91f,
                  1.0f, 0.85f, 0.35f);

    drawRectangle(0.025f, 0.035f, -0.63f, 0.88f,
                  1.0f, 0.85f, 0.35f);

    drawRectangle(0.025f, 0.035f, -0.63f, 0.94f,
                  1.0f, 0.85f, 0.35f);

    // Windows on the right buildings
    drawRectangle(0.025f, 0.035f, 0.68f, 0.88f,
                  1.0f, 0.85f, 0.35f);

    drawRectangle(0.025f, 0.035f, 0.68f, 0.94f,
                  1.0f, 0.85f, 0.35f);

    drawRectangle(0.025f, 0.035f, 0.88f, 0.85f,
                  1.0f, 0.85f, 0.35f);
}


int main() {
    if (!initialize()) {
        return -1;
    }

    if (!pipeline()) {
        cleanup();
        return -1;
    }

    // Render loop
    while (!glfwWindowShouldClose(Window)) {
        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        
        
        
        // ==========================================
        // 1. SKY
        // ==========================================
        drawRectangle(2.0f, 2.0f, 0.0f, 0.0f,
                      0.35f, 0.68f, 0.95f);


        // ==========================================
        // 2. CITY PAVEMENT
        // Covers everything below the skyline
        // ==========================================
        drawRectangle(2.0f, 1.70f, 0.0f, -0.15f,
                      0.43f, 0.45f, 0.46f);


        // ==========================================
        // 3. CITY SKYLINE AND WINDOWS
        // ==========================================
        drawCitySkyline();
        drawCityWindows();


        // ==========================================
        // 4. TOP SIDEWALK
        // Dark backing creates a raised curb effect
        // ==========================================
        drawRectangle(1.94f, 0.15f, 0.0f, 0.625f,
                      0.28f, 0.29f, 0.30f);

        drawRectangle(1.90f, 0.11f, 0.0f, 0.63f,
                      0.65f, 0.66f, 0.64f);

        // Thin highlight on top edge
        drawRectangle(1.88f, 0.012f, 0.0f, 0.675f,
                      0.78f, 0.79f, 0.76f);


        // ==========================================
        // 5. SHADOWS UNDER THE FOUR ROADS
        // Draw these BEFORE the asphalt
        // ==========================================
        drawRectangle(1.89f, 0.30f, 0.0f,  0.398f,
                      0.17f, 0.18f, 0.19f);

        drawRectangle(1.89f, 0.30f, 0.0f,  0.018f,
                      0.17f, 0.18f, 0.19f);

        drawRectangle(1.89f, 0.30f, 0.0f, -0.362f,
                      0.17f, 0.18f, 0.19f);

        drawRectangle(1.89f, 0.30f, 0.0f, -0.742f,
                      0.17f, 0.18f, 0.19f);


        // ==========================================
        // 6. ASPHALT SURFACES
        // ==========================================
        drawRectangle(1.85f, 0.28f, 0.0f,  0.41f,
                      0.25f, 0.27f, 0.30f);

        drawRectangle(1.85f, 0.28f, 0.0f,  0.03f,
                      0.25f, 0.27f, 0.30f);

        drawRectangle(1.85f, 0.28f, 0.0f, -0.35f,
                      0.25f, 0.27f, 0.30f);

        drawRectangle(1.85f, 0.28f, 0.0f, -0.73f,
                      0.25f, 0.27f, 0.30f);


        // ==========================================
        // 7. SUBTLE ROAD EDGE HIGHLIGHTS
        // ==========================================
        drawRectangle(1.82f, 0.012f, 0.0f,  0.535f,
                      0.34f, 0.36f, 0.39f);

        drawRectangle(1.82f, 0.012f, 0.0f,  0.155f,
                      0.34f, 0.36f, 0.39f);

        drawRectangle(1.82f, 0.012f, 0.0f, -0.225f,
                      0.34f, 0.36f, 0.39f);

        drawRectangle(1.82f, 0.012f, 0.0f, -0.605f,
                      0.34f, 0.36f, 0.39f);


        // ==========================================
        // 8. RAISED SIDEWALKS BETWEEN ROAD LANES
        // ==========================================

        // Sidewalk 1
        drawRectangle(1.94f, 0.085f, 0.0f,  0.22f,
                      0.28f, 0.29f, 0.30f);

        drawRectangle(1.90f, 0.055f, 0.0f,  0.22f,
                      0.63f, 0.64f, 0.62f);

        drawRectangle(1.88f, 0.010f, 0.0f,  0.243f,
                      0.78f, 0.79f, 0.76f);


        // Sidewalk 2
        drawRectangle(1.94f, 0.085f, 0.0f, -0.16f,
                      0.28f, 0.29f, 0.30f);

        drawRectangle(1.90f, 0.055f, 0.0f, -0.16f,
                      0.63f, 0.64f, 0.62f);

        drawRectangle(1.88f, 0.010f, 0.0f, -0.137f,
                      0.78f, 0.79f, 0.76f);


        // Sidewalk 3
        drawRectangle(1.94f, 0.085f, 0.0f, -0.54f,
                      0.28f, 0.29f, 0.30f);

        drawRectangle(1.90f, 0.055f, 0.0f, -0.54f,
                      0.63f, 0.64f, 0.62f);

        drawRectangle(1.88f, 0.010f, 0.0f, -0.517f,
                      0.78f, 0.79f, 0.76f);


        // ==========================================
        // 9. BOTTOM SIDEWALK
        // ==========================================
        drawRectangle(1.94f, 0.085f, 0.0f, -0.92f,
                      0.28f, 0.29f, 0.30f);

        drawRectangle(1.90f, 0.055f, 0.0f, -0.92f,
                      0.63f, 0.64f, 0.62f);

        drawRectangle(1.88f, 0.010f, 0.0f, -0.897f,
                      0.78f, 0.79f, 0.76f);


        // ==========================================
        // 10. ROAD MARKINGS
        // ==========================================
        drawRoadMarkings( 0.41f);
        drawRoadMarkings( 0.03f);
        drawRoadMarkings(-0.35f);
        drawRoadMarkings(-0.73f);



        
       


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    cleanup();

    return 0;
}
// quad_playground.cpp
// A tiny standalone program for experimenting with drawQuad.
// Same setup as your project (GLFW + GLAD, OpenGL 3.3 core).
// To run it: back up your main.cpp, then build this file in its place.

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

GLFWwindow* Window = nullptr;
int width = 800;
int height = 600;

GLuint ShaderProgram = 0, VAO = 0, VBO = 0, EBO = 0;

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

const char* FragmentShaderCode = R"(
#version 330 core
in vec3 vertexColor;
out vec4 colors;
void main() {
    colors = vec4(vertexColor, 1.0);
}
)";

void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

bool initialize() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    Window = glfwCreateWindow(width, height, "drawQuad playground", nullptr, nullptr);
    if (!Window) { glfwTerminate(); return false; }

    glfwMakeContextCurrent(Window);
    if (!gladLoadGL(glfwGetProcAddress)) return false;

    glfwGetFramebufferSize(Window, &width, &height);
    glViewport(0, 0, width, height);
    glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
    return true;
}

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Shader error:\n" << log << '\n';
        return 0;
    }
    return s;
}

bool pipeline() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, VertexShaderCode);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FragmentShaderCode);
    if (!vs || !fs) return false;

    ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, vs);
    glAttachShader(ShaderProgram, fs);
    glLinkProgram(ShaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    // Room for 4 vertices, each with 6 floats (x, y, z, r, g, b)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, 70 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    // The two triangles every quad is made of:
    //   (0,1,2) = top-left, bottom-left, top-right
    //   (2,1,3) = top-right, bottom-left, bottom-right
    unsigned int indices[] = { 0, 1, 2,
                               2, 1, 3 };
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return true;
}

// ---------------------------------------------------------------
// YOUR drawQuad (unchanged).
//
//   topY and bottomY are shared by two corners each, so the top and
//   bottom edges are always horizontal. Each of the four corners has
//   its OWN x, which is what lets the sides slant.
//
//        topLeftX ------------ topRightX          <- topY
//           /                      \
//   bottomLeftX ------------------ bottomRightX   <- bottomY
// ---------------------------------------------------------------
void drawQuad(
    float topLeftX, float topLeftY,
    float topRightX, float topRightY,
    float bottomRightX, float bottomRightY,
    float bottomLeftX, float bottomLeftY,
    float r, float g, float b
) {
    float vertices[] = {
        // Position                         // Color
        topLeftX,     topLeftY,             r, g, b,
        bottomLeftX,  bottomLeftY,          r, g, b,
        bottomRightX, bottomRightY,         r, g, b,

        topLeftX,     topLeftY,             r, g, b,
        bottomRightX, bottomRightY,         r, g, b,
        topRightX,    topRightY,            r, g, b
    };

    unsigned int VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(vertices),
        vertices,
        GL_STATIC_DRAW
    );

    // Position attribute: 2 floats (x, y)
    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    // Color attribute: 3 floats (r, g, b)
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        5 * sizeof(float),
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    // Draw the two triangles forming the quadrilateral
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

void drawRectangle(
    float left, float top,
    float right, float bottom,
    float r, float g, float b
) {
    float vertices[] = {
        // x       y        z       r    g    b
        left,     top,     0.0f,   r,   g,   b,  // Top-left
        left,     bottom,  0.0f,   r,   g,   b,  // Bottom-left
        right,    top,     0.0f,   r,   g,   b,  // Top-right
        right,    bottom,  0.0f,   r,   g,   b   // Bottom-right
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glDrawElements(
        GL_TRIANGLES,
        6,
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);
}


void drawTriangle(float posx, float posy, float posz,
                  float width, float height,
                  float r, float g, float b) {

    float vertices[] = {
        // Position                                      // Color
        posx - width / 2, posy,          posz,            r, g, b,
        posx + width / 2, posy,          posz,            r, g, b,
        posx,             posy + height, posz,            r, g, b
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices
    );

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);
}

// ===================== SHAPE HELPERS =====================
void drawFan(const float* v, int count) {
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * 6 * sizeof(float), v);
    glDrawArrays(GL_TRIANGLE_FAN, 0, count);
    glBindVertexArray(0);
}

void setV(float* v, int i, float x, float y, float r, float g, float b) {
    v[i*6] = x; v[i*6+1] = y; v[i*6+2] = 0.0f;
    v[i*6+3] = r; v[i*6+4] = g; v[i*6+5] = b;
}

// rx, ry are in "height units" like drawCircle, so rx == ry gives a round circle
void drawEllipse(float cx, float cy, float rx, float ry, float r, float g, float b) {
    float asp = (float)height / (float)width;
    const int N = 32;
    float v[(N + 2) * 6];
    setV(v, 0, cx, cy, r, g, b);
    for (int i = 0; i <= N; i++) {
        float a = 6.2831853f * i / N;
        setV(v, i + 1, cx + cosf(a) * rx * asp, cy + sinf(a) * ry, r, g, b);
    }
    drawFan(v, N + 2);
}

// Outline given as {x0,y0, x1,y1, ...} in order around the shape (max 64 points)
void drawPolygon(const float* xy, int n, float r, float g, float b) {
    if (n > 64) n = 64;
    float v[(64 + 2) * 6];
    float cx = 0, cy = 0;
    for (int i = 0; i < n; i++) { cx += xy[2*i]; cy += xy[2*i + 1]; }
    cx /= n; cy /= n;
    setV(v, 0, cx, cy, r, g, b);
    for (int i = 0; i <= n; i++)
        setV(v, i + 1, xy[2*(i % n)], xy[2*(i % n) + 1], r, g, b);
    drawFan(v, n + 2);
}


void drawPoly(float x1, float y1, float x2, float y2,
              float x3, float y3, float x4, float y4,
              float r, float g, float b) {
    float v[] = {
        x1, y1, 0.0f, r, g, b,
        x2, y2, 0.0f, r, g, b,
        x3, y3, 0.0f, r, g, b,
        x4, y4, 0.0f, r, g, b
    };
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// Thick line with an even thickness on screen, whatever the window shape
void drawLine(float x1, float y1, float x2, float y2, float thick,
              float r, float g, float b) {
    float asp = (float)height / (float)width;
    float dx = (x2 - x1) / asp, dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.00001f) return;
    float nx = -dy / len * thick * 0.5f * asp, ny = dx / len * thick * 0.5f;
    drawPoly(x1 + nx, y1 + ny, x2 + nx, y2 + ny,
             x2 - nx, y2 - ny, x1 - nx, y1 - ny, r, g, b);
}

// ===================== MODEL-SPACE LAYER =====================
// Everything below takes coordinates from your vertex sheet.
float gOx = 0, gOy = 0, gS = 1, gFlip = 1, gDmg = 0;

float MX(float x) { return gOx + gFlip * x * gS * ((float)height / (float)width); }
float MY(float y) { return gOy + y * gS; }

// Push a color toward red: gDmg 0 = normal, 1 = fully red
void tint(float& r, float& g, float& b) {
    r = r + (1 - r) * gDmg;
    g = g * (1 - 0.8f * gDmg);
    b = b * (1 - 0.8f * gDmg);
}

void mPoly(const float* xy, int n, float r, float g, float b) {
    if (n > 64) n = 64;
    float p[128];
    for (int i = 0; i < n; i++) { p[2*i] = MX(xy[2*i]); p[2*i+1] = MY(xy[2*i+1]); }
    tint(r, g, b);
    drawPolygon(p, n, r, g, b);
}

// Dark copy slightly bigger underneath = cartoon outline
void mPolyOutlined(const float* xy, int n, float r, float g, float b) {
    if (n > 64) n = 64;
    float cx = 0, cy = 0;
    for (int i = 0; i < n; i++) { cx += xy[2*i]; cy += xy[2*i+1]; }
    cx /= n; cy /= n;
    float big[128];
    for (int i = 0; i < n; i++) {
        big[2*i]   = cx + (xy[2*i]   - cx) * 1.07f;
        big[2*i+1] = cy + (xy[2*i+1] - cy) * 1.07f;
    }
    mPoly(big, n, 0.12f, 0.06f, 0.06f);
    mPoly(xy, n, r, g, b);
}

void mEllipse(float cx, float cy, float rx, float ry, float r, float g, float b) {
    tint(r, g, b);
    drawEllipse(MX(cx), MY(cy), rx * gS, ry * gS, r, g, b);
}

void mLine(float x1, float y1, float x2, float y2, float thick,
           float r, float g, float b) {
    tint(r, g, b);
    drawLine(MX(x1), MY(y1), MX(x2), MY(y2), thick * gS, r, g, b);
}

// ===================== THE BOXER (step 1: head and face) =====================
// ox = x position, feetY = where his feet touch the ground, s = scale (about 0.17 fits a lane),
// dir = +1 faces right / -1 faces left, dmg = 0..1 red tint
void drawBoxer(float ox, float feetY, float s, float dir, float dmg) {
    gS = s;
    gOx = ox;
    gOy = feetY + 0.95f * s;              // the lowest point on the sheet is y = -0.95
    gFlip = (dir > 0) ? -1.0f : 1.0f;     // the sheet faces LEFT, so flip it to face right
    gDmg = dmg;

    // Head outline: points 1,3,5,7,8,9,17,19, a chin point I added, then 18,16,6,4,2
    float head[] = {  0.00f,0.82f,  0.30f,0.76f,  0.45f,0.60f,  0.50f,0.42f,  0.55f,0.30f,
                      0.52f,0.18f,  0.40f,0.12f,  0.38f,0.06f,  0.00f,0.02f, -0.38f,0.06f,
                     -0.44f,0.12f, -0.50f,0.42f, -0.45f,0.60f, -0.30f,0.76f };
    mPolyOutlined(head, 14, 0.96f, 0.70f, 0.52f);

    // Scalp shine
    mEllipse(-0.15f, 0.68f, 0.12f, 0.055f, 1.00f, 0.84f, 0.70f);

    // Ear (right side of the sheet): dark rim, orange, inner shade
    mEllipse(0.47f, 0.25f, 0.085f, 0.115f, 0.12f, 0.06f, 0.06f);
    mEllipse(0.47f, 0.25f, 0.072f, 0.100f, 0.93f, 0.55f, 0.35f);
    mEllipse(0.47f, 0.25f, 0.035f, 0.055f, 0.85f, 0.42f, 0.26f);

    // Eye 1: dark rim, white, pupil looking left, shine
    mEllipse(-0.22f, 0.32f, 0.088f, 0.098f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.22f, 0.32f, 0.075f, 0.085f, 1.00f, 1.00f, 1.00f);
    mEllipse(-0.245f, 0.32f, 0.045f, 0.060f, 0.08f, 0.05f, 0.05f);
    mEllipse(-0.26f, 0.345f, 0.014f, 0.016f, 1.00f, 1.00f, 1.00f);

    // Eye 2 (a little smaller)
    mEllipse(0.03f, 0.33f, 0.078f, 0.088f, 0.12f, 0.06f, 0.06f);
    mEllipse(0.03f, 0.33f, 0.066f, 0.076f, 1.00f, 1.00f, 1.00f);
    mEllipse(0.005f, 0.33f, 0.040f, 0.052f, 0.08f, 0.05f, 0.05f);
    mEllipse(-0.008f, 0.352f, 0.012f, 0.014f, 1.00f, 1.00f, 1.00f);

    // Nose
    mEllipse(-0.17f, 0.21f, 0.085f, 0.072f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.17f, 0.21f, 0.072f, 0.060f, 0.98f, 0.74f, 0.56f);

    // Angry brows (V shape: outer ends are sheet points 10 and 11)
    mLine(-0.34f, 0.50f, -0.05f, 0.39f, 0.085f, 0.12f, 0.06f, 0.06f);
    mLine( 0.13f, 0.51f, -0.02f, 0.40f, 0.085f, 0.12f, 0.06f, 0.06f);

    // Smirk: two segments
    mLine(-0.12f, 0.115f, -0.02f, 0.085f, 0.028f, 0.12f, 0.06f, 0.06f);
    mLine(-0.02f, 0.085f,  0.09f, 0.125f, 0.028f, 0.12f, 0.06f, 0.06f);
}


// ===================== THE SENTINEL (original design) =====================
// Paste this below drawBoxer() in quad_playground.cpp (or your main.cpp).
// Needs: mPoly, mPolyOutlined, mEllipse, mLine and the globals gOx, gOy, gS, gFlip, gDmg.
//
// Same signature as drawBoxer:
//   ox = x position, feetY = ground line, s = scale (~0.17 fits a lane),
//   dir = +1 faces right / -1 faces left, dmg = 0..1 red tint
//
// Model space: feet at y = -0.95, antenna tip at about y = 1.0, sheet faces LEFT.

// ===================== THE SENTINEL v2 (original hero design) =====================
// Replaces the first drawSentinel(): delete the old one, paste this below drawBoxer().
// Needs: mPoly, mPolyOutlined, mEllipse, mLine and the globals gOx, gOy, gS, gFlip, gDmg.
//
//   ox = x position, feetY = ground line, s = scale (~0.17 fits a lane),
//   dir = +1 faces right / -1 faces left, dmg = 0..1 red tint
//
// Model space: feet at y = -0.95, helmet fin tip at about y = 0.97, sheet faces LEFT.
// Hero cues: broad V chest, big shoulder pads, cape, strong jaw, confident smirk,
// one fist on the hip, boots with silver cuffs, diamond emblem on chest and helmet.

void drawSkyblade(float ox, float baseY, float s, float dir, float dmg, float t) {
    float bob = sinf(t * 2.5f) * 0.025f;
    gS = s;
    gOx = ox;
    gOy = baseY + 0.95f * s + bob;
    gFlip = (dir > 0) ? -1.0f : 1.0f;
    gDmg = dmg;
 
    // Palette
    const float viR = 0.45f, viG = 0.22f, viB = 0.85f;    // violet armor
    const float dvR = 0.22f, dvG = 0.10f, dvB = 0.48f;    // deep violet
    const float whR = 0.96f, whG = 0.96f, whB = 1.00f;    // white trim / cape
    const float amR = 1.00f, amG = 0.72f, amB = 0.12f;    // amber energy
    const float cyR = 0.20f, cyG = 0.85f, cyB = 0.95f;    // cyan eyes
    const float skR = 0.90f, skG = 0.68f, skB = 0.52f;    // skin
 
    float w  = sinf(t * 3.0f) * 0.06f;
    float w2 = sinf(t * 3.0f + 1.2f) * 0.09f;
    float pulse = 0.5f + 0.5f * sinf(t * 5.0f);
 
    // ---------- FLIGHT: glow ring under him + wind streaks ----------
    mEllipse(0.0f, -1.02f, 0.30f + 0.05f * pulse, 0.05f, 1.00f, 0.85f, 0.45f);
    mEllipse(0.0f, -1.02f, 0.18f + 0.03f * pulse, 0.03f, amR, amG, amB);
    mLine(0.95f + w2, 0.35f, 1.45f + w2, 0.35f, 0.020f, 0.85f, 0.90f, 1.00f);
    mLine(1.00f + w,  0.10f, 1.60f + w,  0.10f, 0.020f, 0.85f, 0.90f, 1.00f);
    mLine(0.90f + w2, -0.15f, 1.35f + w2, -0.15f, 0.020f, 0.85f, 0.90f, 1.00f);
 
    // ---------- CAPE (big, rippling) ----------
    float cape[] = { 0.14f,0.06f,  0.44f,0.04f,  0.70f+w,-0.15f,  0.95f+w2,-0.42f,
                     0.88f+w,-0.72f,  0.55f+w2,-0.62f,  0.30f,-0.40f,  0.14f,-0.30f };
    mPolyOutlined(cape, 8, whR, whG, whB);
    float lining[] = { 0.20f,0.00f,  0.42f,-0.02f,  0.66f+w,-0.18f,  0.86f+w2,-0.42f,
                       0.78f+w,-0.62f,  0.52f+w2,-0.54f,  0.30f,-0.34f };
    mPoly(lining, 7, viR, viG, viB);
 
    // ---------- BACK ARM (swept back, balancing the flight pose) ----------
    mLine(0.48f, -0.02f, 0.62f, -0.20f, 0.15f, dvR, dvG, dvB);
    mLine(0.62f, -0.20f, 0.70f, -0.42f, 0.14f, dvR, dvG, dvB);
    mEllipse(0.71f, -0.46f, 0.09f, 0.085f, whR, whG, whB);
 
    // ---------- LEGS (one straight, one bent back = mid-flight) ----------
    float legL[] = { -0.20f,-0.42f, -0.02f,-0.42f, -0.05f,-0.72f, -0.22f,-0.72f };
    float legR[] = {  0.02f,-0.42f,  0.22f,-0.42f,  0.34f,-0.62f,  0.18f,-0.70f };
    mPolyOutlined(legL, 4, viR, viG, viB);
    mPolyOutlined(legR, 4, viR, viG, viB);
    float bootL[] = { -0.26f,-0.70f, -0.03f,-0.70f, -0.03f,-0.92f, -0.36f,-0.92f, -0.32f,-0.80f };
    float bootR[] = {  0.16f,-0.66f,  0.36f,-0.60f,  0.46f,-0.78f,  0.30f,-0.84f };
    mPolyOutlined(bootL, 5, dvR, dvG, dvB);
    mPolyOutlined(bootR, 4, dvR, dvG, dvB);
    float cuffL[] = { -0.27f,-0.70f, -0.02f,-0.70f, -0.02f,-0.76f, -0.27f,-0.76f };
    mPoly(cuffL, 4, whR, whG, whB);
    mEllipse(-0.13f, -0.55f, 0.07f, 0.05f, whR, whG, whB);   // knee guard
 
    // ---------- TORSO (broad V) ----------
    float torso[] = { -0.44f,0.04f, 0.44f,0.04f, 0.22f,-0.46f, -0.22f,-0.46f };
    mPolyOutlined(torso, 4, viR, viG, viB);
    mEllipse(-0.19f, -0.02f, 0.14f, 0.065f, 0.62f, 0.45f, 1.00f);   // chest shine
    mEllipse( 0.19f, -0.02f, 0.14f, 0.065f, 0.62f, 0.45f, 1.00f);
 
    // amber diamond emblem
    float emb[]   = { 0.00f,-0.03f, 0.10f,-0.19f, 0.00f,-0.36f, -0.10f,-0.19f };
    float embIn[] = { 0.00f,-0.10f, 0.045f,-0.19f, 0.00f,-0.28f, -0.045f,-0.19f };
    mPolyOutlined(emb, 4, amR, amG, amB);
    mPoly(embIn, 4, whR, whG, whB);
 
    // belt
    float belt[] = { -0.24f,-0.38f, 0.24f,-0.38f, 0.22f,-0.48f, -0.22f,-0.48f };
    mPoly(belt, 4, whR, whG, whB);
    mEllipse(0.0f, -0.43f, 0.04f, 0.04f, amR, amG, amB);
 
    // ---------- FRONT ARM + WEAPON (energy blade, raised forward) ----------
    mLine(-0.48f, -0.02f, -0.66f, -0.14f, 0.15f, viR, viG, viB);   // upper arm
    mLine(-0.66f, -0.14f, -0.66f,  0.04f, 0.14f, viR, viG, viB);   // forearm
    mEllipse(-0.66f, -0.05f, 0.08f, 0.07f, whR, whG, whB);          // gauntlet
 
    // hilt + guard
    mLine(-0.66f, 0.06f, -0.72f, 0.20f, 0.045f, dvR, dvG, dvB);
    mLine(-0.78f, 0.15f, -0.64f, 0.25f, 0.035f, amR, amG, amB);     // cross-guard
    // blade glow (pulses) + bright core
    mLine(-0.72f, 0.22f, -1.06f, 0.82f, 0.085f + 0.02f * pulse, 1.00f, 0.82f, 0.40f);
    mLine(-0.72f, 0.22f, -1.06f, 0.82f, 0.045f, amR, amG, amB);
    mLine(-0.73f, 0.24f, -1.04f, 0.79f, 0.018f, 1.00f, 1.00f, 1.00f);
    // fist wrapped on the hilt
    mEllipse(-0.67f, 0.06f, 0.095f, 0.085f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.67f, 0.06f, 0.082f, 0.072f, whR, whG, whB);
 
    // ---------- SHOULDER PADS ----------
    mEllipse(-0.48f, 0.01f, 0.165f, 0.125f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.48f, 0.01f, 0.145f, 0.108f, whR, whG, whB);
    mEllipse(-0.48f, 0.01f, 0.055f, 0.042f, amR, amG, amB);
    mEllipse( 0.48f, 0.01f, 0.165f, 0.125f, 0.12f, 0.06f, 0.06f);
    mEllipse( 0.48f, 0.01f, 0.145f, 0.108f, whR, whG, whB);
    mEllipse( 0.48f, 0.01f, 0.055f, 0.042f, amR, amG, amB);
 
    // ---------- NECK ----------
    float neck[] = { -0.13f,0.12f, 0.13f,0.12f, 0.13f,0.02f, -0.13f,0.02f };
    mPoly(neck, 4, skR * 0.9f, skG * 0.85f, skB * 0.85f);
 
    // ---------- WINGED HELMET ----------
    // side wings (the flight cue), drawn behind the helmet
    float wingL[] = { -0.42f,0.50f, -0.66f,0.80f, -0.62f,0.60f, -0.44f,0.36f };
    float wingR[] = {  0.42f,0.50f,  0.66f,0.80f,  0.62f,0.60f,  0.44f,0.36f };
    mPolyOutlined(wingL, 4, whR, whG, whB);
    mPolyOutlined(wingR, 4, whR, whG, whB);
 
    float helmet[] = { -0.46f,0.40f, -0.42f,0.66f, -0.22f,0.84f, 0.22f,0.84f,
                        0.42f,0.66f,  0.46f,0.40f,  0.40f,0.20f, -0.40f,0.20f };
    mPolyOutlined(helmet, 8, viR, viG, viB);
    mEllipse(-0.22f, 0.74f, 0.10f, 0.04f, 0.70f, 0.58f, 1.00f);    // helmet shine
    // crest stripe
    float crest[] = { -0.05f,0.84f, 0.05f,0.84f, 0.04f,0.66f, -0.04f,0.66f };
    mPoly(crest, 4, whR, whG, whB);
 
    // ---------- FACE ----------
    float face[] = { -0.36f,0.52f, 0.36f,0.52f, 0.34f,0.28f, 0.20f,0.10f,
                     -0.20f,0.10f, -0.34f,0.28f };
    mPolyOutlined(face, 6, skR, skG, skB);
    float band[] = { -0.42f,0.60f, 0.42f,0.60f, 0.40f,0.54f, -0.40f,0.54f };
    mPoly(band, 4, whR, whG, whB);
    // forehead gem
    mEllipse(0.0f, 0.57f, 0.035f, 0.030f, amR, amG, amB);
 
    // eyes (bright, determined)
    mEllipse(-0.18f, 0.38f, 0.080f, 0.082f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.18f, 0.38f, 0.068f, 0.070f, 1.00f, 1.00f, 1.00f);
    mEllipse(-0.205f, 0.38f, 0.045f, 0.055f, cyR, cyG, cyB);
    mEllipse(-0.215f, 0.38f, 0.025f, 0.034f, 0.05f, 0.05f, 0.10f);
    mEllipse(-0.225f, 0.40f, 0.012f, 0.014f, 1.00f, 1.00f, 1.00f);
 
    mEllipse(0.08f, 0.38f, 0.072f, 0.076f, 0.12f, 0.06f, 0.06f);
    mEllipse(0.08f, 0.38f, 0.060f, 0.064f, 1.00f, 1.00f, 1.00f);
    mEllipse(0.055f, 0.38f, 0.040f, 0.050f, cyR, cyG, cyB);
    mEllipse(0.045f, 0.38f, 0.022f, 0.030f, 0.05f, 0.05f, 0.10f);
    mEllipse(0.035f, 0.40f, 0.011f, 0.013f, 1.00f, 1.00f, 1.00f);
 
    // brows, nose, grin
    mLine(-0.30f, 0.50f, -0.08f, 0.455f, 0.060f, 0.30f, 0.15f, 0.08f);
    mLine( 0.00f, 0.455f,  0.20f, 0.50f, 0.060f, 0.30f, 0.15f, 0.08f);
    mEllipse(-0.08f, 0.27f, 0.030f, 0.025f, 0.82f, 0.55f, 0.40f);
    mLine(-0.17f, 0.17f, -0.03f, 0.145f, 0.026f, 0.35f, 0.12f, 0.10f);
    mLine(-0.03f, 0.145f, 0.11f, 0.18f, 0.026f, 0.35f, 0.12f, 0.10f);
}
 
// ---- Usage in your render loop (t is already defined in main: float t = glfwGetTime()) ----
// drawSkyblade(0.5f, -0.1f, 0.5f, -1.0f, 0.0f, t);   // hovering, facing left
// drawSkyblade(0.5f, -0.1f, 0.5f, -1.0f, 0.6f, t);   // taking damage



void drawSentinel(float ox, float feetY, float s, float dir, float dmg) {
    gS = s;
    gOx = ox;
    gOy = feetY + 0.95f * s;
    gFlip = (dir > 0) ? -1.0f : 1.0f;
    gDmg = dmg;

    // Palette
    const float blR = 0.15f, blG = 0.36f, blB = 0.82f;    // hero blue
    const float dbR = 0.07f, dbG = 0.17f, dbB = 0.45f;    // deep blue (shade)
    const float siR = 0.82f, siG = 0.87f, siB = 0.93f;    // silver trim
    const float cyR = 0.20f, cyG = 0.85f, cyB = 0.95f;    // cyan accent
    const float skR = 0.96f, skG = 0.76f, skB = 0.58f;    // skin
    const float cpR = 0.10f, cpG = 0.62f, cpB = 0.78f;    // cape

    // ---------- CAPE (behind everything, trails away from the facing side) ----------
    float cape[] = { 0.15f,0.04f,  0.42f,0.02f,  0.64f,-0.28f,  0.70f,-0.62f,
                     0.42f,-0.70f, 0.30f,-0.55f, 0.15f,-0.40f };
    mPolyOutlined(cape, 7, cpR, cpG, cpB);
    float capeIn[] = { 0.20f,-0.02f, 0.40f,-0.05f, 0.56f,-0.30f, 0.60f,-0.55f, 0.38f,-0.58f, 0.22f,-0.35f };
    mPoly(capeIn, 6, 0.06f, 0.42f, 0.60f);

    // ---------- BACK ARM (fist on hip) ----------
    mLine(0.50f, -0.04f, 0.60f, -0.26f, 0.15f, dbR, dbG, dbB);
    mLine(0.60f, -0.26f, 0.36f, -0.38f, 0.14f, dbR, dbG, dbB);
    mEllipse(0.33f, -0.39f, 0.10f, 0.09f, siR, siG, siB);

    // ---------- LEGS (strong, planted) ----------
    float legL[] = { -0.25f,-0.42f, -0.04f,-0.42f, -0.04f,-0.70f, -0.26f,-0.70f };
    float legR[] = {  0.05f,-0.42f,  0.26f,-0.42f,  0.27f,-0.70f,  0.05f,-0.70f };
    mPolyOutlined(legL, 4, blR, blG, blB);
    mPolyOutlined(legR, 4, blR, blG, blB);

    float bootL[] = { -0.27f,-0.68f, -0.03f,-0.68f, -0.03f,-0.95f, -0.40f,-0.95f, -0.36f,-0.82f };
    float bootR[] = {  0.05f,-0.68f,  0.28f,-0.68f,  0.28f,-0.95f, -0.02f,-0.95f,  0.02f,-0.82f };
    mPolyOutlined(bootL, 5, dbR, dbG, dbB);
    mPolyOutlined(bootR, 5, dbR, dbG, dbB);
    // silver boot cuffs
    float cuffL[] = { -0.28f,-0.68f, -0.02f,-0.68f, -0.02f,-0.75f, -0.28f,-0.75f };
    float cuffR[] = {  0.04f,-0.68f,  0.29f,-0.68f,  0.29f,-0.75f,  0.04f,-0.75f };
    mPoly(cuffL, 4, siR, siG, siB);
    mPoly(cuffR, 4, siR, siG, siB);
    // knee guards
    mEllipse(-0.145f, -0.54f, 0.075f, 0.05f, siR, siG, siB);
    mEllipse( 0.160f, -0.54f, 0.075f, 0.05f, siR, siG, siB);

    // ---------- TORSO (broad V taper) ----------
    float torso[] = { -0.46f,0.04f, 0.46f,0.04f, 0.24f,-0.46f, -0.24f,-0.46f };
    mPolyOutlined(torso, 4, blR, blG, blB);

    // chest highlight (pecs) for a muscular read
    mEllipse(-0.20f, -0.02f, 0.15f, 0.07f, 0.30f, 0.52f, 0.95f);
    mEllipse( 0.20f, -0.02f, 0.15f, 0.07f, 0.30f, 0.52f, 0.95f);

    // diamond emblem
    float emb[]   = { 0.00f,-0.03f, 0.16f,-0.19f, 0.00f,-0.35f, -0.16f,-0.19f };
    float embIn[] = { 0.00f,-0.10f, 0.08f,-0.19f, 0.00f,-0.28f, -0.08f,-0.19f };
    mPolyOutlined(emb, 4, siR, siG, siB);
    mPoly(embIn, 4, cyR, cyG, cyB);

    // belt with buckle
    float belt[] = { -0.25f,-0.38f, 0.25f,-0.38f, 0.24f,-0.48f, -0.24f,-0.48f };
    mPoly(belt, 4, siR, siG, siB);
    float buck[] = { 0.00f,-0.385f, 0.06f,-0.43f, 0.00f,-0.475f, -0.06f,-0.43f };
    mPoly(buck, 4, cyR, cyG, cyB);

    // ---------- FRONT ARM (raised, clenched fist) ----------
    mLine(-0.50f, -0.04f, -0.64f, -0.26f, 0.15f, blR, blG, blB);   // upper arm
    mLine(-0.64f, -0.26f, -0.60f, -0.48f, 0.14f, blR, blG, blB);   // forearm
    mEllipse(-0.615f, -0.40f, 0.085f, 0.075f, siR, siG, siB);      // gauntlet
    mEllipse(-0.60f, -0.54f, 0.115f, 0.105f, 0.12f, 0.06f, 0.06f); // fist rim
    mEllipse(-0.60f, -0.54f, 0.100f, 0.090f, siR, siG, siB);       // fist
    mEllipse(-0.60f, -0.54f, 0.045f, 0.040f, cyR, cyG, cyB);       // knuckle gem

    // ---------- BIG SHOULDER PADS ----------
    mEllipse(-0.50f, 0.01f, 0.165f, 0.125f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.50f, 0.01f, 0.145f, 0.108f, siR, siG, siB);
    mEllipse(-0.50f, 0.01f, 0.060f, 0.045f, cyR, cyG, cyB);
    mEllipse( 0.50f, 0.01f, 0.165f, 0.125f, 0.12f, 0.06f, 0.06f);
    mEllipse( 0.50f, 0.01f, 0.145f, 0.108f, siR, siG, siB);
    mEllipse( 0.50f, 0.01f, 0.060f, 0.045f, cyR, cyG, cyB);

    // ---------- NECK ----------
    float neck[] = { -0.13f,0.12f, 0.13f,0.12f, 0.13f,0.02f, -0.13f,0.02f };
    mPoly(neck, 4, skR * 0.9f, skG * 0.85f, skB * 0.85f);

    // ---------- HELMET ----------
    float helmet[] = { -0.46f,0.40f, -0.42f,0.66f, -0.22f,0.84f, 0.22f,0.84f,
                        0.42f,0.66f,  0.46f,0.40f,  0.40f,0.20f, -0.40f,0.20f };
    mPolyOutlined(helmet, 8, blR, blG, blB);

    // helmet fin (sweeps back)
    float fin[] = { 0.00f,0.83f, 0.20f,0.83f, 0.46f,0.95f, 0.12f,0.98f };
    mPolyOutlined(fin, 4, siR, siG, siB);

    // helmet shine
    mEllipse(-0.22f, 0.74f, 0.10f, 0.04f, 0.55f, 0.72f, 1.00f);

    // ---------- FACE (open, strong jaw) ----------
    float face[] = { -0.36f,0.52f, 0.36f,0.52f, 0.34f,0.28f, 0.20f,0.10f,
                     -0.20f,0.10f, -0.34f,0.28f };
    mPolyOutlined(face, 6, skR, skG, skB);

    // silver brow band over the face
    float band[] = { -0.42f,0.60f, 0.42f,0.60f, 0.40f,0.54f, -0.40f,0.54f };
    mPoly(band, 4, siR, siG, siB);

    // forehead emblem
    float fe[] = { 0.00f,0.82f, 0.07f,0.73f, 0.00f,0.64f, -0.07f,0.73f };
    mPolyOutlined(fe, 4, cyR, cyG, cyB);

    // eyes: big, bright, looking toward the front, slightly narrowed = determined
    mEllipse(-0.18f, 0.38f, 0.080f, 0.082f, 0.12f, 0.06f, 0.06f);
    mEllipse(-0.18f, 0.38f, 0.068f, 0.070f, 1.00f, 1.00f, 1.00f);
    mEllipse(-0.205f, 0.38f, 0.045f, 0.055f, 0.15f, 0.55f, 0.95f);
    mEllipse(-0.215f, 0.38f, 0.025f, 0.034f, 0.05f, 0.05f, 0.10f);
    mEllipse(-0.225f, 0.40f, 0.012f, 0.014f, 1.00f, 1.00f, 1.00f);

    mEllipse(0.08f, 0.38f, 0.072f, 0.076f, 0.12f, 0.06f, 0.06f);
    mEllipse(0.08f, 0.38f, 0.060f, 0.064f, 1.00f, 1.00f, 1.00f);
    mEllipse(0.055f, 0.38f, 0.040f, 0.050f, 0.15f, 0.55f, 0.95f);
    mEllipse(0.045f, 0.38f, 0.022f, 0.030f, 0.05f, 0.05f, 0.10f);
    mEllipse(0.035f, 0.40f, 0.011f, 0.013f, 1.00f, 1.00f, 1.00f);

    // brows: a mild V = focused and brave, not angry
    mLine(-0.30f, 0.50f, -0.08f, 0.455f, 0.060f, 0.30f, 0.15f, 0.08f);
    mLine( 0.00f, 0.455f,  0.20f, 0.50f, 0.060f, 0.30f, 0.15f, 0.08f);

    // nose
    mEllipse(-0.08f, 0.27f, 0.030f, 0.025f, 0.85f, 0.58f, 0.42f);

    // confident smirk (lifts on the front side)
    mLine(-0.17f, 0.17f, -0.03f, 0.145f, 0.026f, 0.35f, 0.12f, 0.10f);
    mLine(-0.03f, 0.145f, 0.11f, 0.175f, 0.026f, 0.35f, 0.12f, 0.10f);

    // helmet ear guard
    mEllipse(0.43f, 0.34f, 0.05f, 0.08f, siR, siG, siB);
}

// ---- Usage inside your render loop (after the background) ----
// drawSentinel(0.5f, -0.6f, 0.5f, -1.0f, 0.0f);   // facing left, normal
// drawSentinel(0.5f, -0.6f, 0.5f, -1.0f, 0.6f);   // taking damage (red tint)

// ---- Usage inside your render loop (after the background, in place of or beside the boxer) ----
// drawSentinel(0.5f, -0.6f, 0.5f, -1.0f, 0.0f);          // normal, facing left
// drawSentinel(0.5f, -0.6f, 0.5f, -1.0f, 0.6f);          // taking damage


void cleanup() {
    if (EBO) glDeleteBuffers(1, &EBO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (ShaderProgram) glDeleteProgram(ShaderProgram);
    if (Window) glfwDestroyWindow(Window);
    glfwTerminate();
}
#include <cmath>

#include <cmath>

void drawCircle(float centerX, float centerY, float radius,
                float r, float g, float b)
{
    const int segments = 100;
    const float PI = 3.14159265f;

    float vertices[(segments + 2) * 6];

    // Center vertex: x, y, z, r, g, b
    vertices[0] = centerX;
    vertices[1] = centerY;
    vertices[2] = 0.0f;
    vertices[3] = r;
    vertices[4] = g;
    vertices[5] = b;

    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * PI * i / segments;

        int index = (i + 1) * 6;

        vertices[index]     = centerX + radius * cos(angle);
        vertices[index + 1] = centerY + radius * sin(angle);
        vertices[index + 2] = 0.0f;
        vertices[index + 3] = r;
        vertices[index + 4] = g;
        vertices[index + 5] = b;
    }

    GLuint circleVAO, circleVBO;

    glGenVertexArrays(1, &circleVAO);
    glGenBuffers(1, &circleVBO);

    glBindVertexArray(circleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, circleVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_FAN, 0, segments + 2);

    glBindVertexArray(0);

    glDeleteBuffers(1, &circleVBO);
    glDeleteVertexArrays(1, &circleVAO);
}

int main() {
    if (!initialize()) return -1;
    if (!pipeline()) { cleanup(); return -1; }

    while (!glfwWindowShouldClose(Window)) {
        float t = (float)glfwGetTime();

        glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(ShaderProgram);

        // ======================= TOP ROW: four basic shapes =======================
        //            topLeftX topRightX bottomLeftX bottomRightX topY bottomY  color

        // 1) Rectangle: left X's match, right X's match
        // Top section
// ====================== ARCHITECTURAL FACADE ======================

// ORANGE ROOF
// LEFT ROOF
drawSkyblade(0.5f, -0.1f, 0.5f, -1.0f, 0.6f, t); 


        // 9) Uncomment to see what goes wrong when the left and right X's are swapped.
        //    The triangles flip over and you get a "bow-tie".
        // drawQuad( 0.30f, -0.10f,   0.30f, -0.10f,   -0.35f, -0.85f,   1.0f, 1.0f, 1.0f);

        // ======================= THINGS TO TRY =======================
        //  - Make shape 1 a square by changing its X's and Y's.
        //  - Swap the two bottom X's of shape 2. What happens?
        //  - Make shape 6 lean the other way (use -lean).
        //  - Put a quad's topY BELOW its bottomY. What happens?
        //  - Draw a road in perspective: a very wide quad at the bottom, narrow at the top.

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    cleanup();
    return 0;
}

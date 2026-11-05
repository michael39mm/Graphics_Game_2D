// pilot_demo.cpp
// OpenGL 3.3 + GLFW + GLAD 2 + stb_image
// el atlas byegy men build_atlas.py (pilot_atlas.png)
//
// el controls:
//   A / D aw el aswhom : mashy
//   Shift + A / D      : gary
//   S aw el sahm el t7t: crouch
//   Space              : noo2a (jump)
//   J                  : ytl3 nar (ranged attack)
//   Esc                : 2fel

#define GLAD_GL_IMPLEMENTATION   // 3shan el GLAD bta3ak header-only (shilha lw 3andak gl.c)
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <vector>
#include <string>

constexpr int WINDOW_W = 1000;
constexpr int WINDOW_H = 750;

// el atlas: 8 columns x 6 rows, kol frame fe cell 181 x 181, kol animation fe row
constexpr int CELL_W = 181;
constexpr int CELL_H = 181;
constexpr float ANCHOR_X = 94.0f;   // makan el rgleen gowa el cell (el python by3melha)
constexpr float ANCHOR_Y = 159.0f;
constexpr float SCALE = 2.2f;       // takbeer el sprite
constexpr float GROUND_Y = 600.0f;  // makan el ard (pixels men fo2)

GLFWwindow* window = nullptr;
GLuint program = 0, vao = 0, vbo = 0, textureID = 0;
int imageW = 0, imageH = 0;

// uniforms
GLint uScreenLoc, uPosLoc, uSizeLoc, uUVLoc, uUseTexLoc, uColorLoc;

enum State { IDLE, WALK, RUN, CROUCH, JUMP, SHOOT };

struct Anim {
    const char* name;
    int row;        // el row fel atlas
    int frames;     // 3add el frames
    double fps;
    bool loop;
};

// kol animation 8 frames fel sheet el gdeeda
const Anim anims[] = {
    { "IDLE",   0, 8,  8.0, true  },
    { "WALK",   1, 8, 11.0, true  },
    { "RUN",    2, 8, 15.0, true  },
    { "CROUCH", 3, 8, 12.0, false },
    { "JUMP",   4, 8, 10.0, false },
    { "SHOOT",  5, 8, 12.0, false },
};

// a5er pixel solid (el rgleen) fe kol row, 3shan el rgleen tfdal 3al ard 7ata lw el rows mo5talfa shwaya
const float FOOT_Y[] = { 158.0f, 158.0f, 157.0f, 159.0f, 158.0f, 158.0f };

State state = IDLE;
double animTime = 0.0;
bool facingRight = true;
float posX = WINDOW_W * 0.5f;
float jumpT = 0.0f;                    // men 0 l 1
constexpr float JUMP_TIME = 0.8f;      // el noo2a ta5od kam sanya
constexpr float JUMP_HEIGHT = 30.0f;   // sora fiha noo2a asln, fa da ezafy
constexpr float WALK_SPEED = 130.0f;
constexpr float RUN_SPEED = 280.0f;

bool jumpPressed = false, attackPressed = false;

// el tl2a (projectile) bt3mal fel code msh fel sheet
struct Bullet { float x, y, vx; float life; };
std::vector<Bullet> bullets;
bool fired = false;                      // 3shan netl3 tl2a wa7da bs fe kol attack
constexpr int   FIRE_FRAME = 4;          // el frame elly fiha el flash (men 0)
constexpr float MUZZLE_DX = 45.0f;       // makan el mawsoura men el rgleen (bel sprite pixels)
constexpr float MUZZLE_DY = 77.0f;       // ertefa3 el mawsoura foo2 el ard fel sora (3ala el sprite)
constexpr float BULLET_SPEED = 800.0f;

const char* vertexSource = R"(
#version 330 core
layout(location=0) in vec2 aCorner;   // 0..1
uniform vec2 uScreen;
uniform vec2 uPos;     // top-left bel pixels
uniform vec2 uSize;    // el 7agm bel pixels
uniform vec4 uUV;      // u0, v0, u1, v1
out vec2 uv;
void main() {
    vec2 p = uPos + aCorner * uSize;
    vec2 ndc = vec2(p.x / uScreen.x * 2.0 - 1.0, 1.0 - p.y / uScreen.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    uv = vec2(mix(uUV.x, uUV.z, aCorner.x), mix(uUV.y, uUV.w, aCorner.y));
})";

const char* fragmentSource = R"(
#version 330 core
in vec2 uv;
out vec4 color;
uniform sampler2D sprite;
uniform int uUseTex;
uniform vec4 uColor;
void main() {
    if (uUseTex == 1) color = texture(sprite, uv);
    else color = uColor;
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

    uScreenLoc = glGetUniformLocation(program, "uScreen");
    uPosLoc    = glGetUniformLocation(program, "uPos");
    uSizeLoc   = glGetUniformLocation(program, "uSize");
    uUVLoc     = glGetUniformLocation(program, "uUV");
    uUseTexLoc = glGetUniformLocation(program, "uUseTex");
    uColorLoc  = glGetUniformLocation(program, "uColor");
    return ok == GL_TRUE;
}

bool loadTexture(const char* path) {
    stbi_set_flip_vertically_on_load(false);   // el row 0 fo2, zay el sora
    int channels = 0;
    unsigned char* pixels = stbi_load(path, &imageW, &imageH, &channels, 4);
    if (!pixels) {
        std::cerr << "Could not load " << path << ": " << stbi_failure_reason() << '\n';
        return false;
    }
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageW, imageH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // NEAREST 3shan el sprite yb2a 7ad w mayt3melsh blur 3ala el 7awaf
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    stbi_image_free(pixels);
    return true;
}

void createQuad() {
    // quad wa7ed (0..1) bnrsmo kol mara b uniforms mo5talefa
    float corners[] = { 0,0,  1,0,  0,1,  1,1 };
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(corners), corners, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void setState(State s) {
    if (s == state) return;
    state = s;
    animTime = 0.0;   // nbda el animation men el awel
}

void keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, GLFW_TRUE);
    if (key == GLFW_KEY_SPACE) jumpPressed = true;
    if (key == GLFW_KEY_J) attackPressed = true;
}

bool down(int a, int b = -1) {
    return glfwGetKey(window, a) == GLFW_PRESS || (b != -1 && glfwGetKey(window, b) == GLFW_PRESS);
}

void update(double dt) {
    bool left  = down(GLFW_KEY_A, GLFW_KEY_LEFT);
    bool right = down(GLFW_KEY_D, GLFW_KEY_RIGHT);
    bool crouch = down(GLFW_KEY_S, GLFW_KEY_DOWN);
    bool shift = down(GLFW_KEY_LEFT_SHIFT, GLFW_KEY_RIGHT_SHIFT);
    int dir = (right ? 1 : 0) - (left ? 1 : 0);

    animTime += dt;

    // n7arek el tl2at w nms7 elly kharg el shasha
    for (auto& b : bullets) { b.x += b.vx * (float)dt; b.life -= (float)dt; }
    for (size_t i = bullets.size(); i-- > 0;)
        if (bullets[i].life <= 0.0f || bullets[i].x < -100 || bullets[i].x > WINDOW_W + 100)
            bullets.erase(bullets.begin() + i);

    // el melee wel jump locked l7ad ma yblsho
    if (state == SHOOT) {
        // lma neosal l frame el flash netl3 tl2a (mara wa7da)
        if (!fired && (int)(animTime * anims[SHOOT].fps) >= FIRE_FRAME) {
            fired = true;
            float d = facingRight ? 1.0f : -1.0f;
            bullets.push_back({ posX + d * MUZZLE_DX * SCALE, GROUND_Y - MUZZLE_DY * SCALE, d * BULLET_SPEED, 2.0f });
        }
        if (animTime * anims[SHOOT].fps >= anims[SHOOT].frames) setState(IDLE);
        jumpPressed = attackPressed = false;
        return;
    }
    if (state == JUMP) {
        jumpT += (float)dt / JUMP_TIME;
        if (dir != 0) { facingRight = dir > 0; posX += dir * WALK_SPEED * (float)dt; }
        if (jumpT >= 1.0f) { jumpT = 0.0f; setState(IDLE); }
        jumpPressed = attackPressed = false;
        return;
    }

    if (attackPressed) { fired = false; setState(SHOOT); attackPressed = jumpPressed = false; return; }
    if (jumpPressed)   { jumpT = 0.0f; setState(JUMP); attackPressed = jumpPressed = false; return; }

    if (crouch) {
        setState(CROUCH);
    } else if (dir != 0) {
        facingRight = dir > 0;
        setState(shift ? RUN : WALK);
        posX += dir * (shift ? RUN_SPEED : WALK_SPEED) * (float)dt;
    } else {
        setState(IDLE);
    }

    // lw 5aragna men el shasha nrg3o men el gehet el tanya
    if (posX < -100) posX = WINDOW_W + 100;
    if (posX > WINDOW_W + 100) posX = -100;
}

void drawQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1) {
    glUniform2f(uPosLoc, x, y);
    glUniform2f(uSizeLoc, w, h);
    glUniform4f(uUVLoc, u0, v0, u1, v1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void render() {
    glClearColor(0.07f, 0.05f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(vao);
    glUniform2f(uScreenLoc, (float)WINDOW_W, (float)WINDOW_H);

    // el ard
    glUniform1i(uUseTexLoc, 0);
    glUniform4f(uColorLoc, 0.22f, 0.15f, 0.09f, 1.0f);
    drawQuad(0, GROUND_Y, (float)WINDOW_W, WINDOW_H - GROUND_Y, 0, 0, 1, 1);

    // el frame el 7aly
    const Anim& a = anims[state];
    int frame;
    if (state == JUMP) {
        // el frame bta3 el noo2a byt7arak 7asab el wa2t fel hawa
        frame = (int)(jumpT * a.frames);
        if (frame >= a.frames) frame = a.frames - 1;
    } else {
        frame = (int)(animTime * a.fps);
        if (a.loop) frame %= a.frames;
        else if (frame >= a.frames) frame = a.frames - 1;   // nesbet 3and a5er frame
    }

    float u0 = (float)(frame * CELL_W) / imageW;
    float u1 = (float)((frame + 1) * CELL_W) / imageW;
    float v0 = (float)(a.row * CELL_H) / imageH;
    float v1 = (float)((a.row + 1) * CELL_H) / imageH;
    if (!facingRight) { float t = u0; u0 = u1; u1 = t; }   // flip lw rayeh shimal

    float lift = 0.0f;
    if (state == JUMP) lift = 4.0f * JUMP_HEIGHT * jumpT * (1.0f - jumpT);

    float w = CELL_W * SCALE, h = CELL_H * SCALE;
    // el rgleen (anchor) lazem tkon 3ala el ard
    float x = posX - ANCHOR_X * SCALE;
    float y = GROUND_Y - (FOOT_Y[a.row] + 1.0f) * SCALE - lift;   // el rgleen 3al ard 7asab el row

    // el zel 3al ard (byso8ar lma yenoo2)
    glUniform1i(uUseTexLoc, 0);
    glUniform4f(uColorLoc, 0.0f, 0.0f, 0.0f, 1.0f);
    float shadowW = 90.0f * (1.0f - 0.4f * (lift / JUMP_HEIGHT));
    drawQuad(posX - shadowW * 0.5f, GROUND_Y - 2.0f, shadowW, 6.0f, 0, 0, 1, 1);

    glUniform1i(uUseTexLoc, 1);
    glBindTexture(GL_TEXTURE_2D, textureID);
    drawQuad(x, y, w, h, u0, v0, u1, v1);

    // el tl2at: zeel (trail) w galb sa5en
    glUniform1i(uUseTexLoc, 0);
    for (const auto& b : bullets) {
        float d = b.vx > 0 ? 1.0f : -1.0f;
        glUniform4f(uColorLoc, 0.2f, 0.75f, 1.0f, 0.45f);
        drawQuad(d > 0 ? b.x - 80.0f : b.x, b.y - 6.0f, 80.0f, 12.0f, 0, 0, 1, 1);
        glUniform4f(uColorLoc, 0.45f, 0.9f, 1.0f, 1.0f);
        drawQuad(b.x - 14.0f, b.y - 9.0f, 28.0f, 18.0f, 0, 0, 1, 1);
        glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        drawQuad(b.x - 7.0f, b.y - 4.0f, 14.0f, 8.0f, 0, 0, 1, 1);
    }
}

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "Pilot", nullptr, nullptr);
    if (!window) { std::cerr << "Window failed\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, keyCallback);

    if (!gladLoadGL(glfwGetProcAddress)) { std::cerr << "GLAD failed\n"; return 1; }
    if (!createProgram()) return 1;
    if (!loadTexture("pilot_atlas.png")) return 1;
    createQuad();

    // lazem el blending yeshta3'al 3shan el background el shafaf
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "sprite"), 0);

    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        double dt = now - last;
        last = now;
        if (dt > 0.05) dt = 0.05;   // 3shan mayt3amalsh jump kbeer lw el window etwa2ef

        glfwPollEvents();
        update(dt);
        render();

        std::string title = std::string("Pilot - ") + anims[state].name;
        glfwSetWindowTitle(window, title.c_str());
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}

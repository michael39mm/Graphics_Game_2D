#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>

// Lazem el define da yeb2a fe file wa7ed bas (3ashan el stb yetkteb hena)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// el wa2t elly 3ada men a5er frame (bel sec)
float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

// Global variables
GLFWwindow* Window = nullptr;

int width = 800;
int height = 600;

GLuint ShaderProgram = 0;
GLuint VAO = 0;
GLuint VBO = 0;
GLuint EBO = 0;

// Makan el offset w el redAmount fel shader
GLint OffsetLocation = -1;
GLint RedLocation = -1;

// Amaken el lanes (nos kol shar3 men fo2 le ta7t)
float LANE_Y[4] = {0.41f, 0.03f, -0.35f, -0.73f};
float LANE_HEIGHT = 0.28f;
float LANE_LEFT = -0.925f;
float LANE_RIGHT = 0.925f;

// Kol lane selected wala la2
bool selectedLanes[4] = {false, false, false, false};

// Anwa3 el players
const int PLAYER_MELEE   = 0;
const int PLAYER_RANGED  = 1;
const int PLAYER_DEFENSE = 2;

// Anwa3 el enemies
const int ENEMY_EASY   = 0;  // upper ranged (lazem duck)
const int ENEMY_MEDIUM = 1;  // lower ranged (lazem jump)
const int ENEMY_HARD   = 2;  // melee

// Kart el player
struct Player {
    int type;
    int lane;
    float x;
    int health;
    int maxHealth;
    float attackTimer;
    float jumpTimer;  // fadel kam sanya fel natta (0 = mesh nate)
    float duckTimer;  // fadel kam sanya fel duck
    int animState;    // el animation elly shaghala
    float animTime;   // men emta bada2t
    bool attacked;    // darab fel frame da
};

// Kart el enemy
struct Enemy {
    int type;
    int lane;
    float x;
    int health;
    int maxHealth;
    float attackTimer;
    int animState;
    float animTime;
    bool attacked;
};

std::vector<Player> players;
std::vector<Enemy> enemies;

// Sor3et el 7araka (kam unit fel sanya)
float PLAYER_SPEED     = 0.12f;
float ENEMY_SPEED      = 0.08f;
float HARD_ENEMY_SPEED = 0.14f;  // el hard bygry

// El natta w el duck
float JUMP_TIME   = 0.6f;
float DUCK_TIME   = 0.6f;
float JUMP_HEIGHT = 0.08f;

// Law el masafa benhom a2al men keda yeb2o "5abato" f ba3d
float TOUCH_DISTANCE = 0.07f;

// ===== El waves =====
int WAVE_SIZE[3] = {3, 5, 10};
float WAVE_SPAWN_TIME[3] = {5.0f, 4.0f, 3.0f};
float WAVE_BREAK_TIME = 4.0f;

int currentWave = 0;
int enemiesSpawnedInWave = 0;
float waveSpawnTimer = 3.0f;
float waveBreakTimer = 0.0f;

// ===== El banner bta3 el waves =====
std::string bannerTitle = "";
std::string bannerSubtitle = "";
float bannerTimer = 0.0f;
float BANNER_TIME = 3.0f;

// ===== 7alet el game =====
const int STATE_PLAYING  = 0;
const int STATE_WON      = 1;
const int STATE_LOST     = 2;
const int STATE_MENU     = 3;
const int STATE_PAUSED   = 4;
const int STATE_CUTSCENE = 5;

int gameState = STATE_MENU;

// Zarayer el menus (kolohom fel nos, el y bta3 kol wa7ed)
float END_BUTTON_Y[2] = {-0.1f, -0.27f};
float MAIN_MENU_BUTTON_Y[2] = {-0.05f, -0.23f};
float PAUSE_BUTTON_Y[3] = {0.02f, -0.15f, -0.32f};
float BUTTON_W = 0.56f;
float BUTTON_H = 0.12f;

// ===== Coins w el cards =====
// El coins elly bnebda2 beeha (30 = 3 melee bel zabt) - 500 lel testing bas
int START_COINS = 500;
int coins = START_COINS;

int PLAYER_COST[3] = {10, 20, 30};  // melee, ranged, defense
int ENEMY_DROP[3]  = {10, 15, 20};  // easy, medium, hard

// Awel player lazem yeb2a melee - ba3d keda kol el cards maftoo7a law el coins kfaya
bool firstMeleePlaced = false;

// Dosna 3ala card el defense w mestaneyeen click 3ala makan fel lane
bool placingDefense = false;

struct Coin {
    int lane;
    float x;
    int value;
};

std::vector<Coin> droppedCoins;

// Amaken el cards fo2 3al shmal
float CARD_X[3] = {-0.82f, -0.62f, -0.42f};
float CARD_Y = 0.86f;
float CARD_W = 0.16f;
float CARD_H = 0.22f;

// Anwa3 el talakat (men anhy ertefa3)
const int SHOT_MIDDLE = 0;  // talka el player el ranged
const int SHOT_HIGH   = 1;  // men fo2 (lazem duck)
const int SHOT_LOW    = 2;  // men ta7t (lazem jump)

struct Projectile {
    int lane;
    float x;
    float speed;      // + yemeen, - shmal
    bool fromPlayer;
    int height;
    bool hit;
};

std::vector<Projectile> projectiles;

float PROJECTILE_SPEED   = 0.5f;
float MELEE_ATTACK_TIME  = 1.0f;
float RANGED_ATTACK_TIME = 1.5f;
float ENEMY_SHOOT_TIME   = 2.0f;


// Shader el rectangles
const char* VertexShaderCode = R"(
#version 330 core
layout (location = 0) in vec3 Vpos;
layout (location = 1) in vec3 Vcolor;
out vec3 vertexColor;
uniform vec2 offset;
void main() {
    gl_Position = vec4(Vpos.xy + offset, Vpos.z, 1.0);
    vertexColor = Vcolor;
}
)";

const char* FragmentShaderCode = R"(
#version 330 core
in vec3 vertexColor;
out vec4 colors;
uniform float redAmount;
void main() {
    colors = vec4(mix(vertexColor, vec3(1.0, 0.0, 0.0), redAmount), 1.0);
}
)";

// Shader el sprites (el soora badal el lon)
const char* TexVertexShaderCode = R"(
#version 330 core
layout (location = 0) in vec2 Vpos;
layout (location = 1) in vec2 Vuv;
out vec2 uv;
void main() {
    gl_Position = vec4(Vpos, 0.0, 1.0);
    uv = Vuv;
}
)";

const char* TexFragmentShaderCode = R"(
#version 330 core
in vec2 uv;
out vec4 colors;
uniform sampler2D atlas;
uniform float redAmount;   // el darb
uniform float greyAmount;  // el card el locked
void main() {
    vec4 c = texture(atlas, uv);
    float grey = dot(c.rgb, vec3(0.3, 0.59, 0.11)) * 0.7;
    vec3 rgb = mix(c.rgb, vec3(grey), greyAmount);
    rgb = mix(rgb, vec3(1.0, 0.0, 0.0) * c.a, redAmount);
    colors = vec4(rgb, c.a);
}
)";

// ==========================================
// El sprites (el animations men el atlases)
// ==========================================

// El row bta3 kol animation gowa el atlas
const int ANIM_IDLE   = 0;
const int ANIM_WALK   = 1;
const int ANIM_RUN    = 2;
const int ANIM_CROUCH = 3;
const int ANIM_JUMP   = 4;
const int ANIM_ATTACK = 5;
const int ANIM_FRAMES = 8;

const int SPRITE_ATREIDES  = 0;  // player melee
const int SPRITE_FREMEN    = 1;  // player ranged
const int SPRITE_SARDAUKAR = 2;  // player defense
const int SPRITE_PILOT     = 3;  // enemy easy
const int SPRITE_BRUTE     = 4;  // enemy medium
const int SPRITE_OFFICER   = 5;  // enemy hard
const int SPRITE_COUNT     = 6;

struct SpriteInfo {
    const char* file;
    int cellW;          // 3ard el frame (bel pixels)
    int cellH;          // tool el frame
    float anchorX;      // makan el rgleen gowa el frame (x)
    float footY[6];     // makan el rgleen gowa el frame (y) le kol animation
    float fps[6];       // sor3et kol animation
    float muzzleDY;     // ertefa3 el mawsoura foo2 el rgleen (lel talakat)
    GLuint texture;
    int imageW;
    int imageH;
};

// (player = el type, enemy = 3 + el type)
SpriteInfo sprites[SPRITE_COUNT] = {
    { "assets/atreides_atlas.png",  181, 181, 95.0f,  {160, 160, 160, 160, 160, 160}, {6, 14, 18, 18, 10, 14}, 0.0f,  0, 0, 0 },
    { "assets/fremen_atlas.png",    181, 181, 91.0f,  {160, 160, 160, 160, 160, 160}, {6, 13, 18, 18, 10, 12}, 80.0f, 0, 0, 0 },
    { "assets/sardaukar_atlas.png", 204, 181, 102.0f, {150, 150, 150, 150, 150, 150}, {6, 10, 10, 10, 10, 10}, 0.0f,  0, 0, 0 },
    { "assets/pilot_atlas.png",     181, 181, 94.0f,  {158, 158, 157, 159, 158, 158}, {8, 15, 18, 18, 10, 12}, 77.0f, 0, 0, 0 },
    { "assets/brute2_atlas.png",    181, 181, 96.0f,  {161, 158, 156, 159, 160, 158}, {6, 11, 14, 14, 9, 9},   32.0f, 0, 0, 0 },
    { "assets/officer_atlas.png",   201, 181, 98.0f,  {158, 158, 155, 157, 157, 158}, {6, 12, 16, 18, 10, 12}, 0.0f,  0, 0, 0 },
};

float SPRITE_SCALE = 0.6f;       // fel game
float CARD_SPRITE_SCALE = 0.4f;  // gowa el cards
float FEET_OFFSET = 0.13f;       // el rgleen ta7t nos el lane be kam

GLuint TexProgram = 0;
GLuint TexVAO = 0;
GLuint TexVBO = 0;
GLint TexRedLocation = -1;
GLint TexGreyLocation = -1;

// Nload el atlas men el file (bnegarrab el path men aktar men makan)
void loadSprite(SpriteInfo& s) {
    const char* prefixes[] = { "", "../", "../../", "../../../" };
    unsigned char* pixels = nullptr;
    int channels = 0;

    for (int i = 0; i < 4 && pixels == nullptr; i++) {
        std::string path = std::string(prefixes[i]) + s.file;
        pixels = stbi_load(path.c_str(), &s.imageW, &s.imageH, &channels, 4);
    }

    if (pixels == nullptr) {
        std::cerr << "Could not load " << s.file << '\n';
        return;
    }

    // Narsom el rgb fel alpha 3ashan el 7awaf mayb2ash feeha lon waser
    for (int i = 0; i < s.imageW * s.imageH; i++) {
        unsigned char* p = pixels + i * 4;
        p[0] = (unsigned char)(p[0] * p[3] / 255);
        p[1] = (unsigned char)(p[1] * p[3] / 255);
        p[2] = (unsigned char)(p[2] * p[3] / 255);
    }

    glGenTextures(1, &s.texture);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s.imageW, s.imageH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
}

// Anhy frame (el idle/walk/run byelef, w el ba2y yo2af 3and a5er frame)
int spriteFrame(int sprite, int anim, float animTime) {
    int frame = (int)(animTime * sprites[sprite].fps[anim]);

    if (anim == ANIM_IDLE || anim == ANIM_WALK || anim == ANIM_RUN)
        return frame % ANIM_FRAMES;

    if (frame >= ANIM_FRAMES)
        frame = ANIM_FRAMES - 1;

    return frame;
}

// Nersem frame wa7da men el atlas, w el rgleen 3ala (feetX, feetY)
void drawSprite(int sprite, int anim, int frame, float feetX, float feetY,
                bool faceRight, float red, float grey, float scale) {
    SpriteInfo& s = sprites[sprite];

    // Anhy gozaa men el atlas (row = el animation, column = el frame)
    float u0 = (float)(frame * s.cellW) / s.imageW;
    float u1 = (float)((frame + 1) * s.cellW) / s.imageW;
    float v0 = (float)(anim * s.cellH) / s.imageH;
    float v1 = (float)((anim + 1) * s.cellH) / s.imageH;

    // Law rayeh shmal: nel2eb el soora
    float anchorX = s.anchorX;
    if (!faceRight) {
        float t = u0;
        u0 = u1;
        u1 = t;
        anchorX = s.cellW - s.anchorX;
    }

    // Men pixels le coordinates el shasha (el sprite beyekbar ma3 el window)
    float pxX = scale * height / 600.0f * 2.0f / width;
    float pxY = scale * 2.0f / 600.0f;

    float left   = feetX - anchorX * pxX;
    float right  = left + s.cellW * pxX;
    float bottom = feetY - (s.cellH - (s.footY[anim] + 1.0f)) * pxY;
    float top    = bottom + s.cellH * pxY;

    float vertices[] = {
        left,  top,    u0, v0,
        left,  bottom, u0, v1,
        right, top,    u1, v0,
        right, bottom, u1, v1
    };

    glUseProgram(TexProgram);
    glUniform1f(TexRedLocation, red);
    glUniform1f(TexGreyLocation, grey);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(TexVAO);
    glBindBuffer(GL_ARRAY_BUFFER, TexVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Nraga3 kol 7aga lel rectangles
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ShaderProgram);
}

// ==========================================
// El mouse
// ==========================================

// Makan el mouse b OpenGL (-1 le 1)
void getMousePosition(float* x, float* y) {
    double mouseX, mouseY;
    glfwGetCursorPos(Window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(Window, &windowWidth, &windowHeight);

    *x = (float)(mouseX / windowWidth) * 2.0f - 1.0f;
    *y = 1.0f - (float)(mouseY / windowHeight) * 2.0f;
}

// El mouse 3ala anhy lane (-1 law mafeesh)
int getMouseLane() {
    float x, y;
    getMousePosition(&x, &y);

    if (x < LANE_LEFT || x > LANE_RIGHT)
        return -1;

    for (int i = 0; i < 4; i++) {
        if (y <= LANE_Y[i] + LANE_HEIGHT / 2.0f && y >= LANE_Y[i] - LANE_HEIGHT / 2.0f)
            return i;
    }

    return -1;
}

// El mouse 3ala anhy card (-1 law mafeesh)
int cardAt(float x, float y) {
    for (int i = 0; i < 3; i++) {
        if (x >= CARD_X[i] - CARD_W / 2.0f && x <= CARD_X[i] + CARD_W / 2.0f &&
            y >= CARD_Y - CARD_H / 2.0f && y <= CARD_Y + CARD_H / 2.0f)
            return i;
    }

    return -1;
}

// El mouse 3ala anhy zorar (-1 law mafeesh)
int buttonAt(float x, float y, float* buttonY, int count) {
    for (int i = 0; i < count; i++) {
        if (x >= -BUTTON_W / 2.0f && x <= BUTTON_W / 2.0f &&
            y >= buttonY[i] - BUTTON_H / 2.0f && y <= buttonY[i] + BUTTON_H / 2.0f)
            return i;
    }

    return -1;
}

void buyPlayers(int type);
void placeDefense(int lane, float x);
void restartGame();
void startGame();

// Lama nedoos click
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    float mx, my;
    getMousePosition(&mx, &my);

    // Fel main menu: Start aw Exit
    if (gameState == STATE_MENU) {
        int b = buttonAt(mx, my, MAIN_MENU_BUTTON_Y, 2);
        if (b == 0)
            startGame();
        if (b == 1)
            glfwSetWindowShouldClose(Window, true);
        return;
    }

    // Fel pause: Continue, Restart, Main Menu
    if (gameState == STATE_PAUSED) {
        int b = buttonAt(mx, my, PAUSE_BUTTON_Y, 3);
        if (b == 0)
            gameState = STATE_PLAYING;
        if (b == 1)
            restartGame();
        if (b == 2) {
            restartGame();
            gameState = STATE_MENU;
        }
        return;
    }

    // El game 5eles: Restart aw Main Menu
    if (gameState == STATE_WON || gameState == STATE_LOST) {
        int b = buttonAt(mx, my, END_BUTTON_Y, 2);
        if (b == 0)
            restartGame();
        if (b == 1) {
            restartGame();
            gameState = STATE_MENU;
        }
        return;
    }

    if (gameState != STATE_PLAYING)
        return;

    int card = cardAt(mx, my);
    if (card != -1) {
        buyPlayers(card);
        return;
    }

    int lane = getMouseLane();

    // Mestaneyeen makan el defense: el click da makano
    if (placingDefense) {
        if (lane != -1)
            placeDefense(lane, mx);
        placingDefense = false;
        return;
    }

    // Click 3ala lane: selected <-> mesh selected
    if (lane != -1)
        selectedLanes[lane] = !selectedLanes[lane];
}

// W/S = natta w duck | Esc = pause | Space = skip el cutscene
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    if (gameState == STATE_CUTSCENE) {
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_ENTER || key == GLFW_KEY_ESCAPE)
            restartGame();
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        if (gameState == STATE_PLAYING)
            gameState = STATE_PAUSED;
        else if (gameState == STATE_PAUSED)
            gameState = STATE_PLAYING;
        return;
    }

    if (gameState != STATE_PLAYING)
        return;

    bool jumpKey = (key == GLFW_KEY_W || key == GLFW_KEY_UP);
    bool duckKey = (key == GLFW_KEY_S || key == GLFW_KEY_DOWN);

    for (int i = 0; i < (int)players.size(); i++) {
        // El defense mabyenotsh w mabye3melsh duck
        if (players[i].type == PLAYER_DEFENSE || !selectedLanes[players[i].lane])
            continue;

        // Law howa aslan nate aw 3amel duck, yekamel
        if (players[i].jumpTimer > 0.0f || players[i].duckTimer > 0.0f)
            continue;

        if (jumpKey)
            players[i].jumpTimer = JUMP_TIME;
        if (duckKey)
            players[i].duckTimer = DUCK_TIME;
    }
}

// ==========================================
// El setup (window, shaders, buffers)
// ==========================================

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
        glfwTerminate();
        return false;
    }

    // Blending 3ashan el 7awaf el shafafa fel sprites
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetMouseButtonCallback(Window, mouseButtonCallback);
    glfwSetKeyCallback(Window, keyCallback);

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
    }

    return shader;
}

// Program men vertex w fragment shader (nafs el 5atawat lel etnen)
GLuint makeProgram(const char* vertexCode, const char* fragmentCode) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Shader link error:\n" << log << '\n';
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void pipeline() {
    ShaderProgram = makeProgram(VertexShaderCode, FragmentShaderCode);
    OffsetLocation = glGetUniformLocation(ShaderProgram, "offset");
    RedLocation = glGetUniformLocation(ShaderProgram, "redAmount");

    TexProgram = makeProgram(TexVertexShaderCode, TexFragmentShaderCode);
    TexRedLocation = glGetUniformLocation(TexProgram, "redAmount");
    TexGreyLocation = glGetUniformLocation(TexProgram, "greyAmount");

    // Rectangle indices (2 triangles)
    unsigned int indices[] = {
        0, 1, 2,
        2, 1, 3
    };

    // ===== VAO el rectangles: 4 vertices, kol wa7ed (x, y, z, r, g, b) =====
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 6 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position: x, y, z
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color: r, g, b
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // ===== VAO el sprites: 4 vertices, kol wa7ed (x, y, u, v) =====
    glGenVertexArrays(1, &TexVAO);
    glGenBuffers(1, &TexVBO);

    glBindVertexArray(TexVAO);

    glBindBuffer(GL_ARRAY_BUFFER, TexVBO);
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

    // Nafs el EBO (nafs el 6 indices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// Release resources
void cleanup() {
    for (int i = 0; i < SPRITE_COUNT; i++)
        glDeleteTextures(1, &sprites[i].texture);

    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &TexVBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &TexVAO);
    glDeleteProgram(ShaderProgram);
    glDeleteProgram(TexProgram);

    glfwDestroyWindow(Window);
    glfwTerminate();
}

// ==========================================
// El shapes (rectangle w triangle bas)
// ==========================================

// Rectangle men el nos bta3o
void drawRectangle(float rectWidth, float rectHeight, float posX, float posY,
                   float r, float g, float b) {
    float left   = posX - rectWidth / 2.0f;
    float right  = posX + rectWidth / 2.0f;
    float top    = posY + rectHeight / 2.0f;
    float bottom = posY - rectHeight / 2.0f;

    float vertices[] = {
        left,  top,    0.0f, r, g, b,
        left,  bottom, 0.0f, r, g, b,
        right, top,    0.0f, r, g, b,
        right, bottom, 0.0f, r, g, b
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Triangle men 3 no2at
void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3,
                  float r, float g, float b) {
    float vertices[] = {
        x1, y1, 0.0f, r, g, b,
        x2, y2, 0.0f, r, g, b,
        x3, y3, 0.0f, r, g, b
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

// ==========================================
// El font: kol 7arf 5 x 7 morab3at ('#' = malyan)
// ==========================================
const char* FONT_LETTERS[26][7] = {
    {".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"},  // A
    {"####.", "#...#", "#...#", "####.", "#...#", "#...#", "####."},  // B
    {".###.", "#...#", "#....", "#....", "#....", "#...#", ".###."},  // C
    {"####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####."},  // D
    {"#####", "#....", "#....", "####.", "#....", "#....", "#####"},  // E
    {"#####", "#....", "#....", "####.", "#....", "#....", "#...."},  // F
    {".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###."},  // G
    {"#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#"},  // H
    {".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###."},  // I
    {"..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.."},  // J
    {"#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#"},  // K
    {"#....", "#....", "#....", "#....", "#....", "#....", "#####"},  // L
    {"#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#"},  // M
    {"#...#", "##..#", "#.#.#", "#..##", "#...#", "#...#", "#...#"},  // N
    {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},  // O
    {"####.", "#...#", "#...#", "####.", "#....", "#....", "#...."},  // P
    {".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#"},  // Q
    {"####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#"},  // R
    {".####", "#....", "#....", ".###.", "....#", "....#", "####."},  // S
    {"#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.."},  // T
    {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},  // U
    {"#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.."},  // V
    {"#...#", "#...#", "#...#", "#.#.#", "#.#.#", "#.#.#", ".#.#."},  // W
    {"#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#"},  // X
    {"#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.."},  // Y
    {"#####", "....#", "...#.", "..#..", ".#...", "#....", "#####"},  // Z
};

const char* FONT_DIGITS[10][7] = {
    {".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###."},  // 0
    {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."},  // 1
    {".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####"},  // 2
    {"####.", "....#", "....#", ".###.", "....#", "....#", "####."},  // 3
    {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."},  // 4
    {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."},  // 5
    {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."},  // 6
    {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."},  // 7
    {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."},  // 8
    {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."},  // 9
};

// 3alamat el kalam (! . , ? ' - :)
const char FONT_PUNCT_CHARS[7] = {'!', '.', ',', '?', '\'', '-', ':'};
const char* FONT_PUNCT[7][7] = {
    {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."},  // !
    {".....", ".....", ".....", ".....", ".....", ".##..", ".##.."},  // .
    {".....", ".....", ".....", ".....", ".##..", "..#..", ".#..."},  // ,
    {".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.."},  // ?
    {"..#..", "..#..", ".#...", ".....", ".....", ".....", "....."},  // '
    {".....", ".....", ".....", "#####", ".....", ".....", "....."},  // -
    {".....", ".##..", ".##..", ".....", ".##..", ".##..", "....."},  // :
};

// 7agm morab3 el 7arf bel pixels (rakam sa7ee7 3ashan el 7oroof tetla3 7adda)
// Bena5od el 3ard aw el tool * 4/3 (el as8ar) 3ashan law el shasha 3areeda
// el 7oroof mateb2ash atwal men el boxes
// masalan 800x600 w pixel 0.0075 -> 0.0075 * 400 = 3 pixels
int textPixels(float pixel) {
    float baseWidth = width;
    if (height * 4.0f / 3.0f < baseWidth)
        baseWidth = height * 4.0f / 3.0f;

    int px = (int)(pixel * baseWidth / 2.0f + 0.5f);
    if (px < 1)
        px = 1;

    return px;
}

// 3ard el kelma: kol 7arf 5 morab3at + 2 fadyeen
float textWidth(std::string text, float pixel) {
    float pixelX = textPixels(pixel) * 2.0f / width;
    return text.size() * 7.0f * pixelX - 2.0f * pixelX;
}

// Kelma el shmal bta3ha 3and leftX w el nos bta3ha 3and y
void drawTextLeft(std::string text, float leftX, float y, float pixel,
                  float r, float g, float b) {
    int px = textPixels(pixel);
    float pixelX = px * 2.0f / width;
    float pixelY = px * 2.0f / height;

    // Nzabbat el bedaya 3ala pixel kamel (3ashan kol morab3 yeb2a 7add)
    float startX = floor((leftX + 1.0f) * width / 2.0f + 0.5f) * 2.0f / width - 1.0f;
    float topY = floor((y + 3.5f * pixelY + 1.0f) * height / 2.0f + 0.5f) * 2.0f / height - 1.0f;

    for (int i = 0; i < (int)text.size(); i++) {
        char c = text[i];
        const char** rows = nullptr;

        if (c >= 'A' && c <= 'Z')
            rows = FONT_LETTERS[c - 'A'];
        if (c >= '0' && c <= '9')
            rows = FONT_DIGITS[c - '0'];

        for (int k = 0; k < 7; k++) {
            if (c == FONT_PUNCT_CHARS[k])
                rows = FONT_PUNCT[k];
        }

        // Masafa: nseebha fadya
        if (rows == nullptr)
            continue;

        float letterX = startX + i * 7.0f * pixelX;

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (rows[row][col] == '#')
                    drawRectangle(pixelX, pixelY,
                                  letterX + col * pixelX + pixelX / 2.0f,
                                  topY - row * pixelY - pixelY / 2.0f, r, g, b);
            }
        }
    }
}

// Kelma fel nos 3and centerX
void drawText(std::string text, float centerX, float y, float pixel,
              float r, float g, float b) {
    drawTextLeft(text, centerX - textWidth(text, pixel) / 2.0f, y, pixel, r, g, b);
}

// Kelma b dell gham2 wara (3ashan teban 3ala ay 5alfeya)
void drawTextShadow(std::string text, float centerX, float y, float pixel,
                    float r, float g, float b) {
    int shadow = textPixels(pixel) / 3;
    if (shadow < 1)
        shadow = 1;

    drawText(text, centerX + shadow * 2.0f / width, y - shadow * 2.0f / height, pixel, 0.15f, 0.07f, 0.04f);
    drawText(text, centerX, y, pixel, r, g, b);
}

// ==========================================
// El UI (panels, zarayer, coins)
// ==========================================

// Panel zay lo7 sa5r: bervaz gham2, bervaz bel lon, gowa bonny, w 4 morab3at fel zawaya
void drawPanel(float w, float h, float x, float y, float ar, float ag, float ab) {
    drawRectangle(w + 0.04f, h + 0.05f, x, y, 0.12f, 0.07f, 0.04f);
    drawRectangle(w, h, x, y, ar, ag, ab);
    drawRectangle(w - 0.03f, h - 0.04f, x, y, 0.24f, 0.15f, 0.09f);

    float cornerX = w / 2.0f - 0.04f;
    float cornerY = h / 2.0f - 0.05f;

    for (int i = 0; i < 4; i++) {
        float sx = (i % 2 == 0) ? -1.0f : 1.0f;
        float sy = (i < 2) ? 1.0f : -1.0f;
        drawRectangle(0.02f, 0.027f, x + sx * cornerX, y + sy * cornerY, ar, ag, ab);
    }
}

// Zarayer menu (bytnawaro bortoo2any lama el mouse 3alehom)
void drawButtons(std::string* labels, float* buttonY, int count) {
    float mx, my;
    getMousePosition(&mx, &my);
    int hovered = buttonAt(mx, my, buttonY, count);

    for (int i = 0; i < count; i++) {
        // El 3ady: sa5r bonny | el mouse 3aleh: bortoo2any
        float br = 0.62f, bg = 0.44f, bb = 0.28f;
        float ir = 0.40f, ig = 0.26f, ib = 0.16f;
        if (hovered == i) {
            br = 1.0f;  bg = 0.85f; bb = 0.45f;
            ir = 0.95f; ig = 0.55f; ib = 0.18f;
        }

        drawRectangle(BUTTON_W + 0.02f, BUTTON_H + 0.025f, 0.0f, buttonY[i], 0.12f, 0.07f, 0.04f);
        drawRectangle(BUTTON_W, BUTTON_H, 0.0f, buttonY[i], br, bg, bb);
        drawRectangle(BUTTON_W - 0.016f, BUTTON_H - 0.02f, 0.0f, buttonY[i], ir, ig, ib);

        drawTextShadow(labels[i], 0.0f, buttonY[i], 0.0075f, 1.0f, 0.95f, 0.85f);
    }
}

// Shakl el coin (dahaby gham2 bara, asfar gowa, no2ta beyda)
void drawCoinIcon(float x, float y, float size) {
    drawRectangle(size, size, x, y, 0.75f, 0.5f, 0.05f);
    drawRectangle(size * 0.7f, size * 0.7f, x, y, 1.0f, 0.85f, 0.2f);
    drawRectangle(size * 0.2f, size * 0.2f, x - size * 0.15f, y + size * 0.15f, 1.0f, 1.0f, 0.8f);
}

// Bar el health fo2 el character
void drawHealthBar(float x, float y, int health, int maxHealth, float r, float g, float b) {
    float fill = (float)health / maxHealth;
    if (fill < 0.0f)
        fill = 0.0f;

    drawRectangle(0.08f, 0.012f, x, y, 0.15f, 0.15f, 0.15f);
    drawRectangle(0.08f * fill, 0.012f, x - 0.04f + 0.04f * fill, y, r, g, b);
}

// Bervaz 7awalen el lane (4 rectangles rofa3)
void drawLaneHighlight(int lane, float thickness, float r, float g, float b) {
    float top    = LANE_Y[lane] + LANE_HEIGHT / 2.0f;
    float bottom = LANE_Y[lane] - LANE_HEIGHT / 2.0f;

    drawRectangle(LANE_RIGHT - LANE_LEFT, thickness, 0.0f, top, r, g, b);
    drawRectangle(LANE_RIGHT - LANE_LEFT, thickness, 0.0f, bottom, r, g, b);
    drawRectangle(thickness, LANE_HEIGHT, LANE_LEFT, LANE_Y[lane], r, g, b);
    drawRectangle(thickness, LANE_HEIGHT, LANE_RIGHT, LANE_Y[lane], r, g, b);
}

// ==========================================
// El game logic
// ==========================================

void spawnPlayer(int type, int lane, float x) {
    Player p;
    p.type = type;
    p.lane = lane;
    p.x = x;
    p.attackTimer = 0.0f;
    p.jumpTimer = 0.0f;
    p.duckTimer = 0.0f;
    p.animState = ANIM_IDLE;
    p.animTime = 0.0f;
    p.attacked = false;
    p.maxHealth = (type == PLAYER_DEFENSE) ? 7 : 5;
    p.health = p.maxHealth;
    players.push_back(p);
}

void spawnEnemy(int type, int lane) {
    Enemy e;
    e.type = type;
    e.lane = lane;
    e.x = LANE_RIGHT - 0.05f;
    e.attackTimer = 0.0f;
    e.animState = ANIM_IDLE;
    e.animTime = 0.0f;
    e.attacked = false;
    e.maxHealth = (type == ENEMY_HARD) ? 7 : 5;
    e.health = e.maxHealth;
    enemies.push_back(e);
}

// El card da maftoo7? (el melee daymann - el ba2y ba3d awel melee w law el coins kfaya)
bool canBuy(int type) {
    if (coins < PLAYER_COST[type])
        return false;

    return type == PLAYER_MELEE || firstMeleePlaced;
}

// Nesht3ry player fe kol lane selected
void buyPlayers(int type) {
    if (!canBuy(type))
        return;

    // El defense: bnestana click 3ala makan (click tany 3al card byelghy)
    if (type == PLAYER_DEFENSE) {
        placingDefense = !placingDefense;
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (!selectedLanes[i] || !canBuy(type))
            continue;

        coins -= PLAYER_COST[type];
        spawnPlayer(type, i, LANE_LEFT + 0.05f);

        if (type == PLAYER_MELEE)
            firstMeleePlaced = true;
    }
}

// Ne7ot defense fe makan el click bel zabt
void placeDefense(int lane, float x) {
    if (!canBuy(PLAYER_DEFENSE))
        return;

    if (x < LANE_LEFT + 0.05f)
        x = LANE_LEFT + 0.05f;
    if (x > LANE_RIGHT - 0.05f)
        x = LANE_RIGHT - 0.05f;

    coins -= PLAYER_COST[PLAYER_DEFENSE];
    spawnPlayer(PLAYER_DEFENSE, lane, x);
}

// Etnen 2orayyebeen men ba3d?
bool isTouching(float x1, float x2) {
    return fabs(x1 - x2) < TOUCH_DISTANCE;
}

// Anhy enemy 5abat fel player da (-1 law mafeesh)
int findTouchingEnemy(Player p) {
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (enemies[i].lane == p.lane && isTouching(p.x, enemies[i].x))
            return i;
    }

    return -1;
}

// El player da wara defense fe nafs el lane? (yeb2a ma7mee)
bool isProtected(Player p) {
    if (p.type == PLAYER_DEFENSE)
        return false;

    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].type == PLAYER_DEFENSE && players[i].lane == p.lane && p.x <= players[i].x)
            return true;
    }

    return false;
}

// Anhy player 5abat fel enemy da (-1 law mafeesh)
int findTouchingPlayer(Enemy e) {
    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].lane == e.lane && isTouching(e.x, players[i].x))
            return i;
    }

    return -1;
}

// Ntala3 talka gdeeda
void fireProjectile(int lane, float x, float speed, bool fromPlayer, int height) {
    Projectile s;
    s.lane = lane;
    s.x = x;
    s.speed = speed;
    s.fromPlayer = fromPlayer;
    s.height = height;
    s.hit = false;
    projectiles.push_back(s);
}

// El players: yemsho, yedrabo
void updatePlayers() {
    for (int i = 0; i < (int)players.size(); i++) {
        Player& p = players[i];

        if (p.jumpTimer > 0.0f)
            p.jumpTimer -= deltaTime;
        if (p.duckTimer > 0.0f)
            p.duckTimer -= deltaTime;

        // El defense mabyet7araksh w mabyedrabsh
        if (p.type == PLAYER_DEFENSE)
            continue;

        int e = findTouchingEnemy(p);

        // Law 5abat fe enemy ye2af
        if (e == -1)
            p.x += PLAYER_SPEED * deltaTime;

        // Wesel a5er el shar3: yo2af hena le7ad ma enemy yeegy y2tlo
        if (p.x > LANE_RIGHT - 0.05f)
            p.x = LANE_RIGHT - 0.05f;

        p.attackTimer += deltaTime;

        if (p.type == PLAYER_MELEE) {
            // Mafeesh enemy: yefdal gahez yedrab awel ma yelmes
            if (e == -1 && p.attackTimer > MELEE_ATTACK_TIME)
                p.attackTimer = MELEE_ATTACK_TIME;

            if (e != -1 && p.attackTimer >= MELEE_ATTACK_TIME) {
                enemies[e].health -= 1;
                p.attackTimer = 0.0f;
                p.attacked = true;
            }
        }

        // El ranged byedrab 3ala tool 7atta law mafeesh enemies
        if (p.type == PLAYER_RANGED && p.attackTimer >= RANGED_ATTACK_TIME) {
            p.attackTimer = 0.0f;
            p.attacked = true;
            fireProjectile(p.lane, p.x, PROJECTILE_SPEED, true, SHOT_MIDDLE);
        }
    }
}

// El enemies: yemsho, yedrabo
void updateEnemies() {
    for (int i = 0; i < (int)enemies.size(); i++) {
        Enemy& e = enemies[i];
        int p = findTouchingPlayer(e);

        // Law 5abat fe player ye2af
        if (p == -1) {
            if (e.type == ENEMY_HARD)
                e.x -= HARD_ENEMY_SPEED * deltaTime;
            else
                e.x -= ENEMY_SPEED * deltaTime;
        }

        // Wesel lel home base: 5esrna (kol 7aga bto2af)
        if (e.x < LANE_LEFT)
            gameState = STATE_LOST;

        e.attackTimer += deltaTime;

        if (e.type == ENEMY_HARD) {
            if (p == -1 && e.attackTimer > MELEE_ATTACK_TIME)
                e.attackTimer = MELEE_ATTACK_TIME;

            // El hard byedrab 2 damage (bas law el player mesh ma7mee wara defense)
            if (p != -1 && e.attackTimer >= MELEE_ATTACK_TIME) {
                if (!isProtected(players[p]))
                    players[p].health -= 2;
                e.attackTimer = 0.0f;
                e.attacked = true;
            }
        }
        else if (e.attackTimer >= ENEMY_SHOOT_TIME) {
            e.attackTimer = 0.0f;
            e.attacked = true;

            // El easy men fo2, el medium men ta7t
            if (e.type == ENEMY_EASY)
                fireProjectile(e.lane, e.x, -PROJECTILE_SPEED, false, SHOT_HIGH);
            else
                fireProjectile(e.lane, e.x, -PROJECTILE_SPEED, false, SHOT_LOW);
        }
    }
}

// Nem4y el talakat w neshoof 5abatet f 7ad wala la2
void updateProjectiles() {
    for (int i = 0; i < (int)projectiles.size(); i++) {
        Projectile& s = projectiles[i];
        s.x += s.speed * deltaTime;

        // Talka el player btedrab enemies bas
        if (s.fromPlayer) {
            for (int j = 0; j < (int)enemies.size() && !s.hit; j++) {
                if (enemies[j].lane == s.lane && isTouching(s.x, enemies[j].x)) {
                    enemies[j].health -= 1;
                    s.hit = true;
                }
            }
        }

        // Talka el enemy btedrab players
        if (!s.fromPlayer) {
            for (int j = 0; j < (int)players.size() && !s.hit; j++) {
                Player p = players[j];

                if (p.lane != s.lane || !isTouching(s.x, p.x) || isProtected(p))
                    continue;

                // Men fo2 w howa 3amel duck, aw men ta7t w howa nate: 3adet
                if (s.height == SHOT_HIGH && p.duckTimer > 0.0f)
                    continue;
                if (s.height == SHOT_LOW && p.jumpTimer > 0.0f)
                    continue;

                players[j].health -= 1;
                s.hit = true;
            }
        }

        if (s.x > LANE_RIGHT || s.x < LANE_LEFT)
            s.hit = true;
    }

    // Nshel el talakat elly 5elset (men el a5er lel awel)
    for (int i = (int)projectiles.size() - 1; i >= 0; i--) {
        if (projectiles[i].hit)
            projectiles.erase(projectiles.begin() + i);
    }
}

// Nshel ay 7ad mat (w el enemy yo2a3 coin makano)
void removeDeadEntities() {
    for (int i = (int)players.size() - 1; i >= 0; i--) {
        if (players[i].health <= 0)
            players.erase(players.begin() + i);
    }

    for (int i = (int)enemies.size() - 1; i >= 0; i--) {
        if (enemies[i].health <= 0) {
            Coin c;
            c.lane = enemies[i].lane;
            c.x = enemies[i].x;
            c.value = ENEMY_DROP[enemies[i].type];
            droppedCoins.push_back(c);

            enemies.erase(enemies.begin() + i);
        }
    }
}

// El players yelmo el coins elly 3al ard
void updateCoins() {
    for (int i = (int)droppedCoins.size() - 1; i >= 0; i--) {
        for (int j = 0; j < (int)players.size(); j++) {
            if (players[j].lane == droppedCoins[i].lane && isTouching(players[j].x, droppedCoins[i].x)) {
                coins += droppedCoins[i].value;
                droppedCoins.erase(droppedCoins.begin() + i);
                break;
            }
        }
    }
}

// Ne5tar el animation: law darab yebda2 el attack, law el attack lessa shaghal yekamel,
// law la2 elly 3ayzeeno (want). Law el animation et8ayaret nebda2ha men el awel.
void updateAnimation(int sprite, int* state, float* time, bool attacked, int want) {
    bool attackPlaying = (*state == ANIM_ATTACK &&
                          *time * sprites[sprite].fps[ANIM_ATTACK] < ANIM_FRAMES);

    // El natta w el duck aham men el darb
    if (want != ANIM_JUMP && want != ANIM_CROUCH) {
        if (attackPlaying)
            want = ANIM_ATTACK;

        if (attacked) {
            *state = ANIM_ATTACK;
            *time = 0.0f;
            return;
        }
    }

    if (want != *state) {
        *state = want;
        *time = 0.0f;
    }
    else
        *time += deltaTime;
}

void updateAnimations() {
    for (int i = 0; i < (int)players.size(); i++) {
        Player& p = players[i];

        int want = ANIM_WALK;
        // Wa2ef: defense, aw 5abat fe enemy, aw wesel a5er el shar3
        if (p.type == PLAYER_DEFENSE || findTouchingEnemy(p) != -1 || p.x >= LANE_RIGHT - 0.05f)
            want = ANIM_IDLE;
        if (p.duckTimer > 0.0f)
            want = ANIM_CROUCH;
        if (p.jumpTimer > 0.0f)
            want = ANIM_JUMP;

        updateAnimation(p.type, &p.animState, &p.animTime, p.attacked, want);
        p.attacked = false;
    }

    for (int i = 0; i < (int)enemies.size(); i++) {
        Enemy& e = enemies[i];

        int want = ANIM_WALK;
        if (e.type == ENEMY_HARD)
            want = ANIM_RUN;
        if (findTouchingPlayer(e) != -1)
            want = ANIM_IDLE;

        updateAnimation(3 + e.type, &e.animState, &e.animTime, e.attacked, want);
        e.attacked = false;
    }
}

// ===== El waves =====

void showBanner(std::string title, std::string subtitle) {
    bannerTitle = title;
    bannerSubtitle = subtitle;
    bannerTimer = BANNER_TIME;
}

void updateWaves() {
    if (bannerTimer > 0.0f)
        bannerTimer -= deltaTime;

    std::string waveName = "WAVE " + std::to_string(currentWave + 1);

    // Lessa fe enemies mnazalteesh fel wave di
    if (enemiesSpawnedInWave < WAVE_SIZE[currentWave]) {
        waveSpawnTimer += deltaTime;

        if (waveSpawnTimer < WAVE_SPAWN_TIME[currentWave])
            return;

        waveSpawnTimer = 0.0f;

        // Awel enemy: el wave bada2et
        if (enemiesSpawnedInWave == 0) {
            if (currentWave == 0)
                showBanner(waveName, "3 PILOTS INCOMING");
            if (currentWave == 1)
                showBanner(waveName, "5 PILOTS AND BRUTES INCOMING");
            if (currentWave == 2)
                showBanner(waveName, "10 ENEMIES - ALL TYPES!");
        }

        // No3 el enemy: wave 1 easy bas, wave 2 easy/medium, wave 3 el talata
        int type = ENEMY_EASY;
        if (currentWave == 1)
            type = enemiesSpawnedInWave % 2;
        if (currentWave == 2)
            type = enemiesSpawnedInWave % 3;

        spawnEnemy(type, rand() % 4);
        enemiesSpawnedInWave++;

        // A5er enemy nezel: el wave 5elset - fadel n5alas 3alehom
        if (enemiesSpawnedInWave == WAVE_SIZE[currentWave])
            showBanner(waveName + " COMPLETE", "CLEAR THE REMAINING ENEMIES!");

        return;
    }

    // Lazem n5alas 3ala elly fadel 3al shasha
    if (!enemies.empty())
        return;

    // Kanet el wave el a5eera: kesebna
    if (currentWave == 2) {
        gameState = STATE_WON;
        return;
    }

    if (waveBreakTimer == 0.0f)
        showBanner(waveName + " CLEARED!", "GET READY FOR WAVE " + std::to_string(currentWave + 2));

    // Ra7a soghayara w ba3den el wave elly ba3daha (awel enemy byenzel 3ala tool)
    waveBreakTimer += deltaTime;

    if (waveBreakTimer >= WAVE_BREAK_TIME) {
        currentWave++;
        enemiesSpawnedInWave = 0;
        waveBreakTimer = 0.0f;
        waveSpawnTimer = WAVE_SPAWN_TIME[currentWave];
    }
}

// Nrga3 kol 7aga zay awel el game
void restartGame() {
    players.clear();
    enemies.clear();
    projectiles.clear();
    droppedCoins.clear();

    coins = START_COINS;
    firstMeleePlaced = false;
    placingDefense = false;

    for (int i = 0; i < 4; i++)
        selectedLanes[i] = false;

    currentWave = 0;
    enemiesSpawnedInWave = 0;
    waveSpawnTimer = 3.0f;
    waveBreakTimer = 0.0f;
    bannerTimer = 0.0f;

    gameState = STATE_PLAYING;
}

// ==========================================
// Rasm el characters
// ==========================================

void drawPlayer(Player p) {
    int frame = spriteFrame(p.type, p.animState, p.animTime);

    // Law nate: byetla3 fel nos el awel w yenzel fel tany
    float jumpY = 0.0f;
    if (p.jumpTimer > 0.0f) {
        float progress = 1.0f - p.jumpTimer / JUMP_TIME;

        if (progress < 0.5f)
            jumpY = JUMP_HEIGHT * progress * 2.0f;
        else
            jumpY = JUMP_HEIGHT * (1.0f - progress) * 2.0f;

        frame = (int)(progress * ANIM_FRAMES);
    }

    // Kol ma el health ye2el yeb2a a7mar aktar
    float red = (1.0f - (float)p.health / p.maxHealth) * 0.8f;

    drawSprite(p.type, p.animState, frame, p.x, LANE_Y[p.lane] - FEET_OFFSET + jumpY,
               true, red, 0.0f, SPRITE_SCALE);

    drawHealthBar(p.x, LANE_Y[p.lane] + 0.17f + jumpY, p.health, p.maxHealth, 0.2f, 0.9f, 0.3f);
}

void drawEnemy(Enemy e) {
    int sprite = 3 + e.type;
    int frame = spriteFrame(sprite, e.animState, e.animTime);
    float red = (1.0f - (float)e.health / e.maxHealth) * 0.8f;

    drawSprite(sprite, e.animState, frame, e.x, LANE_Y[e.lane] - FEET_OFFSET,
               false, red, 0.0f, SPRITE_SCALE);

    drawHealthBar(e.x, LANE_Y[e.lane] + 0.17f, e.health, e.maxHealth, 1.0f, 0.55f, 0.1f);
}

// El talka: 3ala ertefa3 el mawsoura bta3et elly darabha (nawa + 7ala)
void drawProjectile(Projectile s) {
    int shooter = SPRITE_FREMEN;
    if (s.height == SHOT_HIGH)
        shooter = SPRITE_PILOT;
    if (s.height == SHOT_LOW)
        shooter = SPRITE_BRUTE;

    float y = LANE_Y[s.lane] - FEET_OFFSET + sprites[shooter].muzzleDY * SPRITE_SCALE * 2.0f / 600.0f;

    if (s.fromPlayer) {
        drawRectangle(0.034f, 0.016f, s.x, y, 0.9f, 0.6f, 0.1f);
        drawRectangle(0.020f, 0.008f, s.x, y, 1.0f, 0.95f, 0.5f);
    }
    else {
        drawRectangle(0.034f, 0.016f, s.x, y, 0.55f, 0.1f, 0.8f);
        drawRectangle(0.020f, 0.008f, s.x, y, 0.9f, 0.6f, 1.0f);
    }
}

// Law mestaneyeen makan el defense: nwareeh hayeb2a fein ta7t el mouse
void drawDefensePreview() {
    int lane = getMouseLane();
    if (!placingDefense || lane == -1)
        return;

    float mx, my;
    getMousePosition(&mx, &my);

    drawRectangle(0.10f, 0.012f, mx, LANE_Y[lane] - FEET_OFFSET, 1.0f, 0.85f, 0.1f);
    drawSprite(SPRITE_SARDAUKAR, ANIM_IDLE, 0, mx, LANE_Y[lane] - FEET_OFFSET, true, 0.0f, 0.6f, SPRITE_SCALE);
}

// ==========================================
// El HUD (cards, coins, waves, banner)
// ==========================================

void drawCards() {
    float mx, my;
    getMousePosition(&mx, &my);
    int hoveredCard = cardAt(mx, my);

    // Panel wara el cards
    drawRectangle(0.66f, 0.29f, -0.62f, CARD_Y, 0.55f, 0.38f, 0.24f);
    drawRectangle(0.64f, 0.27f, -0.62f, CARD_Y, 0.22f, 0.13f, 0.08f);

    for (int i = 0; i < 3; i++) {
        bool locked = !canBuy(i);
        float x = CARD_X[i];

        // Card el defense mestany makan: bervaz asfar
        if (i == PLAYER_DEFENSE && placingDefense)
            drawRectangle(CARD_W + 0.03f, CARD_H + 0.03f, x, CARD_Y, 1.0f, 0.85f, 0.1f);

        // Hover: bervaz abyad (bas law unlocked)
        if (!locked && hoveredCard == i)
            drawRectangle(CARD_W + 0.02f, CARD_H + 0.02f, x, CARD_Y, 1.0f, 1.0f, 1.0f);

        // Wesh el card (ramady law locked)
        if (locked)
            drawRectangle(CARD_W, CARD_H, x, CARD_Y, 0.55f, 0.55f, 0.55f);
        else
            drawRectangle(CARD_W, CARD_H, x, CARD_Y, 0.85f, 0.8f, 0.6f);

        drawSprite(i, ANIM_IDLE, 0, x, CARD_Y - 0.045f, true, 0.0f, locked ? 1.0f : 0.0f, CARD_SPRITE_SCALE);

        // El se3r ta7t
        if (locked)
            drawText(std::to_string(PLAYER_COST[i]), x, CARD_Y - 0.075f, 0.005f, 0.35f, 0.35f, 0.35f);
        else
            drawText(std::to_string(PLAYER_COST[i]), x, CARD_Y - 0.075f, 0.005f, 0.2f, 0.15f, 0.05f);
    }
}

// 3add el coins fo2 3al yemeen
void drawCoinCounter() {
    drawRectangle(0.38f, 0.16f, 0.74f, CARD_Y, 0.55f, 0.38f, 0.24f);
    drawRectangle(0.36f, 0.14f, 0.74f, CARD_Y, 0.22f, 0.13f, 0.08f);

    drawCoinIcon(0.62f, CARD_Y, 0.07f);
    drawTextLeft(std::to_string(coins), 0.68f, CARD_Y, 0.01f, 1.0f, 0.85f, 0.2f);
}

// 3 morab3at fel nos fo2 (a5dar = 5elset, asfar = delwa2ty, ramady = lessa)
void drawWaveIndicator() {
    drawRectangle(0.44f, 0.16f, 0.15f, CARD_Y, 0.55f, 0.38f, 0.24f);
    drawRectangle(0.42f, 0.14f, 0.15f, CARD_Y, 0.22f, 0.13f, 0.08f);

    for (int i = 0; i < 3; i++) {
        float x = 0.02f + i * 0.13f;

        if (i < currentWave || gameState == STATE_WON)
            drawRectangle(0.1f, 0.1f, x, CARD_Y, 0.2f, 0.7f, 0.3f);
        else if (i == currentWave) {
            drawRectangle(0.115f, 0.115f, x, CARD_Y, 1.0f, 1.0f, 1.0f);
            drawRectangle(0.1f, 0.1f, x, CARD_Y, 1.0f, 0.85f, 0.1f);
        }
        else
            drawRectangle(0.1f, 0.1f, x, CARD_Y, 0.4f, 0.4f, 0.4f);

        drawText(std::to_string(i + 1), x, CARD_Y, 0.008f, 0.1f, 0.1f, 0.1f);
    }
}

void drawWaveBanner() {
    if (bannerTimer <= 0.0f)
        return;

    drawPanel(1.3f, 0.26f, 0.0f, 0.6f, 0.95f, 0.78f, 0.25f);
    drawTextShadow(bannerTitle, 0.0f, 0.655f, 0.010f, 0.95f, 0.78f, 0.25f);
    drawText(bannerSubtitle, 0.0f, 0.535f, 0.005f, 0.95f, 0.92f, 0.85f);
}

// ==========================================
// El menus
// ==========================================

void drawEndScreen() {
    std::string labels[2] = {"RESTART", "MAIN MENU"};

    if (gameState == STATE_WON) {
        drawPanel(1.42f, 0.86f, 0.0f, 0.0f, 0.40f, 0.80f, 0.40f);
        drawTextShadow("VICTORY!", 0.0f, 0.25f, 0.015f, 0.55f, 0.95f, 0.45f);
        drawTextShadow("ARRAKIS IS SAFE", 0.0f, 0.08f, 0.0075f, 1.0f, 0.92f, 0.78f);
    }
    else {
        drawPanel(1.42f, 0.86f, 0.0f, 0.0f, 0.85f, 0.25f, 0.18f);
        drawTextShadow("GAME OVER", 0.0f, 0.25f, 0.015f, 1.0f, 0.35f, 0.25f);
        drawTextShadow("THE ENEMY BROKE THROUGH", 0.0f, 0.08f, 0.0075f, 1.0f, 0.92f, 0.78f);
    }

    drawButtons(labels, END_BUTTON_Y, 2);
}

void drawPauseMenu() {
    std::string labels[3] = {"CONTINUE", "RESTART", "MAIN MENU"};

    drawPanel(0.8f, 0.92f, 0.0f, 0.0f, 0.95f, 0.72f, 0.28f);
    drawTextShadow("PAUSED", 0.0f, 0.27f, 0.015f, 1.0f, 0.78f, 0.30f);
    drawButtons(labels, PAUSE_BUTTON_Y, 3);
}

// ==========================================
// El arena (Arrakis) - rectangles w triangles bas
// ==========================================

// 2amar: morab3 + morab3 tany 3ard (3ashan yeb2a mdawwar shwaya zay pixel art) + 2 7ofar
void drawMoon(float x, float y, float size) {
    float sizeX = size * height / width;

    drawRectangle(sizeX * 1.3f, size, x, y, 0.86f, 0.51f, 0.27f);
    drawRectangle(sizeX, size * 1.3f, x, y, 0.86f, 0.51f, 0.27f);
    drawRectangle(sizeX * 1.1f, size * 0.8f, x, y, 0.98f, 0.94f, 0.84f);
    drawRectangle(sizeX * 0.8f, size * 1.1f, x, y, 0.98f, 0.94f, 0.84f);
    drawRectangle(sizeX * 0.25f, size * 0.25f, x - sizeX * 0.2f, y + size * 0.2f, 0.88f, 0.82f, 0.70f);
    drawRectangle(sizeX * 0.18f, size * 0.18f, x + sizeX * 0.25f, y - size * 0.2f, 0.88f, 0.82f, 0.70f);
}

// Sa5ra 3aleya (mesa): dor wase3 ta7t, gesm, ras ad2a2, w nour 3al gamb
void drawMesa(float x, float baseY, float w, float h) {
    drawRectangle(w * 1.35f, h * 0.3f, x, baseY + h * 0.15f, 0.45f, 0.27f, 0.17f);
    drawRectangle(w, h, x, baseY + h / 2.0f, 0.48f, 0.29f, 0.18f);
    drawRectangle(w * 0.75f, h * 0.25f, x, baseY + h * 1.125f, 0.50f, 0.30f, 0.18f);
    drawRectangle(w * 0.22f, h, x - w * 0.39f, baseY + h / 2.0f, 0.65f, 0.42f, 0.26f);
}

// Saff kseeban: triangles gamb ba3d (kol wa7ed ertefa3o mo5talef)
void drawDunes(float baseY, float duneHeight, float offset, float r, float g, float b) {
    for (int i = 0; i < 8; i++) {
        float x = -1.2f + i * 0.32f + offset;
        float h = duneHeight;
        if (i % 2 == 1)
            h = duneHeight * 0.6f;

        drawTriangle(x - 0.25f, baseY, x + 0.25f, baseY, x, baseY + h, r, g, b);
    }
}

// Ornithopter (tayyara zay el yaasoob): gesm, kabeena, deil, w 2 agne7a men triangles
// s = el 7agm (1 = 3adi), dark = soora sooda 3ala el 2amar
void drawOrnithopter(float x, float y, float s) {
    float r = 0.22f, g = 0.17f, b = 0.14f;

    // El agne7a (wara el gesm): etnen fo2 w etnen ta7t mayleen
    drawTriangle(x - 0.02f * s, y, x + 0.02f * s, y, x - 0.08f * s, y + 0.05f * s, 0.30f, 0.25f, 0.21f);
    drawTriangle(x - 0.02f * s, y, x + 0.02f * s, y, x + 0.04f * s, y + 0.055f * s, 0.30f, 0.25f, 0.21f);
    drawTriangle(x - 0.02f * s, y, x + 0.02f * s, y, x - 0.07f * s, y - 0.03f * s, 0.30f, 0.25f, 0.21f);
    drawTriangle(x - 0.02f * s, y, x + 0.02f * s, y, x + 0.05f * s, y - 0.035f * s, 0.30f, 0.25f, 0.21f);

    // El gesm w el deil
    drawRectangle(0.09f * s, 0.022f * s, x, y, r, g, b);
    drawRectangle(0.06f * s, 0.008f * s, x + 0.07f * s, y + 0.004f * s, r, g, b);
    drawRectangle(0.008f * s, 0.022f * s, x + 0.10f * s, y + 0.01f * s, r, g, b);

    // El kabeena (shebbak fate7 oddam)
    drawTriangle(x - 0.045f * s, y + 0.011f * s, x - 0.045f * s, y - 0.011f * s, x - 0.07f * s, y - 0.004f * s, r, g, b);
    drawRectangle(0.022f * s, 0.010f * s, x - 0.03f * s, y + 0.004f * s, 0.75f, 0.62f, 0.40f);
}

// Spice harvester: 3arabeyya kbeera bel 3agal, kabeena fo2, w torab spice tale3 wara
void drawHarvester(float x, float y, float s) {
    // Torab el spice (bortoo2any) tale3 men wara
    drawTriangle(x + 0.03f * s, y + 0.03f * s, x + 0.09f * s, y + 0.03f * s, x + 0.13f * s, y + 0.10f * s, 0.95f, 0.55f, 0.20f);
    drawTriangle(x + 0.06f * s, y + 0.03f * s, x + 0.12f * s, y + 0.03f * s, x + 0.17f * s, y + 0.07f * s, 0.98f, 0.70f, 0.35f);

    // El 3agal (sooda) w 3alehom 7etat fat7a
    drawRectangle(0.15f * s, 0.022f * s, x, y + 0.011f * s, 0.15f, 0.12f, 0.10f);
    for (int i = 0; i < 6; i++)
        drawRectangle(0.012f * s, 0.008f * s, x - 0.06f * s + i * 0.024f * s, y + 0.011f * s, 0.38f, 0.32f, 0.26f);

    // El gesm (ramady me7a2a2) w 5att ta7to
    drawRectangle(0.13f * s, 0.045f * s, x, y + 0.045f * s, 0.46f, 0.40f, 0.34f);
    drawRectangle(0.13f * s, 0.006f * s, x, y + 0.025f * s, 0.30f, 0.25f, 0.21f);

    // El kabeena w shebbakha, w el antenna
    drawRectangle(0.045f * s, 0.025f * s, x - 0.035f * s, y + 0.078f * s, 0.40f, 0.35f, 0.30f);
    drawRectangle(0.030f * s, 0.010f * s, x - 0.038f * s, y + 0.080f * s, 0.95f, 0.75f, 0.35f);
    drawRectangle(0.004f * s, 0.035f * s, x + 0.03f * s, y + 0.085f * s, 0.30f, 0.25f, 0.21f);
}

// Dahr sandworm tale3 men el raml: 3awameed (el nos a3la) w 7ala2at gham2a
void drawWormHump(float x, float y, float s) {
    for (int i = -5; i <= 5; i++) {
        int distance = i;
        if (distance < 0)
            distance = -distance;

        float h = (0.05f - distance * 0.008f) * s;
        float shade = 0.0f;
        if (i % 2 == 0)
            shade = 0.06f;

        drawRectangle(0.016f * s, h, x + i * 0.016f * s, y + h / 2.0f, 0.58f - shade, 0.42f - shade, 0.30f - shade);
    }

    // Raml beyetrash men 3and el atraf
    drawTriangle(x - 0.11f * s, y, x - 0.07f * s, y, x - 0.10f * s, y + 0.025f * s, 0.97f, 0.80f, 0.55f);
    drawTriangle(x + 0.07f * s, y, x + 0.11f * s, y, x + 0.10f * s, y + 0.025f * s, 0.97f, 0.80f, 0.55f);
}

// Madeena fel sa7ra (zay Arrakeen): mabany morab3a 3la ba3d w shababeek sooda w borg
void drawDesertCity(float x, float y) {
    float heights[6] = {0.05f, 0.08f, 0.06f, 0.11f, 0.07f, 0.045f};

    for (int i = 0; i < 6; i++) {
        float bx = x + i * 0.035f;
        float h = heights[i];

        drawRectangle(0.034f, h, bx, y + h / 2.0f, 0.62f, 0.45f, 0.32f);
        drawRectangle(0.008f, h, bx - 0.013f, y + h / 2.0f, 0.75f, 0.58f, 0.42f);

        // Shababeek
        for (int k = 1; k * 0.02f < h; k++)
            drawRectangle(0.008f, 0.006f, bx + 0.004f, y + k * 0.02f, 0.25f, 0.16f, 0.11f);
    }

    // Borg el moraqaba fel nos b sa2f mosallas
    drawRectangle(0.014f, 0.16f, x + 0.105f, y + 0.08f, 0.55f, 0.40f, 0.28f);
    drawTriangle(x + 0.095f, y + 0.16f, x + 0.115f, y + 0.16f, x + 0.105f, y + 0.19f, 0.55f, 0.40f, 0.28f);
}

// Bawwabet el Harkonnen 3al yemeen (men henak el enemies btedkhol): 7adeed eswed w nour a7mar
void drawEnemyGate(float y) {
    drawRectangle(0.08f, 0.26f, 0.965f, y, 0.10f, 0.09f, 0.10f);
    drawRectangle(0.06f, 0.24f, 0.965f, y, 0.20f, 0.18f, 0.20f);

    // 5otoot 7adeed
    for (int i = 0; i < 4; i++)
        drawRectangle(0.06f, 0.008f, 0.965f, y - 0.09f + i * 0.06f, 0.12f, 0.11f, 0.12f);

    // Shoka fo2 w nour a7mar
    drawTriangle(0.935f, y + 0.13f, 0.995f, y + 0.13f, 0.965f, y + 0.18f, 0.10f, 0.09f, 0.10f);
    drawRectangle(0.03f, 0.012f, 0.965f, y + 0.10f, 0.85f, 0.10f, 0.08f);
    drawRectangle(0.012f, 0.012f, 0.965f, y + 0.10f, 1.0f, 0.55f, 0.45f);
}

// Wind trap (bey-lamm el maya men el hawa): 3amood + ras mosallas + ze3aneef
void drawWindTrap(float x, float y) {
    drawRectangle(0.03f, 0.07f, x, y + 0.035f, 0.42f, 0.42f, 0.44f);
    drawRectangle(0.006f, 0.07f, x - 0.01f, y + 0.035f, 0.58f, 0.58f, 0.60f);

    for (int i = 0; i < 3; i++)
        drawRectangle(0.036f, 0.005f, x, y + 0.015f + i * 0.02f, 0.25f, 0.25f, 0.27f);

    drawTriangle(x - 0.024f, y + 0.07f, x + 0.024f, y + 0.07f, x, y + 0.10f, 0.32f, 0.32f, 0.35f);
}

// El sama w ba3eed (bnesta5demha fel arena w fel main menu)
void drawBackground() {
    // El sama: 3 shera2et men gham2 fo2 le fate7 3and el ofo2
    drawRectangle(2.0f, 0.4f, 0.0f, 0.80f, 0.42f, 0.17f, 0.10f);
    drawRectangle(2.0f, 0.4f, 0.0f, 0.40f, 0.70f, 0.34f, 0.16f);
    drawRectangle(2.0f, 1.4f, 0.0f, -0.30f, 0.95f, 0.65f, 0.38f);

    // Negoom soghayara fel gham2
    for (int i = 0; i < 12; i++)
        drawRectangle(0.006f, 0.008f, -0.95f + i * 0.17f, 0.97f - (i % 3) * 0.05f, 0.95f, 0.85f, 0.70f);

    // El 2amarein bto3 Arrakis
    drawMoon(0.46f, 0.79f, 0.15f);
    drawMoon(-0.17f, 0.88f, 0.07f);

    // Sa5r be3eed
    drawMesa(-0.18f, 0.62f, 0.12f, 0.12f);
    drawMesa(0.97f, 0.62f, 0.10f, 0.08f);

    // Ornithopters fel sama (wa7da oddam el 2amar el kebeer)
    drawOrnithopter(0.46f, 0.74f, 0.8f);
    drawOrnithopter(-0.62f, 0.84f, 0.6f);

    // Kseeban be3eeda w 2orayyeba
    drawDunes(0.60f, 0.10f, 0.0f, 0.88f, 0.62f, 0.38f);
    drawDunes(0.60f, 0.06f, 0.16f, 0.80f, 0.55f, 0.33f);

    // 3al ofo2: el madeena, el harvester beyetla3 spice, w worm gay 3aleh
    drawDesertCity(-0.27f, 0.61f);
    drawHarvester(0.20f, 0.61f, 0.8f);
    drawWormHump(0.72f, 0.61f, 1.0f);
}

// El fasel el sa5ry ben el lanes: sa5r + nour fo2 + dell ta7t + 7agar + spice
void drawRockRidge(float y, float h) {
    drawRectangle(2.0f, h, 0.0f, y, 0.52f, 0.35f, 0.23f);
    drawRectangle(2.0f, 0.008f, 0.0f, y + h / 2.0f - 0.004f, 0.78f, 0.60f, 0.42f);
    drawRectangle(2.0f, 0.008f, 0.0f, y - h / 2.0f + 0.004f, 0.24f, 0.15f, 0.10f);

    for (int i = 0; i < 9; i++) {
        float x = -0.88f + i * 0.22f;
        if (i % 2 == 1)
            x += 0.03f;

        drawRectangle(0.045f, 0.018f, x, y - h * 0.05f, 0.40f, 0.26f, 0.17f);
        drawRectangle(0.028f, 0.010f, x - 0.004f, y + h * 0.05f, 0.58f, 0.41f, 0.27f);

        // Spice bortoo2any kol talat 7agar
        if (i % 3 == 1)
            drawRectangle(0.012f, 0.012f, x + 0.08f, y + h * 0.1f, 1.0f, 0.55f, 0.15f);
    }
}

// El home base (sietch el Atreides) 3al shmal: soor 7agar + toob + bab fe kol lane + 3alam a5dar
void drawHomeBase() {
    // El soor
    drawRectangle(0.10f, 1.62f, -0.95f, -0.19f, 0.12f, 0.10f, 0.10f);
    drawRectangle(0.08f, 1.60f, -0.955f, -0.19f, 0.36f, 0.38f, 0.40f);

    // Toob el soor (5otoot ofo2eya)
    for (int i = 0; i < 16; i++)
        drawRectangle(0.08f, 0.006f, -0.955f, 0.58f - i * 0.1f, 0.26f, 0.27f, 0.29f);

    // Ashraf fo2 el soor
    for (int i = 0; i < 3; i++)
        drawRectangle(0.022f, 0.04f, -0.985f + i * 0.03f, 0.63f, 0.36f, 0.38f, 0.40f);

    // Bab fe kol lane (el players byetla3o meno)
    for (int i = 0; i < 4; i++) {
        drawRectangle(0.05f, 0.17f, -0.94f, LANE_Y[i] - 0.03f, 0.18f, 0.12f, 0.08f);
        drawRectangle(0.038f, 0.15f, -0.94f, LANE_Y[i] - 0.035f, 0.08f, 0.05f, 0.03f);
        drawTriangle(-0.965f, LANE_Y[i] + 0.055f, -0.915f, LANE_Y[i] + 0.055f,
                     -0.94f, LANE_Y[i] + 0.085f, 0.18f, 0.12f, 0.08f);
    }

    // El 3alam: 3amood + 3alam a5dar (Atreides) + sa2r dahaby
    drawRectangle(0.008f, 0.08f, -0.97f, 0.66f, 0.25f, 0.2f, 0.15f);
    drawTriangle(-0.966f, 0.70f, -0.966f, 0.64f, -0.89f, 0.67f, 0.15f, 0.55f, 0.25f);
    drawRectangle(0.012f, 0.016f, -0.948f, 0.67f, 0.95f, 0.8f, 0.3f);
}

// El arena kolaha
void drawArena() {
    drawBackground();

    // El ard ta7t el kseeban
    drawRectangle(2.0f, 1.62f, 0.0f, -0.19f, 0.80f, 0.60f, 0.38f);

    // El fasel elly fo2 el lanes, w benhom, w ta7t
    drawRockRidge(0.585f, 0.07f);
    for (int i = 0; i < 4; i++)
        drawRockRidge(LANE_Y[i] - 0.19f, 0.085f);

    // El lanes: dell + raml + dell rofayya3 fo2
    for (int i = 0; i < 4; i++) {
        drawRectangle(1.89f, 0.30f, 0.0f, LANE_Y[i] - 0.012f, 0.33f, 0.22f, 0.15f);
        drawRectangle(1.85f, 0.28f, 0.0f, LANE_Y[i], 0.58f, 0.43f, 0.30f);
        drawRectangle(1.85f, 0.010f, 0.0f, LANE_Y[i] + 0.135f, 0.42f, 0.30f, 0.21f);
    }

    // Bawwabat el Harkonnen 3al yemeen (fe kol lane)
    for (int i = 0; i < 4; i++)
        drawEnemyGate(LANE_Y[i]);

    drawHomeBase();

    // Wind traps gamb el home base (el Atreides byelammo beha el maya)
    drawWindTrap(-0.84f, 0.62f);
    drawWindTrap(-0.77f, 0.62f);
}

// El main menu: el sama w el kseeban + raml + el esm + el zarayer
void drawMainMenu() {
    drawBackground();
    drawRectangle(2.0f, 1.6f, 0.0f, -0.2f, 0.80f, 0.55f, 0.33f);

    // Sandworm kbeer tale3 men el raml 3al yemeen: 7ala2at fo2 ba3d, w bo2o maftoo7 b snan
    for (int k = 0; k < 6; k++) {
        float y = -0.40f + k * 0.075f;
        float x = 0.68f - k * 0.012f;
        float w = 0.22f - k * 0.006f;

        drawRectangle(w, 0.075f, x, y, 0.55f, 0.40f, 0.28f);
        drawRectangle(w, 0.012f, x, y - 0.03f, 0.40f, 0.28f, 0.19f);
        drawRectangle(w * 0.15f, 0.075f, x - w * 0.42f, y, 0.68f, 0.52f, 0.38f);
    }

    // El bo2: 7erf gham2, gowa eswed, w snan beyda men fo2 w ta7t
    drawRectangle(0.24f, 0.06f, 0.61f, 0.04f, 0.36f, 0.25f, 0.17f);
    drawRectangle(0.19f, 0.04f, 0.61f, 0.04f, 0.10f, 0.05f, 0.04f);
    for (int i = 0; i < 6; i++) {
        float tx = 0.525f + i * 0.03f;
        drawTriangle(tx, 0.06f, tx + 0.02f, 0.06f, tx + 0.01f, 0.038f, 0.95f, 0.90f, 0.80f);
        drawTriangle(tx, 0.02f, tx + 0.02f, 0.02f, tx + 0.01f, 0.042f, 0.95f, 0.90f, 0.80f);
    }

    // El kseeban el 2orayyeba (oddam regl el worm) w raml beyetrash
    drawDunes(-0.4f, 0.25f, 0.1f, 0.70f, 0.46f, 0.27f);
    drawRectangle(2.0f, 0.6f, 0.0f, -0.7f, 0.70f, 0.46f, 0.27f);
    drawTriangle(0.45f, -0.40f, 0.55f, -0.40f, 0.47f, -0.30f, 0.97f, 0.80f, 0.55f);
    drawTriangle(0.80f, -0.40f, 0.90f, -0.40f, 0.88f, -0.28f, 0.97f, 0.80f, 0.55f);

    // Harvester kbeer 3al shmal w ornithopter gay yen2azo
    drawHarvester(-0.68f, -0.42f, 2.2f);
    drawOrnithopter(-0.62f, 0.08f, 1.6f);

    // No2at spice bortoo2any fel raml
    for (int i = 0; i < 14; i++)
        drawRectangle(0.012f, 0.012f, -0.92f + i * 0.14f, -0.62f - (i % 3) * 0.1f, 1.0f, 0.55f, 0.15f);

    drawTextShadow("ARRAKIS DEFENSE", 0.0f, 0.46f, 0.015f, 1.0f, 0.78f, 0.30f);
    drawTextShadow("THE SPICE MUST FLOW", 0.0f, 0.27f, 0.0075f, 1.0f, 0.92f, 0.78f);

    std::string labels[2] = {"START", "EXIT"};
    drawButtons(labels, MAIN_MENU_BUTTON_Y, 2);

    drawTextShadow("ESC TO PAUSE", 0.0f, -0.4f, 0.005f, 1.0f, 0.92f, 0.78f);
}

// ==========================================
// El cutscene (20 sanya 2abl el game)
// ==========================================

float cutsceneTime = 0.0f;
float CUTSCENE_LENGTH = 20.0f;
float CUTSCENE_SCALE = 0.7f;

// Kol character: anhy sprite, anhy lane, beyemshy men fein le fein, emta,
// w emta yedrab (-1 = mabyedrabsh)
struct CutsceneActor {
    int sprite;
    int lane;
    float startX;
    float endX;
    float walkStart;
    float walkEnd;
    float attackStart;
    bool isPlayer;
    const char* name;
};

// El enemies beyed5olo el awel men el yemeen, ba3den el abtal men el shmal
CutsceneActor CUTSCENE_ACTORS[6] = {
    {SPRITE_ATREIDES,  1, -1.15f, -0.35f, 8.4f, 10.0f, 14.9f, true,  "ATREIDES"},
    {SPRITE_FREMEN,    0, -1.15f, -0.50f, 8.8f, 10.4f, 15.2f, true,  "FREMEN"},
    {SPRITE_SARDAUKAR, 2, -1.15f, -0.45f, 9.2f, 10.8f, -1.0f, true,  "SARDAUKAR"},
    {SPRITE_PILOT,     0,  1.15f,  0.50f, 4.4f, 6.0f,  15.8f, false, "HARKONNEN PILOT"},
    {SPRITE_BRUTE,     2,  1.15f,  0.45f, 5.0f, 6.6f,  15.5f, false, "HARKONNEN BRUTE"},
    {SPRITE_OFFICER,   1,  1.15f,  0.35f, 5.6f, 7.2f,  12.1f, false, "HARKONNEN OFFICER"},
};

// Kol gomla: emta tebda2 w te5las, meen beyetkalem (-1 = narration), w el kalam (satreen)
struct CutsceneLine {
    float start;
    float end;
    int speaker;
    const char* line1;
    const char* line2;
};

CutsceneLine CUTSCENE_LINES[6] = {
    {0.6f,  4.2f,  -1, "ARRAKIS, THE DESERT",  "PLANET OF SPICE."},
    {4.2f,  8.2f,  -1, "HOUSE HARKONNEN",      "MARCHES FOR THE SPICE."},
    {8.2f,  11.8f, -1, "ONLY THE ATREIDES",    "STAND IN THEIR WAY."},
    {12.0f, 14.8f, 5,  "THIS SPICE IS OURS.",  "SURRENDER THE LANES!"},
    {14.8f, 17.6f, 0,  "NEVER. WHILE WE",      "STAND, IT WILL FLOW!"},
    {17.6f, 20.0f, -1, "",                     ""},
};

void startGame() {
    restartGame();
    gameState = STATE_CUTSCENE;
    cutsceneTime = 0.0f;
}

// El character fein delwa2ty (beyemshy men startX le endX)
float actorX(CutsceneActor a) {
    float t = (cutsceneTime - a.walkStart) / (a.walkEnd - a.walkStart);
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    return a.startX + (a.endX - a.startX) * t;
}

// Character: walk le 7ad makano, ba3den idle, w attack lama yedrab
void drawCutsceneActor(CutsceneActor a) {
    float t = cutsceneTime;

    if (t < a.walkStart)
        return;

    int anim = ANIM_IDLE;
    float animTime = t;

    if (t < a.walkEnd) {
        anim = ANIM_WALK;
        animTime = t - a.walkStart;
    }
    else if (a.attackStart >= 0.0f && t >= a.attackStart && t < a.attackStart + 1.0f) {
        anim = ANIM_ATTACK;
        animTime = t - a.attackStart;
    }

    int frame = spriteFrame(a.sprite, anim, animTime);
    drawSprite(a.sprite, anim, frame, actorX(a), LANE_Y[a.lane] - FEET_OFFSET,
               a.isPlayer, 0.0f, 0.0f, CUTSCENE_SCALE);
}

// Gomla men el cutscene: narration (fel nos) aw kalam character (b sorto w esmo)
// el kalam beyetkteb 7arf 7arf (30 7arf fel sanya)
void drawCutsceneLine(CutsceneLine line) {
    std::string text1 = line.line1;
    std::string text2 = line.line2;

    int count1 = (int)((cutsceneTime - line.start) * 30.0f);
    int count2 = count1 - (int)text1.size();

    if (count1 > (int)text1.size())
        count1 = (int)text1.size();
    if (count2 < 0)
        count2 = 0;
    if (count2 > (int)text2.size())
        count2 = (int)text2.size();

    // El title fel a5er
    if (text1 == "") {
        drawRectangle(2.0f, 0.36f, 0.0f, -0.72f, 0.06f, 0.04f, 0.03f);
        drawTextShadow("ARRAKIS DEFENSE", 0.0f, -0.67f, 0.015f, 1.0f, 0.78f, 0.30f);
        drawText("DEFEND ALL FOUR LANES", 0.0f, -0.81f, 0.0075f, 1.0f, 0.95f, 0.85f);
        return;
    }

    // Narration: shereet eswed w el kalam fel nos
    if (line.speaker == -1) {
        drawRectangle(2.0f, 0.36f, 0.0f, -0.72f, 0.06f, 0.04f, 0.03f);
        drawRectangle(2.0f, 0.008f, 0.0f, -0.54f, 0.85f, 0.62f, 0.25f);
        drawRectangle(2.0f, 0.008f, 0.0f, -0.90f, 0.85f, 0.62f, 0.25f);

        drawTextLeft(text1.substr(0, count1), -textWidth(text1, 0.01f) / 2.0f, -0.64f, 0.01f, 0.98f, 0.82f, 0.45f);
        drawTextLeft(text2.substr(0, count2), -textWidth(text2, 0.01f) / 2.0f, -0.79f, 0.01f, 1.0f, 0.97f, 0.90f);
        return;
    }

    CutsceneActor speaker = CUTSCENE_ACTORS[line.speaker];

    // Lon azra2 lel players, a7mar lel enemies
    float r = 1.0f, g = 0.50f, b = 0.40f;
    if (speaker.isPlayer) {
        r = 0.60f; g = 0.82f; b = 1.0f;
    }

    // Sahm fo2 ras elly beyetkalem (3 rectangles ad2a2 fe ba3d)
    float ax = actorX(speaker);
    float ay = LANE_Y[speaker.lane] + 0.25f;
    drawRectangle(0.030f, 0.012f, ax, ay + 0.012f, 1.0f, 0.82f, 0.25f);
    drawRectangle(0.018f, 0.012f, ax, ay, 1.0f, 0.82f, 0.25f);
    drawRectangle(0.006f, 0.012f, ax, ay - 0.012f, 1.0f, 0.82f, 0.25f);

    // El sandoo2: sorto 3al shmal, esmo soghayar, w el kalam kbeer
    drawPanel(1.8f, 0.40f, 0.0f, -0.70f, r, g, b);

    int frame = spriteFrame(speaker.sprite, ANIM_IDLE, cutsceneTime);
    drawSprite(speaker.sprite, ANIM_IDLE, frame, -0.75f, -0.885f, speaker.isPlayer, 0.0f, 0.0f, 0.65f);

    drawTextLeft(speaker.name, -0.59f, -0.575f, 0.0075f, r, g, b);
    drawTextLeft(text1.substr(0, count1), -0.59f, -0.685f, 0.01f, 1.0f, 0.97f, 0.90f);
    drawTextLeft(text2.substr(0, count2), -0.59f, -0.80f, 0.01f, 1.0f, 0.97f, 0.90f);
}

void drawCutscene() {
    // El characters (lane 0 el awel 3ashan elly ta7t yeb2a oddam)
    for (int lane = 0; lane < 3; lane++) {
        for (int i = 0; i < 6; i++) {
            if (CUTSCENE_ACTORS[i].lane == lane)
                drawCutsceneActor(CUTSCENE_ACTORS[i]);
        }
    }

    // Shera2et cinema sooda fo2 w ta7t
    drawRectangle(2.0f, 0.10f, 0.0f, 0.95f, 0.0f, 0.0f, 0.0f);
    drawRectangle(2.0f, 0.10f, 0.0f, -0.95f, 0.0f, 0.0f, 0.0f);

    for (int i = 0; i < 6; i++) {
        if (cutsceneTime >= CUTSCENE_LINES[i].start && cutsceneTime < CUTSCENE_LINES[i].end)
            drawCutsceneLine(CUTSCENE_LINES[i]);
    }

    drawText("SPACE TO SKIP", 0.74f, 0.95f, 0.005f, 0.75f, 0.72f, 0.68f);
}

// ==========================================
// Main
// ==========================================

// Bn7seb Time kol frame
void updateDeltaTime() {
    float currentTime = (float)glfwGetTime();

    deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    // Law el frame et2a5ar awy bn7ot limit 3ashan el 7agat matnotsh
    if (deltaTime > 0.05f)
        deltaTime = 0.05f;
}

int main() {
    if (!initialize())
        return -1;

    pipeline();

    for (int i = 0; i < SPRITE_COUNT; i++)
        loadSprite(sprites[i]);

    lastFrameTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(Window)) {
        updateDeltaTime();

        // 7agm el shasha el 7a2e2y kol frame (law el window et8ayar aw el laptop 3amel scaling)
        glfwGetFramebufferSize(Window, &width, &height);
        glViewport(0, 0, width, height);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(ShaderProgram);
        glUniform2f(OffsetLocation, 0.0f, 0.0f);
        glUniform1f(RedLocation, 0.0f);

        // ===== Main menu =====
        if (gameState == STATE_MENU) {
            drawMainMenu();
            glfwSwapBuffers(Window);
            glfwPollEvents();
            continue;
        }

        drawArena();

        // ===== Cutscene =====
        if (gameState == STATE_CUTSCENE) {
            cutsceneTime += deltaTime;
            if (cutsceneTime >= CUTSCENE_LENGTH)
                restartGame();

            drawCutscene();
            glfwSwapBuffers(Window);
            glfwPollEvents();
            continue;
        }

        // ===== El game (wa2ef law kesebna aw 5esrna aw pause) =====
        if (gameState == STATE_PLAYING) {
            updateWaves();
            updatePlayers();
            updateEnemies();
            updateProjectiles();
            removeDeadEntities();
            updateAnimations();
            updateCoins();
        }

        // Lane highlight: selected asfar te5een, hover abyad rofayya3
        int hoveredLane = getMouseLane();
        for (int i = 0; i < 4; i++) {
            if (selectedLanes[i])
                drawLaneHighlight(i, 0.02f, 1.0f, 0.85f, 0.1f);
        }
        if (hoveredLane != -1 && !selectedLanes[hoveredLane])
            drawLaneHighlight(hoveredLane, 0.01f, 1.0f, 1.0f, 1.0f);

        for (int i = 0; i < (int)droppedCoins.size(); i++)
            drawCoinIcon(droppedCoins[i].x, LANE_Y[droppedCoins[i].lane] - 0.07f, 0.04f);

        for (int i = 0; i < (int)players.size(); i++)
            drawPlayer(players[i]);

        for (int i = 0; i < (int)enemies.size(); i++)
            drawEnemy(enemies[i]);

        for (int i = 0; i < (int)projectiles.size(); i++)
            drawProjectile(projectiles[i]);

        // ===== UI =====
        drawDefensePreview();
        drawCards();
        drawCoinCounter();
        drawWaveIndicator();
        drawWaveBanner();

        if (gameState == STATE_WON || gameState == STATE_LOST)
            drawEndScreen();

        if (gameState == STATE_PAUSED)
            drawPauseMenu();

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    cleanup();
    return 0;
}

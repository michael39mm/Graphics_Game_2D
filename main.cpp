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

// Makan el offset fel shader (3ashan nersem ay 7aga fe ay makan)
GLint OffsetLocation = -1;

// Makan el redAmount fel shader (2ad eh el 7aga ma7mara men el darb)
GLint RedLocation = -1;

// Amaken el lanes (nos kol shar3 men fo2 le ta7t)
float LANE_Y[4] = {0.41f, 0.03f, -0.35f, -0.73f};
float LANE_HEIGHT = 0.28f;
float LANE_LEFT = -0.925f;
float LANE_RIGHT = 0.925f;

// El lane elly el mouse 3aleha delwa2ty (-1 = mafeesh)
int hoveredLane = -1;

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

// Kart el player: kol el ma3loomat 3an player wa7ed
struct Player {
    int type;
    int lane;
    float x;
    int health;
    int maxHealth;
    float attackTimer;
    float jumpTimer;  // fadel kam sanya fel natta (0 = mesh nate)
    float duckTimer;  // fadel kam sanya fel duck (0 = mesh 3amel duck)

    // El animation (el rasm bas - mafeesh 3alaqa bel mechanics)
    int animState;    // el animation elly shaghala (ANIM_IDLE, ANIM_WALK, ...)
    float animTime;   // men emta bada2t el animation di (bel sanya)
    bool attacked;    // darab fel frame da (3ashan nebda2 animation el darb)
};

// Kart el enemy
struct Enemy {
    int type;
    int lane;
    float x;
    int health;
    int maxHealth;
    float attackTimer;

    // El animation (el rasm bas - mafeesh 3alaqa bel mechanics)
    int animState;
    float animTime;
    bool attacked;
};

// Kol el players w el enemies elly fel game
std::vector<Player> players;
std::vector<Enemy> enemies;

// Sor3et el 7araka (kam unit fel sanya)
float PLAYER_SPEED     = 0.12f;
float ENEMY_SPEED      = 0.08f;
float HARD_ENEMY_SPEED = 0.14f;  // el hard bygry

// El natta w el duck byo2a3do 2ad eh w el player byetla3 2ad eh
float JUMP_TIME   = 0.6f;
float DUCK_TIME   = 0.6f;
float JUMP_HEIGHT = 0.08f;

// Law el masafa benhom a2al men keda yeb2o "5abato" f ba3d
float TOUCH_DISTANCE = 0.07f;

// ===== El waves =====

// 3add el enemies fe kol wave (1: easy bas, 2: easy + medium, 3: el talata)
int WAVE_SIZE[3] = {3, 5, 10};

// Kol kam sanya yenzel enemy fe kol wave (el wave el a5eera asra3)
float WAVE_SPAWN_TIME[3] = {5.0f, 4.0f, 3.0f};

// El ra7a ben kol wave w el tanya (bel sanya)
float WAVE_BREAK_TIME = 4.0f;

int currentWave = 0;           // 0, 1, 2
int enemiesSpawnedInWave = 0;  // nazal kam enemy fel wave di le7ad delwa2ty
float waveSpawnTimer = 3.0f;   // awel enemy byenzel ba3d 2 sanya
float waveBreakTimer = 0.0f;
bool allWavesDone = false;     // el 3 waves 5elso w kol el enemies matet

// ===== 7alet el game =====
const int STATE_PLAYING = 0;
const int STATE_WON     = 1;
const int STATE_LOST    = 2;
const int STATE_MENU    = 3;  // el main menu
const int STATE_PAUSED  = 4;  // dosna Esc

// El game bybda2 fel main menu
int gameState = STATE_MENU;

// Zorar el win/lose screen (Restart fo2, Main Menu ta7t)
float END_BUTTON_Y[2] = {-0.1f, -0.27f};

// 7agm kol zarayer el menus
float END_BUTTON_W = 0.56f;
float END_BUTTON_H = 0.12f;

// Zarayer el main menu (Start, Exit)
float MAIN_MENU_BUTTON_Y[2] = {-0.05f, -0.23f};

// Zarayer el pause menu (Continue, Restart, Main Menu)
float PAUSE_BUTTON_Y[3] = {0.02f, -0.15f, -0.32f};

// Law enemy wesel lel home base
bool gameLost = false;

// ===== Coins w el cards =====

// El coins elly bnebda2 beeha (30 = 3 melee bel zabt) - 500 lel testing bas
int START_COINS = 500;

// El coins elly ma3ana delwa2ty
int coins = START_COINS;

// Se3r kol player
int MELEE_COST   = 10;
int RANGED_COST  = 20;
int DEFENSE_COST = 30;

// Awel player lazem yeb2a melee - ba3d keda kol el cards maftoo7a law el coins kfaya
bool firstMeleePlaced = false;

// Dosna 3ala card el defense w mestaneyeen click 3ala makan fel lane
bool placingDefense = false;

// Kol enemy by2a3 kam coin lama ymoot
int EASY_DROP   = 10;
int MEDIUM_DROP = 15;
int HARD_DROP   = 20;

// Kart el coin elly wa2e3 3al ard
struct Coin {
    int lane;
    float x;
    int value;
};

// Kol el coins elly wa2e3a fel shawa3
std::vector<Coin> droppedCoins;

// Amaken el cards fo2 3al shmal (el x bta3 kol card bel tarteeb: melee, ranged, defense)
float CARD_X[3] = {-0.82f, -0.62f, -0.42f};
float CARD_Y = 0.86f;
float CARD_W = 0.16f;
float CARD_H = 0.22f;

// Anwa3 el talakat (men anhy ertefa3)
const int SHOT_MIDDLE = 0;  // talka el player el ranged
const int SHOT_HIGH   = 1;  // men fo2 (el player lazem duck)
const int SHOT_LOW    = 2;  // men ta7t (el player lazem jump)

// Kart el talka
struct Projectile {
    int lane;
    float x;
    float speed;      // + yemeen, - shmal
    int damage;
    bool fromPlayer;  // talka bta3et player wala enemy
    int height;
    bool hit;         // 5abatet f 7aga aw 5aragat barra
};

// Kol el talakat elly fel game
std::vector<Projectile> projectiles;

// Sor3et el talka w kol 2ad eh kol wa7ed yedrab (bel sanya)
float PROJECTILE_SPEED   = 0.5f;
float MELEE_ATTACK_TIME  = 1.0f;
float RANGED_ATTACK_TIME = 1.5f;
float ENEMY_SHOOT_TIME   = 2.0f;


// Vertex shader
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

// Fragment shader
const char* FragmentShaderCode = R"(
#version 330 core
in vec3 vertexColor;
out vec4 colors;

uniform float redAmount;

void main() {
    colors = vec4(mix(vertexColor, vec3(1.0, 0.0, 0.0), redAmount), 1.0);
}
)";

// ==========================================
// El sprites (el animations men el atlases)
// ==========================================

// Shader el sprites: nafs fekret el rectangle bas bel soora (uv) badal el lon
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
uniform float redAmount;   // el darb: kol ma yezeed yeb2a a7mar
uniform float greyAmount;  // el card el locked: ramady

void main() {
    // El rgb hena already marsoom fe el alpha (3ashan el 7awaf)
    vec4 c = texture(atlas, uv);

    float grey = dot(c.rgb, vec3(0.3, 0.59, 0.11)) * 0.7;
    vec3 rgb = mix(c.rgb, vec3(grey), greyAmount);
    rgb = mix(rgb, vec3(1.0, 0.0, 0.0) * c.a, redAmount);

    colors = vec4(rgb, c.a);
}
)";

// El row bta3 kol animation gowa el atlas (nafs el tarteeb fe kol el atlases)
const int ANIM_IDLE   = 0;
const int ANIM_WALK   = 1;
const int ANIM_RUN    = 2;
const int ANIM_CROUCH = 3;
const int ANIM_JUMP   = 4;
const int ANIM_ATTACK = 5;

// Kol animation feeha 8 frames
const int ANIM_FRAMES = 8;

// Anwa3 el sprites (kol wa7ed leh atlas)
const int SPRITE_ATREIDES = 0;  // player melee
const int SPRITE_FREMEN   = 1;  // player ranged
const int SPRITE_PILOT    = 2;  // enemy easy (byedrab men fo2)
const int SPRITE_BRUTE    = 3;  // enemy medium (byedrab men ta7t)
const int SPRITE_OFFICER  = 4;  // enemy hard (melee)
const int SPRITE_COUNT    = 5;

// Kol el ma3loomat 3an atlas wa7ed (da elly kan mo5talef bein el demos)
struct SpriteInfo {
    const char* file;
    int cellW;          // 3ard el frame (bel pixels)
    int cellH;          // tool el frame
    float anchorX;      // makan el rgleen gowa el frame (x)
    float footY[6];     // makan el rgleen gowa el frame (y) le kol animation
    float fps[6];       // sor3et kol animation (frame fel sanya)
    int hitFrame;       // el frame elly el darba btewsal feeha (el talka btetla3 / el sayf yedrab)
    float muzzleDY;     // ertefa3 el mawsoura foo2 el rgleen (lel talakat)

    // Bet3abba lama el game yeshta3'al
    GLuint texture;
    int imageW;
    int imageH;
};

SpriteInfo sprites[SPRITE_COUNT] = {
    // Atreides (melee player)
    { "assets/atreides_atlas.png", 181, 181, 95.0f,
      {160, 160, 160, 160, 160, 160},
      {6, 14, 18, 18, 10, 14},
      4, 0.0f, 0, 0, 0 },

    // Fremen (ranged player)
    { "assets/fremen_atlas.png", 181, 181, 91.0f,
      {160, 160, 160, 160, 160, 160},
      {6, 13, 18, 18, 10, 12},
      4, 80.0f, 0, 0, 0 },

    // Pilot (enemy easy)
    { "assets/pilot_atlas.png", 181, 181, 94.0f,
      {158, 158, 157, 159, 158, 158},
      {8, 15, 18, 18, 10, 12},
      4, 77.0f, 0, 0, 0 },

    // Brute (enemy medium)
    { "assets/brute2_atlas.png", 181, 181, 96.0f,
      {161, 158, 156, 159, 160, 158},
      {6, 11, 14, 14, 9, 9},
      4, 32.0f, 0, 0, 0 },

    // Officer (enemy hard)
    { "assets/officer_atlas.png", 201, 181, 98.0f,
      {158, 158, 155, 157, 157, 158},
      {6, 12, 16, 18, 10, 12},
      4, 0.0f, 0, 0, 0 },
};

// Takbeer el sprite (1.0 = el sora zay ma heya bel pixels, 0.5 = nos)
float SPRITE_SCALE = 0.6f;

// Takbeer el sprite gowa el cards (asghar men el game)
float CARD_SPRITE_SCALE = 0.4f;

// El rgleen ta7t nos el lane be kam (3ashan el rasm yeb2a gowa el lane)
float FEET_OFFSET = 0.13f;

// Shader el sprites w el VAO/VBO bto3o
GLuint TexProgram = 0;
GLuint TexVAO = 0;
GLuint TexVBO = 0;
GLint TexRedLocation = -1;
GLint TexGreyLocation = -1;

// Anhy sprite lel player da (-1 law mafeesh sprite leh, zay el defense)
int playerSprite(int type) {
    if (type == PLAYER_MELEE)
        return SPRITE_ATREIDES;

    if (type == PLAYER_RANGED)
        return SPRITE_FREMEN;

    return -1;
}

// Anhy sprite lel enemy da
int enemySprite(int type) {
    if (type == ENEMY_EASY)
        return SPRITE_PILOT;

    if (type == ENEMY_MEDIUM)
        return SPRITE_BRUTE;

    return SPRITE_OFFICER;
}

// Nload el atlas men el file l el vram (bnegarrab el path men aktar men makan)
bool loadSpriteTexture(SpriteInfo& s) {
    stbi_set_flip_vertically_on_load(false);

    const char* prefixes[] = { "", "../", "../../", "../../../" };
    unsigned char* pixels = nullptr;
    int channels = 0;

    for (int i = 0; i < 4 && pixels == nullptr; i++) {
        std::string path = std::string(prefixes[i]) + s.file;
        pixels = stbi_load(path.c_str(), &s.imageW, &s.imageH, &channels, 4);
    }

    if (pixels == nullptr) {
        std::cerr << "Could not load " << s.file << ": "
                  << stbi_failure_reason() << '\n';
        return false;
    }

    // Narsom el rgb fel alpha (premultiply) 3ashan el 7awaf mayb2ash feeha lon waser
    for (int i = 0; i < s.imageW * s.imageH; i++) {
        unsigned char* p = pixels + i * 4;
        p[0] = (unsigned char)(p[0] * p[3] / 255);
        p[1] = (unsigned char)(p[1] * p[3] / 255);
        p[2] = (unsigned char)(p[2] * p[3] / 255);
    }

    glGenTextures(1, &s.texture);
    glBindTexture(GL_TEXTURE_2D, s.texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s.imageW, s.imageH, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // El sora hatetersem asghar men el asl: mipmaps 3ashan tebqa nadeefa
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(pixels);
    return true;
}

// Nload kol el atlases (law wa7ed fashal, el game beyersem rectangle badalo)
void loadSprites() {
    for (int i = 0; i < SPRITE_COUNT; i++)
        loadSpriteTexture(sprites[i]);
}

// Anhy frame el mafrood yetersem (el loop lel idle/walk/run, w el ba2y yewa2af 3and a5er frame)
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

    // El UV: anhy gozaa men el atlas (row = el animation, column = el frame)
    float u0 = (float)(frame * s.cellW) / s.imageW;
    float u1 = (float)((frame + 1) * s.cellW) / s.imageW;
    float v0 = (float)(anim * s.cellH) / s.imageH;
    float v1 = (float)((anim + 1) * s.cellH) / s.imageH;

    // Law rayeh shimal: nel2eb el UV w el rgleen yeb2o men el naha el tanya
    float anchorX = s.anchorX;
    if (!faceRight) {
        float t = u0;
        u0 = u1;
        u1 = t;
        anchorX = s.cellW - s.anchorX;
    }

    // Men pixels le coordinates el shasha (-1 le 1), bel nesba lel width w el height
    // El sprite beyetkabbar ma3 el window (600 = tool el window el asleya)
    float windowScale = scale * height / 600.0f;
    float pxX = windowScale * 2.0f / width;
    float pxY = windowScale * 2.0f / height;

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.texture);

    // El sora marsooma fel alpha: nesta5dem blend mo5talef lel sprites bas
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(TexVAO);
    glBindBuffer(GL_ARRAY_BUFFER, TexVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Nraga3 kol 7aga zay ma kant lel rectangles
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ShaderProgram);
}

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

// Edeeny y w ana a2olak anhy lane (-1 law mafeesh)
int laneAtY(float y) {
    for (int i = 0; i < 4; i++) {
        float top    = LANE_Y[i] + LANE_HEIGHT / 2.0f;
        float bottom = LANE_Y[i] - LANE_HEIGHT / 2.0f;

        if (y <= top && y >= bottom)
            return i;
    }

    return -1;
}

// El mouse 3ala anhy lane delwa2ty
int getMouseLane() {
    double mouseX, mouseY;
    glfwGetCursorPos(Window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(Window, &windowWidth, &windowHeight);

    // Ne7awel men pixels le OpenGL (-1 le 1)
    float x = (float)(mouseX / windowWidth) * 2.0f - 1.0f;
    float y = 1.0f - (float)(mouseY / windowHeight) * 2.0f;

    // Law barra el shawa3 men el gamb
    if (x < LANE_LEFT || x > LANE_RIGHT)
        return -1;

    return laneAtY(y);
}

// Makan el mouse b OpenGL (-1 le 1)
void getMousePosition(float* x, float* y) {
    double mouseX, mouseY;
    glfwGetCursorPos(Window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(Window, &windowWidth, &windowHeight);

    *x = (float)(mouseX / windowWidth) * 2.0f - 1.0f;
    *y = 1.0f - (float)(mouseY / windowHeight) * 2.0f;
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

void buyPlayers(int type);
void restartGame();
int endButtonAt(float x, float y);
int buttonAt(float x, float y, float* buttonY, int count);
void startGame();
void goToMainMenu();
void placeDefense(int lane, float x);

// Lama nedoos click
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Click 3ala card: nesht3ry el player da fe kol el lanes el selected
        float mx, my;
        getMousePosition(&mx, &my);

        // Fel main menu: Start aw Exit
        if (gameState == STATE_MENU) {
            int button = buttonAt(mx, my, MAIN_MENU_BUTTON_Y, 2);

            if (button == 0)
                startGame();

            if (button == 1)
                glfwSetWindowShouldClose(Window, true);

            return;
        }

        // Fel pause: Continue, Restart, Main Menu
        if (gameState == STATE_PAUSED) {
            int button = buttonAt(mx, my, PAUSE_BUTTON_Y, 3);

            if (button == 0)
                gameState = STATE_PLAYING;

            if (button == 1)
                restartGame();

            if (button == 2)
                goToMainMenu();

            return;
        }

        // El game 5eles (win aw lose): Restart aw Main Menu
        if (gameState == STATE_WON || gameState == STATE_LOST) {
            int button = endButtonAt(mx, my);

            if (button == 0)
                restartGame();

            if (button == 1)
                goToMainMenu();

            return;
        }

        int card = cardAt(mx, my);
        if (card != -1) {
            buyPlayers(card);
            return;
        }

        // Mestaneyeen makan el defense: el click da byeb2a makano
        // (law el click barra el shawa3 byelghy)
        if (placingDefense) {
            int defenseLane = getMouseLane();
            if (defenseLane != -1)
                placeDefense(defenseLane, mx);

            placingDefense = false;
            return;
        }

        int lane = getMouseLane();

        // Click 3ala lane: law selected teb2a la2, law la2 teb2a selected
        if (lane != -1)
            selectedLanes[lane] = !selectedLanes[lane];
    }
}

void spawnPlayer(int type, int lane);
void spawnEnemy(int type, int lane);

// W/S = natta w duck | Esc = pause
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    // Esc: pause law bnel3ab, w continue law fel pause
    if (key == GLFW_KEY_ESCAPE) {
        if (gameState == STATE_PLAYING)
            gameState = STATE_PAUSED;
        else if (gameState == STATE_PAUSED)
            gameState = STATE_PLAYING;

        return;
    }

    // Mesh bnel3ab (menu, pause, win, lose): mafeesh natta wala duck
    if (gameState != STATE_PLAYING)
        return;

    // W / Up = natta, S / Down = duck (lel players fe kol el lanes el selected)
    bool jumpKey = (key == GLFW_KEY_W || key == GLFW_KEY_UP);
    bool duckKey = (key == GLFW_KEY_S || key == GLFW_KEY_DOWN);

    if (!jumpKey && !duckKey)
        return;

    for (int i = 0; i < (int)players.size(); i++) {
        // El defense mabyenotsh w mabye3melsh duck
        if (players[i].type == PLAYER_DEFENSE)
            continue;

        if (!selectedLanes[players[i].lane])
            continue;

        // Law howa aslan nate aw 3amel duck, yekamel elly by3mlo
        if (players[i].jumpTimer > 0.0f || players[i].duckTimer > 0.0f)
            continue;

        if (jumpKey)
            players[i].jumpTimer = JUMP_TIME;
        else
            players[i].duckTimer = DUCK_TIME;
    }
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

    // Blending 3ashan el 7awaf el shafafa fel sprites matetersemsh sooda
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glfwSetFramebufferSizeCallback(Window, framebufferSizeCallback);
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

    // Ngeeb makan el offset mn el shader
    OffsetLocation = glGetUniformLocation(ShaderProgram, "offset");
    RedLocation = glGetUniformLocation(ShaderProgram, "redAmount");

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

    // ===== Shader el sprites =====
    GLuint TexVertexShader =
        compileShader(GL_VERTEX_SHADER, TexVertexShaderCode);

    GLuint TexFragmentShader =
        compileShader(GL_FRAGMENT_SHADER, TexFragmentShaderCode);

    if (TexVertexShader == 0 || TexFragmentShader == 0) {
        if (TexVertexShader != 0)
            glDeleteShader(TexVertexShader);

        if (TexFragmentShader != 0)
            glDeleteShader(TexFragmentShader);

        return false;
    }

    TexProgram = glCreateProgram();

    glAttachShader(TexProgram, TexVertexShader);
    glAttachShader(TexProgram, TexFragmentShader);
    glLinkProgram(TexProgram);

    glGetProgramiv(TexProgram, GL_LINK_STATUS, &success);

    if (!success) {
        char log[512];

        glGetProgramInfoLog(TexProgram, 512, nullptr, log);
        std::cerr << "Sprite shader link error:\n" << log << '\n';

        glDeleteShader(TexVertexShader);
        glDeleteShader(TexFragmentShader);
        glDeleteProgram(TexProgram);

        TexProgram = 0;
        return false;
    }

    glDeleteShader(TexVertexShader);
    glDeleteShader(TexFragmentShader);

    TexRedLocation = glGetUniformLocation(TexProgram, "redAmount");
    TexGreyLocation = glGetUniformLocation(TexProgram, "greyAmount");

    // El atlas fel texture unit 0
    glUseProgram(TexProgram);
    glUniform1i(glGetUniformLocation(TexProgram, "atlas"), 0);
    glUseProgram(0);

    // VAO/VBO lel sprites: 4 vertices, kol wa7ed (x, y, u, v)
    glGenVertexArrays(1, &TexVAO);
    glGenBuffers(1, &TexVBO);

    glBindVertexArray(TexVAO);

    glBindBuffer(GL_ARRAY_BUFFER, TexVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        4 * 4 * sizeof(float),
        nullptr,
        GL_DYNAMIC_DRAW
    );

    // Nafs el EBO bta3 el rectangles (nafs el 6 indices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Position attribute: x, y
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // UV attribute: u, v
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)(2 * sizeof(float)));
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

    for (int i = 0; i < SPRITE_COUNT; i++) {
        if (sprites[i].texture != 0)
            glDeleteTextures(1, &sprites[i].texture);
    }

    if (TexVBO != 0)
        glDeleteBuffers(1, &TexVBO);

    if (TexVAO != 0)
        glDeleteVertexArrays(1, &TexVAO);

    if (TexProgram != 0)
        glDeleteProgram(TexProgram);

    if (ShaderProgram != 0)
        glDeleteProgram(ShaderProgram);

    if (Window != nullptr) {
        glfwDestroyWindow(Window);
        Window = nullptr;
    }

    glfwTerminate();
}



// Kol el rasm elly ba3daha yet7arak b (x, y)
void setOffset(float x, float y) {
    glUniform2f(OffsetLocation, x, y);
}

// 2ad eh el rasm elly ba3daha yeb2a a7mar (0 = 3ady, 1 = a7mar 5ales)
void setRed(float amount) {
    glUniform1f(RedLocation, amount);
}


// Nersem bervaz 7awalen el lane (4 rectangles rofa3)
void drawLaneHighlight(int lane, float thickness,
                       float r, float g, float b) {
    float laneWidth = LANE_RIGHT - LANE_LEFT;
    float top    = LANE_Y[lane] + LANE_HEIGHT / 2.0f;
    float bottom = LANE_Y[lane] - LANE_HEIGHT / 2.0f;

    // Fo2 w ta7t
    drawRectangle(laneWidth, thickness, 0.0f, top, r, g, b);
    drawRectangle(laneWidth, thickness, 0.0f, bottom, r, g, b);

    // Shmal w yemeen
    drawRectangle(thickness, LANE_HEIGHT, LANE_LEFT, LANE_Y[lane], r, g, b);
    drawRectangle(thickness, LANE_HEIGHT, LANE_RIGHT, LANE_Y[lane], r, g, b);
}


// Nzawed player gdeed fe awel el lane
void spawnPlayer(int type, int lane) {
    Player p;
    p.type = type;
    p.lane = lane;
    p.x = LANE_LEFT + 0.05f;
    p.attackTimer = 0.0f;
    p.jumpTimer = 0.0f;
    p.duckTimer = 0.0f;
    p.animState = ANIM_IDLE;
    p.animTime = 0.0f;
    p.attacked = false;

    if (type == PLAYER_DEFENSE)
        p.maxHealth = 7;
    else
        p.maxHealth = 5;

    p.health = p.maxHealth;
    players.push_back(p);
}

// Nzawed enemy gdeed fe a5er el lane (men el yemeen)
void spawnEnemy(int type, int lane) {
    Enemy e;
    e.type = type;
    e.lane = lane;
    e.x = LANE_RIGHT - 0.05f;
    e.attackTimer = 0.0f;
    e.animState = ANIM_IDLE;
    e.animTime = 0.0f;
    e.attacked = false;

    if (type == ENEMY_HARD)
        e.maxHealth = 7;
    else
        e.maxHealth = 5;

    e.health = e.maxHealth;
    enemies.push_back(e);
}

// El card da unlocked wala la2
// (el melee daymann maftoo7 - el ba2y yetfete7o ba3d awel melee)
bool isUnlocked(int type) {
    if (type == PLAYER_MELEE)
        return true;

    return firstMeleePlaced;
}

// Se3r el player
int playerCost(int type) {
    if (type == PLAYER_MELEE)
        return MELEE_COST;

    if (type == PLAYER_RANGED)
        return RANGED_COST;

    return DEFENSE_COST;
}

// Nesht3ry player wa7ed fe kol lane selected (law el card unlocked w ma3ana coins)
void buyPlayers(int type) {
    if (!isUnlocked(type) || coins < playerCost(type))
        return;

    // El defense: mesh bnet7at fel lanes el selected - bnestana click 3ala makan
    // (click tany 3ala nafs el card byelghy)
    if (type == PLAYER_DEFENSE) {
        placingDefense = !placingDefense;
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (!selectedLanes[i])
            continue;

        // Bnetshayek kol mara 3ashan el coins bte2el w momken el card ye2fel
        if (!isUnlocked(type) || coins < playerCost(type))
            return;

        coins -= playerCost(type);
        spawnPlayer(type, i);

        if (type == PLAYER_MELEE)
            firstMeleePlaced = true;
    }
}

// Ne7ot defense fe el lane da fe makan el click bel zabt
void placeDefense(int lane, float x) {
    if (!isUnlocked(PLAYER_DEFENSE) || coins < DEFENSE_COST)
        return;

    // Mayetla3sh barra el shar3
    if (x < LANE_LEFT + 0.05f)
        x = LANE_LEFT + 0.05f;
    if (x > LANE_RIGHT - 0.05f)
        x = LANE_RIGHT - 0.05f;

    coins -= DEFENSE_COST;
    spawnPlayer(PLAYER_DEFENSE, lane);

    // spawnPlayer bey7oto fel awel - n7oto fe makan el click
    players.back().x = x;
}

// El enemy mat: yo2a3 coin makano
void dropCoin(Enemy e) {
    Coin c;
    c.lane = e.lane;
    c.x = e.x;

    if (e.type == ENEMY_EASY)
        c.value = EASY_DROP;
    else if (e.type == ENEMY_MEDIUM)
        c.value = MEDIUM_DROP;
    else
        c.value = HARD_DROP;

    droppedCoins.push_back(c);
}

// Nshel ay 7ad health bta3o 5eles
// (bnemshy men el a5er lel awel 3ashan el erase maybawazsh el 3ad)
void removeDeadEntities() {
    for (int i = (int)players.size() - 1; i >= 0; i--) {
        if (players[i].health <= 0)
            players.erase(players.begin() + i);
    }

    for (int i = (int)enemies.size() - 1; i >= 0; i--) {
        if (enemies[i].health <= 0) {
            dropCoin(enemies[i]);
            enemies.erase(enemies.begin() + i);
        }
    }
}

// Bar el health fo2 el character (el ge2 el malyan = el health el fadel)
void drawHealthBar(float x, float y, int health, int maxHealth,
                   float r, float g, float b) {
    float barWidth = 0.08f;
    float barHeight = 0.012f;

    float fill = (float)health / maxHealth;
    if (fill < 0.0f)
        fill = 0.0f;

    // El 5alfeya el gham2a (el bar kolo)
    drawRectangle(barWidth, barHeight, x, y, 0.15f, 0.15f, 0.15f);

    // El ge2 el malyan bybda2 men el shmal
    float fillWidth = barWidth * fill;
    drawRectangle(fillWidth, barHeight,
                  x - barWidth / 2.0f + fillWidth / 2.0f, y,
                  r, g, b);
}

// Shakl kol no3 player (3ard, tool, lon) - bnesta5demha fel game w fel cards
void getPlayerLook(int type, float* w, float* h, float* r, float* g, float* b) {
    *w = 0.06f; *h = 0.16f;
    *r = 0.2f; *g = 0.8f; *b = 0.3f;

    if (type == PLAYER_RANGED) {
        *w = 0.05f; *h = 0.18f;
        *r = 0.2f; *g = 0.6f; *b = 1.0f;
    }
    else if (type == PLAYER_DEFENSE) {
        *w = 0.09f; *h = 0.22f;
        *r = 0.6f; *g = 0.6f; *b = 0.6f;
    }
}

// Rasm el player bel rectangle (lel defense, w law el sprite mat-load-ash)
void drawPlayerRectangle(Player p) {
    // Law nate: byetla3 w yenzel (sin bta3mel 2os)
    float jumpY = 0.0f;
    if (p.jumpTimer > 0.0f) {
        float progress = 1.0f - p.jumpTimer / JUMP_TIME;
        jumpY = JUMP_HEIGHT * sin(progress * 3.14159f);
    }

    setOffset(p.x, LANE_Y[p.lane] + jumpY);

    // Kol ma el health ye2el yeb2a a7mar aktar
    setRed((1.0f - (float)p.health / p.maxHealth) * 0.8f);

    // Shakl kol no3 (3ard, tool, lon)
    float w, h, r, g, b;
    getPlayerLook(p.type, &w, &h, &r, &g, &b);

    // Law 3amel duck: el tool yeb2a nos w regleeh fe nafs el makan
    float drawH = h;
    float drawY = 0.0f;
    if (p.duckTimer > 0.0f) {
        drawH = h * 0.5f;
        drawY = -h * 0.25f;
    }

    drawRectangle(w, drawH, 0.0f, drawY, r, g, b);

    setRed(0.0f);
    setOffset(0.0f, 0.0f);

    // Bar a5dar fo2 el player (byetla3 ma3ah law nate)
    drawHealthBar(p.x, LANE_Y[p.lane] + 0.125f + jumpY, p.health, p.maxHealth,
                  0.2f, 0.9f, 0.3f);
}

// Rasm el enemy bel rectangle (law el sprite mat-load-ash)
void drawEnemyRectangle(Enemy e) {
    setOffset(e.x, LANE_Y[e.lane]);

    // Kol ma el health ye2el yeb2a a7mar aktar
    setRed((1.0f - (float)e.health / e.maxHealth) * 0.8f);


    if (e.type == ENEMY_EASY)
        drawRectangle(0.06f, 0.16f, 0.0f, 0.0f, 0.8f, 0.3f, 0.8f);
    else if (e.type == ENEMY_MEDIUM)
        drawRectangle(0.06f, 0.16f, 0.0f, 0.0f, 0.9f, 0.5f, 0.1f);
    else
        drawRectangle(0.08f, 0.20f, 0.0f, 0.0f, 0.9f, 0.1f, 0.1f);

    setRed(0.0f);
    setOffset(0.0f, 0.0f);

    // Bar bortoo2any fo2 el enemy
    drawHealthBar(e.x, LANE_Y[e.lane] + 0.125f, e.health, e.maxHealth,
                  1.0f, 0.55f, 0.1f);
}


// Rasm el player (sprite law leh, law la2 rectangle)
void drawPlayer(Player p) {
    int sprite = playerSprite(p.type);

    // El defense mafeesh leh sprite, w law el atlas mat-load-ash nersem el rectangle
    if (sprite == -1 || sprites[sprite].texture == 0) {
        drawPlayerRectangle(p);
        return;
    }

    // Law nate: byetla3 w yenzel (nafs el sin bta3 el rectangle)
    float jumpY = 0.0f;
    int anim = p.animState;
    int frame = spriteFrame(sprite, anim, p.animTime);

    if (p.jumpTimer > 0.0f) {
        float progress = 1.0f - p.jumpTimer / JUMP_TIME;
        jumpY = JUMP_HEIGHT * sin(progress * 3.14159f);

        // El natta: el frame beyetghayar 7asab el wa2t fel hawa
        anim = ANIM_JUMP;
        frame = (int)(progress * ANIM_FRAMES);
        if (frame >= ANIM_FRAMES)
            frame = ANIM_FRAMES - 1;
    }

    // Kol ma el health ye2el yeb2a a7mar aktar
    float red = (1.0f - (float)p.health / p.maxHealth) * 0.8f;

    drawSprite(sprite, anim, frame, p.x, LANE_Y[p.lane] - FEET_OFFSET + jumpY,
               true, red, 0.0f, SPRITE_SCALE);

    // Bar a5dar fo2 el player (byetla3 ma3ah law nate)
    drawHealthBar(p.x, LANE_Y[p.lane] + 0.17f + jumpY, p.health, p.maxHealth,
                  0.2f, 0.9f, 0.3f);
}

// Rasm el enemy (sprite law leh, law la2 rectangle) - el enemies bytsh shmal
void drawEnemy(Enemy e) {
    int sprite = enemySprite(e.type);

    if (sprites[sprite].texture == 0) {
        drawEnemyRectangle(e);
        return;
    }

    int frame = spriteFrame(sprite, e.animState, e.animTime);

    // Kol ma el health ye2el yeb2a a7mar aktar
    float red = (1.0f - (float)e.health / e.maxHealth) * 0.8f;

    drawSprite(sprite, e.animState, frame, e.x, LANE_Y[e.lane] - FEET_OFFSET,
               false, red, 0.0f, SPRITE_SCALE);

    // Bar bortoo2any fo2 el enemy
    drawHealthBar(e.x, LANE_Y[e.lane] + 0.17f, e.health, e.maxHealth,
                  1.0f, 0.55f, 0.1f);
}


// Etnen fe nafs el lane w 2orayyebeen men ba3d?
bool isTouching(float x1, float x2) {
    return fabs(x1 - x2) < TOUCH_DISTANCE;
}

// Fe enemy 5abat fel player da?
bool playerIsBlocked(Player p) {
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (enemies[i].lane == p.lane && isTouching(p.x, enemies[i].x))
            return true;
    }

    return false;
}

// Fe player 5abat fel enemy da?
bool enemyIsBlocked(Enemy e) {
    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].lane == e.lane && isTouching(e.x, players[i].x))
            return true;
    }

    return false;
}

// Nem4y el players le el yemeen
void updatePlayers() {
    for (int i = 0; i < (int)players.size(); i++) {
        // Ne2alel wa2t el natta w el duck
        if (players[i].jumpTimer > 0.0f)
            players[i].jumpTimer -= deltaTime;

        if (players[i].duckTimer > 0.0f)
            players[i].duckTimer -= deltaTime;

        // El defense mabyet7arakshi 5ales
        if (players[i].type == PLAYER_DEFENSE)
            continue;

        // Law 5abat fe enemy ye2af
        if (playerIsBlocked(players[i]))
            continue;

        players[i].x += PLAYER_SPEED * deltaTime;

        // Wesel a5er el shar3: ye5rog men el game
        if (players[i].x > LANE_RIGHT)
            players[i].health = 0;
    }
}

// Nem4y el enemies le el shmal
void updateEnemies() {
    for (int i = 0; i < (int)enemies.size(); i++) {
        // Law 5abat fe player ye2af
        if (enemyIsBlocked(enemies[i]))
            continue;

        if (enemies[i].type == ENEMY_HARD)
            enemies[i].x -= HARD_ENEMY_SPEED * deltaTime;
        else
            enemies[i].x -= ENEMY_SPEED * deltaTime;

        // Wesel lel home base: 5esrna
        if (enemies[i].x < LANE_LEFT) {
            enemies[i].x = LANE_LEFT;

            if (!gameLost) {
                gameLost = true;
                gameState = STATE_LOST;
                std::cout << "Enemy wesel lel home base!\n";
            }
        }
    }
}

// El player da wara defense fe nafs el lane? (yeb2a ma7mee men el enemies)
bool isProtected(Player p) {
    // El defense nafso mesh ma7mee
    if (p.type == PLAYER_DEFENSE)
        return false;

    for (int i = 0; i < (int)players.size(); i++) {
        if (players[i].type != PLAYER_DEFENSE || players[i].health <= 0)
            continue;

        // Fe defense oddamo (aw fe nafs makano) fe nafs el lane
        if (players[i].lane == p.lane && p.x <= players[i].x)
            return true;
    }

    return false;
}

// Anhy enemy 5abat fel player da (-1 law mafeesh)
int findTouchingEnemy(Player p) {
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (enemies[i].lane == p.lane && isTouching(p.x, enemies[i].x))
            return i;
    }

    return -1;
}

// Anhy player 5abat fel enemy da (-1 law mafeesh)
int findTouchingPlayer(Enemy e) {
    for (int i = 0; i < (int)players.size(); i++) {
        // El player el ma7mee wara defense mayendrabsh
        if (players[i].lane == e.lane && isTouching(e.x, players[i].x) &&
            !isProtected(players[i]))
            return i;
    }

    return -1;
}

// Ntala3 talka gdeeda
void fireProjectile(int lane, float x, float speed, int damage,
                    bool fromPlayer, int height) {
    Projectile s;
    s.lane = lane;
    s.x = x;
    s.speed = speed;
    s.damage = damage;
    s.fromPlayer = fromPlayer;
    s.height = height;
    s.hit = false;
    projectiles.push_back(s);
}

// El players yedrabo (melee men 2orayeb, ranged yetala3 talakat)
void updatePlayerAttacks() {
    for (int i = 0; i < (int)players.size(); i++) {
        // El defense mabyedrabsh
        if (players[i].type == PLAYER_DEFENSE)
            continue;

        players[i].attackTimer += deltaTime;

        if (players[i].type == PLAYER_MELEE) {
            int e = findTouchingEnemy(players[i]);

            // Mafeesh enemy: yefdal gahez yedrab awel ma yelmes
            if (e == -1) {
                if (players[i].attackTimer > MELEE_ATTACK_TIME)
                    players[i].attackTimer = MELEE_ATTACK_TIME;
            }
            else if (players[i].attackTimer >= MELEE_ATTACK_TIME) {
                enemies[e].health -= 1;
                players[i].attackTimer = 0.0f;
                players[i].attacked = true;
            }
        }
        else if (players[i].type == PLAYER_RANGED) {
            // Byedrab 3ala tool 7atta law mafeesh enemies
            if (players[i].attackTimer >= RANGED_ATTACK_TIME) {
                players[i].attackTimer -= RANGED_ATTACK_TIME;
                players[i].attacked = true;
                fireProjectile(players[i].lane, players[i].x,
                               PROJECTILE_SPEED, 1, true, SHOT_MIDDLE);
            }
        }
    }
}

// El enemies yedrabo (easy men fo2, medium men ta7t, hard melee)
void updateEnemyAttacks() {
    for (int i = 0; i < (int)enemies.size(); i++) {
        enemies[i].attackTimer += deltaTime;

        if (enemies[i].type == ENEMY_HARD) {
            int p = findTouchingPlayer(enemies[i]);

            if (p == -1) {
                if (enemies[i].attackTimer > MELEE_ATTACK_TIME)
                    enemies[i].attackTimer = MELEE_ATTACK_TIME;
            }
            else if (enemies[i].attackTimer >= MELEE_ATTACK_TIME) {
                // El hard byedrab 2 damage
                players[p].health -= 2;
                enemies[i].attackTimer = 0.0f;
                enemies[i].attacked = true;
            }
        }
        else if (enemies[i].attackTimer >= ENEMY_SHOOT_TIME) {
            enemies[i].attackTimer -= ENEMY_SHOOT_TIME;
            enemies[i].attacked = true;

            int height = SHOT_LOW;
            if (enemies[i].type == ENEMY_EASY)
                height = SHOT_HIGH;

            fireProjectile(enemies[i].lane, enemies[i].x,
                           -PROJECTILE_SPEED, 1, false, height);
        }
    }
}

// El animations bas (mafeesh 3alaqa bel mechanics): kol player byekhtar el animation bta3o
// men el 7aga elly by3melha (natta, duck, darb, wa2ef, mashy)
void updatePlayerAnimations() {
    for (int i = 0; i < (int)players.size(); i++) {
        Player& p = players[i];
        int sprite = playerSprite(p.type);

        // El defense mafeesh leh sprite
        if (sprite == -1)
            continue;

        SpriteInfo& s = sprites[sprite];

        // Kam sanya men awel el darba le7ad el frame elly btewsal feeha
        float windup = s.hitFrame / s.fps[ANIM_ATTACK];

        // El animation el darb lessa shaghala?
        bool attackPlaying = p.animState == ANIM_ATTACK &&
                             p.animTime * s.fps[ANIM_ATTACK] < ANIM_FRAMES;

        int want = ANIM_IDLE;
        float startTime = 0.0f;

        if (p.jumpTimer > 0.0f)
            want = ANIM_JUMP;
        else if (p.duckTimer > 0.0f)
            want = ANIM_CROUCH;
        else if (attackPlaying)
            want = ANIM_ATTACK;
        else {
            // Wa2ef (5abat fe enemy) wala mashy
            if (playerIsBlocked(p))
                want = ANIM_IDLE;
            else
                want = ANIM_WALK;

            // Kam sanya lel darba el gaya (el darba btewsal fel frame el hitFrame)
            float timeLeft = 99.0f;

            if (p.type == PLAYER_RANGED)
                timeLeft = RANGED_ATTACK_TIME - p.attackTimer;
            else if (findTouchingEnemy(p) != -1)
                timeLeft = MELEE_ATTACK_TIME - p.attackTimer;

            // Law darab fel frame da, el darba hasalet delwa2ty
            if (p.attacked)
                timeLeft = 0.0f;

            // Nebda2 el animation 2abl el darba be shwaya 3ashan el darba tewsal fel frame el sa7
            if (timeLeft <= windup) {
                want = ANIM_ATTACK;
                startTime = windup - timeLeft;

                if (startTime > windup)
                    startTime = windup;
            }
        }

        p.attacked = false;

        // Law el animation et3'ayyaret (aw el darb el gedeed), nebda2 men el awel
        if (want != p.animState || (want == ANIM_ATTACK && !attackPlaying)) {
            p.animState = want;
            p.animTime = startTime;
        }
        else
            p.animTime += deltaTime;
    }
}

// Nafs el fekra lel enemies
void updateEnemyAnimations() {
    for (int i = 0; i < (int)enemies.size(); i++) {
        Enemy& e = enemies[i];
        SpriteInfo& s = sprites[enemySprite(e.type)];

        float windup = s.hitFrame / s.fps[ANIM_ATTACK];

        bool attackPlaying = e.animState == ANIM_ATTACK &&
                             e.animTime * s.fps[ANIM_ATTACK] < ANIM_FRAMES;

        int want = ANIM_IDLE;
        float startTime = 0.0f;

        if (attackPlaying)
            want = ANIM_ATTACK;
        else {
            // Wa2ef (5abat fe player), el hard bygry, el ba2y mashy
            if (enemyIsBlocked(e))
                want = ANIM_IDLE;
            else if (e.type == ENEMY_HARD)
                want = ANIM_RUN;
            else
                want = ANIM_WALK;

            float timeLeft = 99.0f;

            if (e.type == ENEMY_HARD) {
                if (findTouchingPlayer(e) != -1)
                    timeLeft = MELEE_ATTACK_TIME - e.attackTimer;
            }
            else
                timeLeft = ENEMY_SHOOT_TIME - e.attackTimer;

            if (e.attacked)
                timeLeft = 0.0f;

            if (timeLeft <= windup) {
                want = ANIM_ATTACK;
                startTime = windup - timeLeft;

                if (startTime > windup)
                    startTime = windup;
            }
        }

        e.attacked = false;

        if (want != e.animState || (want == ANIM_ATTACK && !attackPlaying)) {
            e.animState = want;
            e.animTime = startTime;
        }
        else
            e.animTime += deltaTime;
    }
}

void updateAnimations() {
    updatePlayerAnimations();
    updateEnemyAnimations();
}

// Nem4y el talakat w neshoof 5abatet f 7ad wala la2
void updateProjectiles() {
    for (int i = 0; i < (int)projectiles.size(); i++) {
        projectiles[i].x += projectiles[i].speed * deltaTime;

        if (projectiles[i].fromPlayer) {
            // Talka el player btedrab enemies bas (mabtedrabsh players)
            for (int j = 0; j < (int)enemies.size(); j++) {
                if (enemies[j].lane == projectiles[i].lane &&
                    isTouching(projectiles[i].x, enemies[j].x)) {
                    enemies[j].health -= projectiles[i].damage;
                    projectiles[i].hit = true;
                    break;
                }
            }
        }
        else {
            // Talka el enemy btedrab players
            for (int j = 0; j < (int)players.size(); j++) {
                if (players[j].lane == projectiles[i].lane &&
                    isTouching(projectiles[i].x, players[j].x)) {
                    // El player wara defense: el talka mat5abatosh (el defense hayeshelha)
                    if (isProtected(players[j]))
                        continue;

                    // Talka men fo2 w el player 3amel duck: 3adet men fo2o
                    if (projectiles[i].height == SHOT_HIGH && players[j].duckTimer > 0.0f)
                        continue;

                    // Talka men ta7t w el player nate: 3adet men ta7to
                    if (projectiles[i].height == SHOT_LOW && players[j].jumpTimer > 0.0f)
                        continue;

                    players[j].health -= projectiles[i].damage;
                    projectiles[i].hit = true;
                    break;
                }
            }
        }

        // 5aragat barra el shar3
        if (projectiles[i].x > LANE_RIGHT || projectiles[i].x < LANE_LEFT)
            projectiles[i].hit = true;
    }

    // Nshel el talakat elly 5elset (men el a5er lel awel)
    for (int i = (int)projectiles.size() - 1; i >= 0; i--) {
        if (projectiles[i].hit)
            projectiles.erase(projectiles.begin() + i);
    }
}

// Rasm mo2akat lel talka
void drawProjectile(Projectile s) {
    float y = LANE_Y[s.lane];

    // Ertefa3 el talka = ertefa3 el mawsoura bta3et el sprite elly darabha
    // (el player el ranged, el pilot men fo2, el brute men ta7t)
    int shooter = SPRITE_FREMEN;
    if (s.height == SHOT_HIGH)
        shooter = SPRITE_PILOT;
    else if (s.height == SHOT_LOW)
        shooter = SPRITE_BRUTE;

    if (sprites[shooter].texture != 0)
        y = LANE_Y[s.lane] - FEET_OFFSET +
            sprites[shooter].muzzleDY * SPRITE_SCALE * 2.0f / 600.0f;
    else if (s.height == SHOT_HIGH)
        y += 0.06f;
    else if (s.height == SHOT_LOW)
        y -= 0.06f;

    setOffset(s.x, y);

    if (s.fromPlayer)
        drawRectangle(0.03f, 0.015f, 0.0f, 0.0f, 1.0f, 0.95f, 0.3f);
    else
        drawRectangle(0.03f, 0.015f, 0.0f, 0.0f, 0.8f, 0.2f, 1.0f);

    setOffset(0.0f, 0.0f);
}

// No3 el enemy rakam i fel wave (bnlef 3al anwa3 3ashan yeb2a fe mix akeed)
int waveEnemyType(int wave, int i) {
    if (wave == 0)
        return ENEMY_EASY;            // wave 1: easy bas

    if (wave == 1)
        return i % 2;                 // wave 2: easy, medium, easy, ...

    return i % 3;                     // wave 3: easy, medium, hard, ...
}

// El waves: nenazel el enemies wa7ed wa7ed, w lama el wave te5las
// (kolohom nezlo) nestana le7ad ma n5alas 3alehom, ba3den el wave elly ba3daha
void updateWaves() {
    if (allWavesDone)
        return;

    // Lessa fe enemies mnazalteesh fel wave di
    if (enemiesSpawnedInWave < WAVE_SIZE[currentWave]) {
        waveSpawnTimer += deltaTime;

        if (waveSpawnTimer >= WAVE_SPAWN_TIME[currentWave]) {
            waveSpawnTimer = 0.0f;
            spawnEnemy(waveEnemyType(currentWave, enemiesSpawnedInWave), rand() % 4);
            enemiesSpawnedInWave++;
        }

        return;
    }

    // El wave 5elset (kolohom nezlo) - lazem n5alas 3ala elly fadel 3al shasha
    if (!enemies.empty())
        return;

    // Kanet el wave el a5eera: kesebna
    if (currentWave == 2) {
        allWavesDone = true;
        gameState = STATE_WON;
        std::cout << "Kol el waves 5elset - kesebt!\n";
        return;
    }

    // Ra7a soghayara w ba3den el wave elly ba3daha
    waveBreakTimer += deltaTime;

    if (waveBreakTimer >= WAVE_BREAK_TIME) {
        currentWave++;
        enemiesSpawnedInWave = 0;
        waveBreakTimer = 0.0f;

        // Awel enemy fel wave el gdeeda byenzel 3ala tool
        waveSpawnTimer = WAVE_SPAWN_TIME[currentWave];
    }
}


// El players yelmo el coins elly 3al ard
void updateCoins() {
    for (int i = (int)droppedCoins.size() - 1; i >= 0; i--) {
        for (int j = 0; j < (int)players.size(); j++) {
            if (players[j].lane == droppedCoins[i].lane &&
                isTouching(players[j].x, droppedCoins[i].x)) {
                coins += droppedCoins[i].value;
                droppedCoins.erase(droppedCoins.begin() + i);
                break;
            }
        }
    }
}

// Shakl el coin (dahaby gham2 bara, asfar gowa, no2ta beyda)
void drawCoinIcon(float x, float y, float size) {
    drawRectangle(size, size, x, y, 0.75f, 0.5f, 0.05f);
    drawRectangle(size * 0.7f, size * 0.7f, x, y, 1.0f, 0.85f, 0.2f);
    drawRectangle(size * 0.2f, size * 0.2f, x - size * 0.15f, y + size * 0.15f,
                  1.0f, 1.0f, 0.8f);
}

// Kol segment fel ra2am (a b c d e f g) shaghal wala la2 - zay sa3a digital
int DIGIT_SEGMENTS[10][7] = {
    {1, 1, 1, 1, 1, 1, 0},  // 0
    {0, 1, 1, 0, 0, 0, 0},  // 1
    {1, 1, 0, 1, 1, 0, 1},  // 2
    {1, 1, 1, 1, 0, 0, 1},  // 3
    {0, 1, 1, 0, 0, 1, 1},  // 4
    {1, 0, 1, 1, 0, 1, 1},  // 5
    {1, 0, 1, 1, 1, 1, 1},  // 6
    {1, 1, 1, 0, 0, 0, 0},  // 7
    {1, 1, 1, 1, 1, 1, 1},  // 8
    {1, 1, 1, 1, 0, 1, 1}   // 9
};

// Nersem ra2am wa7ed (0-9) men 7 rectangles
void drawDigit(int d, float x, float y, float size, float r, float g, float b) {
    float w = size * 0.55f;
    float t = size * 0.15f;
    float half = size / 2.0f;

    // a: fo2
    if (DIGIT_SEGMENTS[d][0]) drawRectangle(w, t, x, y + half - t / 2.0f, r, g, b);
    // b: fo2 yemeen
    if (DIGIT_SEGMENTS[d][1]) drawRectangle(t, half, x + w / 2.0f - t / 2.0f, y + size / 4.0f, r, g, b);
    // c: ta7t yemeen
    if (DIGIT_SEGMENTS[d][2]) drawRectangle(t, half, x + w / 2.0f - t / 2.0f, y - size / 4.0f, r, g, b);
    // d: ta7t
    if (DIGIT_SEGMENTS[d][3]) drawRectangle(w, t, x, y - half + t / 2.0f, r, g, b);
    // e: ta7t shmal
    if (DIGIT_SEGMENTS[d][4]) drawRectangle(t, half, x - w / 2.0f + t / 2.0f, y - size / 4.0f, r, g, b);
    // f: fo2 shmal
    if (DIGIT_SEGMENTS[d][5]) drawRectangle(t, half, x - w / 2.0f + t / 2.0f, y + size / 4.0f, r, g, b);
    // g: el nos
    if (DIGIT_SEGMENTS[d][6]) drawRectangle(w, t, x, y, r, g, b);
}

// Nersem rakam kamel (masalan 120) - x = awel ra2am men el shmal
void drawNumber(int value, float x, float y, float size, float r, float g, float b) {
    std::string text = std::to_string(value);

    for (int i = 0; i < (int)text.size(); i++)
        drawDigit(text[i] - '0', x + i * size * 0.75f, y, size, r, g, b);
}

// 3add el coins fo2 3al yemeen
void drawCoinCounter() {
    // Panel gham2 wara
    drawRectangle(0.36f, 0.14f, 0.74f, CARD_Y, 0.12f, 0.12f, 0.15f);

    drawCoinIcon(0.62f, CARD_Y, 0.07f);
    drawNumber(coins, 0.7f, CARD_Y, 0.07f, 1.0f, 0.85f, 0.2f);
}

// Rasm 7etta men el card (law locked byeb2a ramady)
void drawCardPart(float w, float h, float x, float y,
                  float r, float g, float b, bool locked) {
    if (locked) {
        float gray = (0.3f * r + 0.59f * g + 0.11f * b) * 0.7f;
        r = gray; g = gray; b = gray;
    }

    drawRectangle(w, h, x, y, r, g, b);
}

// Law mestaneyeen makan el defense: nwareeh hayeb2a fein ta7t el mouse
void drawDefensePreview() {
    if (!placingDefense)
        return;

    float mx, my;
    getMousePosition(&mx, &my);

    int lane = getMouseLane();
    if (lane == -1)
        return;

    float w, h, r, g, b;
    getPlayerLook(PLAYER_DEFENSE, &w, &h, &r, &g, &b);

    // Bervaz asfar w el defense fate7 shwaya (lessa mat7atsh)
    drawRectangle(w + 0.015f, h + 0.015f, mx, LANE_Y[lane], 1.0f, 0.85f, 0.1f);
    drawRectangle(w, h, mx, LANE_Y[lane], 0.8f, 0.8f, 0.8f);
}

// ===== El font: kol 7arf 5 x 7 morab3at ('#' = malyan) =====
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

const char* FONT_EXCLAIM[7] = {"..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.."};

// Nersem kelma fel nos 3and centerX (pixel = 7agm el morab3 el soghayar)
void drawText(std::string text, float centerX, float y, float pixel,
                float r, float g, float b) {
    // El morab3 yeb2a morab3 bel zabt 7atta law el shasha 3areeda
    float pixelX = pixel;
    float pixelY = pixel * (float)width / (float)height;

    // Kol 7arf 5 morab3at + 1 fady ben el 7oroof
    float totalWidth = text.size() * 6.0f * pixelX - pixelX;
    float startX = centerX - totalWidth / 2.0f;

    for (int i = 0; i < (int)text.size(); i++) {
        char c = text[i];
        const char** rows = nullptr;

        if (c >= 'A' && c <= 'Z')
            rows = FONT_LETTERS[c - 'A'];
        else if (c >= '0' && c <= '9')
            rows = FONT_DIGITS[c - '0'];
        else if (c == '!')
            rows = FONT_EXCLAIM;

        // Masafa (aw 7arf mesh 3andena): nseebo fady
        if (rows == nullptr)
            continue;

        float letterX = startX + i * 6.0f * pixelX;

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (rows[row][col] != '#')
                    continue;

                // Row 0 fo2 - row 6 ta7t (el nos = row 3)
                drawRectangle(pixelX, pixelY,
                              letterX + col * pixelX + pixelX / 2.0f,
                              y + (3 - row) * pixelY,
                              r, g, b);
            }
        }
    }
}

// El mouse 3ala anhy zorar (-1 law mafeesh) - lay menu
// (el zarayer kolohom fel nos, w buttonY feeh el y bta3 kol wa7ed)
int buttonAt(float x, float y, float* buttonY, int count) {
    for (int i = 0; i < count; i++) {
        if (x >= -END_BUTTON_W / 2.0f && x <= END_BUTTON_W / 2.0f &&
            y >= buttonY[i] - END_BUTTON_H / 2.0f &&
            y <= buttonY[i] + END_BUTTON_H / 2.0f)
            return i;
    }

    return -1;
}

// El mouse 3ala anhy zorar fel win/lose screen (-1 law mafeesh)
int endButtonAt(float x, float y) {
    return buttonAt(x, y, END_BUTTON_Y, 2);
}

// Nersem zarayer menu (bytnawaro asfar lama el mouse 3alehom)
void drawButtons(std::string* labels, float* buttonY, int count) {
    float mx, my;
    getMousePosition(&mx, &my);
    int hovered = buttonAt(mx, my, buttonY, count);

    for (int i = 0; i < count; i++) {
        float r = 0.25f, g = 0.25f, b = 0.3f;
        if (hovered == i) {
            r = 0.95f; g = 0.8f; b = 0.2f;
        }

        drawRectangle(END_BUTTON_W, END_BUTTON_H, 0.0f, buttonY[i], r, g, b);
        drawText(labels[i], 0.0f, buttonY[i], 0.009f, 1.0f, 1.0f, 1.0f);
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
    allWavesDone = false;

    gameLost = false;
    gameState = STATE_PLAYING;
}

// Start men el main menu (step 17: el cutscene hayeb2a hena 2abl el game)
void startGame() {
    restartGame();
}

// Nrga3 lel main menu (w el game yebda2 men el awel lama ndoos Start)
void goToMainMenu() {
    restartGame();
    gameState = STATE_MENU;
}

// Shashet el win aw el lose (fo2 el game el wa2ef)
void drawEndScreen() {
    bool won = (gameState == STATE_WON);

    // Panel fel nos b bervaz (a5dar law kesebt, a7mar law 5esert)
    float br = 0.8f, bg = 0.15f, bb = 0.15f;
    if (won) {
        br = 0.2f; bg = 0.75f; bb = 0.3f;
    }

    drawRectangle(1.16f, 0.86f, 0.0f, 0.0f, br, bg, bb);
    drawRectangle(1.1f, 0.8f, 0.0f, 0.0f, 0.1f, 0.1f, 0.13f);

    // El 3enwan w el sabab
    if (won) {
        drawText("YOU WIN!", 0.0f, 0.23f, 0.019f, 0.3f, 0.9f, 0.4f);
        drawText("ALL WAVES CLEARED", 0.0f, 0.08f, 0.009f, 0.9f, 0.9f, 0.9f);
    }
    else {
        drawText("GAME OVER", 0.0f, 0.23f, 0.019f, 0.95f, 0.25f, 0.25f);
        drawText("AN ENEMY REACHED THE BASE", 0.0f, 0.08f, 0.0065f, 0.9f, 0.9f, 0.9f);
    }

    // El zarayer
    std::string labels[2] = {"RESTART", "MAIN MENU"};
    drawButtons(labels, END_BUTTON_Y, 2);
}

// Shashet el pause (Esc)
void drawPauseMenu() {
    // Panel fel nos b bervaz asfar
    drawRectangle(0.86f, 0.98f, 0.0f, 0.0f, 0.95f, 0.8f, 0.2f);
    drawRectangle(0.8f, 0.92f, 0.0f, 0.0f, 0.1f, 0.1f, 0.13f);

    drawText("PAUSED", 0.0f, 0.27f, 0.019f, 0.95f, 0.8f, 0.2f);

    std::string labels[3] = {"CONTINUE", "RESTART", "MAIN MENU"};
    drawButtons(labels, PAUSE_BUTTON_Y, 3);
}

// El main menu (awel ma el game yeftah) - bey3'atty el shasha kolaha
void drawMainMenu() {
    // 5alfeya: sama bel leil
    drawRectangle(2.0f, 2.0f, 0.0f, 0.0f, 0.06f, 0.08f, 0.16f);

    // Mabany sooda ta7t (zay el madeena men ba3eed)
    float buildingX[7] = {-0.85f, -0.6f, -0.35f, -0.1f, 0.15f, 0.4f, 0.7f};
    float buildingH[7] = {0.45f, 0.65f, 0.35f, 0.55f, 0.4f, 0.6f, 0.5f};

    for (int i = 0; i < 7; i++) {
        drawRectangle(0.22f, buildingH[i], buildingX[i], -1.0f + buildingH[i] / 2.0f,
                      0.12f, 0.14f, 0.22f);

        // Shebbak menawar fe kol mabna
        drawRectangle(0.035f, 0.05f, buildingX[i], -1.0f + buildingH[i] - 0.1f,
                      1.0f, 0.85f, 0.35f);
    }

    // Esm el game (8ayaro 3ala 7asab el theme bta3ko)
    drawText("CITY DEFENSE", 0.0f, 0.45f, 0.022f, 0.95f, 0.8f, 0.2f);
    drawText("PROTECT THE CITY", 0.0f, 0.26f, 0.009f, 0.85f, 0.85f, 0.9f);

    std::string labels[2] = {"START", "EXIT"};
    drawButtons(labels, MAIN_MENU_BUTTON_Y, 2);

    drawText("ESC TO PAUSE", 0.0f, -0.4f, 0.006f, 0.6f, 0.6f, 0.7f);
}

// 3 morab3at fel nos fo2: kol wa7ed wave (a5dar = 5elset, asfar = delwa2ty, ramady = lessa)
void drawWaveIndicator() {
    // Panel gham2 wara
    drawRectangle(0.42f, 0.14f, 0.15f, CARD_Y, 0.12f, 0.12f, 0.15f);

    for (int i = 0; i < 3; i++) {
        float x = 0.02f + i * 0.13f;

        float r = 0.4f, g = 0.4f, b = 0.4f;      // lessa
        if (i < currentWave || allWavesDone) {
            r = 0.2f; g = 0.7f; b = 0.3f;         // 5elset
        }
        else if (i == currentWave) {
            r = 1.0f; g = 0.85f; b = 0.1f;        // delwa2ty

            // Bervaz abyad 7awalen el wave el delwa2ty
            drawRectangle(0.115f, 0.115f, x, CARD_Y, 1.0f, 1.0f, 1.0f);
        }

        drawRectangle(0.1f, 0.1f, x, CARD_Y, r, g, b);
        drawNumber(i + 1, x, CARD_Y, 0.06f, 0.1f, 0.1f, 0.1f);
    }
}

// El 3 cards fo2 3al shmal
void drawCards() {
    float mx, my;
    getMousePosition(&mx, &my);
    int hoveredCard = cardAt(mx, my);

    // Panel gham2 wara el cards
    drawRectangle(0.64f, 0.27f, -0.62f, CARD_Y, 0.12f, 0.12f, 0.15f);

    for (int i = 0; i < 3; i++) {
        // Ramady law lessa locked aw law el coins mesh kfaya lel se3r
        bool locked = !isUnlocked(i) || coins < playerCost(i);
        float x = CARD_X[i];

        // Card el defense mestany makan: bervaz asfar
        if (i == PLAYER_DEFENSE && placingDefense)
            drawRectangle(CARD_W + 0.03f, CARD_H + 0.03f, x, CARD_Y, 1.0f, 0.85f, 0.1f);

        // Hover: bervaz abyad (bas law unlocked - el locked mabyrodsh)
        if (!locked && hoveredCard == i)
            drawRectangle(CARD_W + 0.02f, CARD_H + 0.02f, x, CARD_Y, 1.0f, 1.0f, 1.0f);

        // Wesh el card
        drawCardPart(CARD_W, CARD_H, x, CARD_Y, 0.85f, 0.8f, 0.6f, locked);

        // Soret el player (el sprite law mawgood, law la2 el rectangle)
        float w, h, r, g, b;
        getPlayerLook(i, &w, &h, &r, &g, &b);

        int cardSprite = playerSprite(i);
        if (cardSprite != -1 && sprites[cardSprite].texture != 0)
            drawSprite(cardSprite, ANIM_IDLE, 0, x, CARD_Y - 0.045f,
                       true, 0.0f, locked ? 1.0f : 0.0f, CARD_SPRITE_SCALE);
        else
            drawCardPart(w * 0.8f, h * 0.8f, x, CARD_Y + 0.03f, r, g, b, locked);

        // El se3r ta7t
        float nr = 0.2f, ng = 0.15f, nb = 0.05f;
        if (locked) {
            nr = 0.35f; ng = 0.35f; nb = 0.35f;
        }

        drawNumber(playerCost(i), x - 0.025f, CARD_Y - 0.075f, 0.04f, nr, ng, nb);
    }
}


// ==========================================
// El arena (Arrakis) - kolaha rectangles
// ==========================================

// Zay drawRectangle bas el lon byetghayar men fo2 le ta7t (gradient na3em)
void drawGradientRectangle(float rectWidth, float rectHeight, float posX, float posY,
                           float rTop, float gTop, float bTop,
                           float rBottom, float gBottom, float bBottom) {
    float left   = posX - rectWidth / 2.0f;
    float right  = posX + rectWidth / 2.0f;
    float top    = posY + rectHeight / 2.0f;
    float bottom = posY - rectHeight / 2.0f;

    // El vertices elly fo2 leha lon, w elly ta7t leha lon tany (el GPU byemla elly benhom)
    float vertices[] = {
        left,  top,    0.0f, rTop,    gTop,    bTop,
        left,  bottom, 0.0f, rBottom, gBottom, bBottom,
        right, top,    0.0f, rTop,    gTop,    bTop,
        right, bottom, 0.0f, rBottom, gBottom, bBottom
    };

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// Dayra men rectangles ofo2eya fo2 ba3d
// (el 3ard byetsala7 bel aspect 3ashan tefdal dayra mesh beyda)
void drawCircle(float x, float y, float radius, float r, float g, float b) {
    int strips = 24;
    float stripH = 2.0f * radius / strips;
    float aspect = (float)height / width;

    for (int i = 0; i < strips; i++) {
        float stripY = y - radius + stripH * (i + 0.5f);
        float dy = stripY - y;
        float halfW = sqrt(radius * radius - dy * dy);

        drawRectangle(2.0f * halfW * aspect, stripH + 0.002f, x, stripY, r, g, b);
    }
}

// Sama Arrakis: gradient men bonny/a7mar fo2 le bortoo2any le asfar fate7 3and el ofo2
void drawSky() {
    drawGradientRectangle(2.0f, 0.20f, 0.0f, 0.90f,
                          0.42f, 0.17f, 0.10f,
                          0.80f, 0.40f, 0.18f);

    drawGradientRectangle(2.0f, 0.18f, 0.0f, 0.71f,
                          0.80f, 0.40f, 0.18f,
                          0.99f, 0.80f, 0.52f);
}

// El 2amarein bto3 Arrakis, kol wa7ed 7awaleeh nour (dawayer asghar fo2 ba3d)
void drawMoons() {
    // El kebeer
    drawCircle(0.46f, 0.79f, 0.115f, 0.86f, 0.51f, 0.27f);
    drawCircle(0.46f, 0.79f, 0.092f, 0.93f, 0.66f, 0.42f);
    drawCircle(0.46f, 0.79f, 0.075f, 0.98f, 0.94f, 0.84f);
    drawCircle(0.44f, 0.81f, 0.018f, 0.91f, 0.86f, 0.75f);
    drawCircle(0.49f, 0.77f, 0.012f, 0.91f, 0.86f, 0.75f);

    // El soghayar
    drawCircle(-0.17f, 0.88f, 0.050f, 0.72f, 0.40f, 0.22f);
    drawCircle(-0.17f, 0.88f, 0.035f, 0.93f, 0.87f, 0.77f);
}

// Ertefa3 el kseeban el be3eeda 3and x (sin fo2 sin 3ashan el shakl yeb2a tabee3y)
float farDuneTop(float x) {
    return 0.71f + 0.045f * sin(2.7f * x + 0.6f) + 0.02f * sin(6.9f * x + 2.1f);
}

// Ertefa3 el kseeban el 2orayyeba
float nearDuneTop(float x) {
    return 0.655f + 0.025f * sin(3.9f * x + 2.4f) + 0.012f * sin(9.7f * x + 0.3f);
}

// Saff kseeban: 3awameed rofa3 gamb ba3d, kol 3amood tooloh 7asab el sin
// (w 5att nour rofayya3 3ala el 2emma)
void drawDuneLayer(bool far,
                   float rTop, float gTop, float bTop,
                   float rBottom, float gBottom, float bBottom,
                   float rLight, float gLight, float bLight) {
    int columns = 160;
    float colW = 2.0f / columns;
    float bottom = 0.60f;

    for (int i = 0; i < columns; i++) {
        float x = -1.0f + colW * (i + 0.5f);
        float top;

        if (far)
            top = farDuneTop(x);
        else
            top = nearDuneTop(x);

        drawGradientRectangle(colW + 0.002f, top - bottom, x, (top + bottom) / 2.0f,
                              rTop, gTop, bTop, rBottom, gBottom, bBottom);

        drawRectangle(colW + 0.002f, 0.006f, x, top - 0.003f, rLight, gLight, bLight);
    }
}

// Sa5ra 3aleya (mesa): talat dorr (wase3 ta7t w ad2a2 fo2), gamb fel dell w gamb fel nour
void drawMesa(float x, float baseY, float mesaWidth, float mesaHeight) {
    // El dor el wase3 ta7t
    drawGradientRectangle(mesaWidth * 1.35f, mesaHeight * 0.3f, x, baseY + mesaHeight * 0.15f,
                          0.50f, 0.30f, 0.19f,
                          0.40f, 0.24f, 0.15f);

    // El gesm
    drawGradientRectangle(mesaWidth, mesaHeight, x, baseY + mesaHeight / 2.0f,
                          0.52f, 0.31f, 0.19f,
                          0.38f, 0.22f, 0.14f);

    // El ras el ad2a2
    drawRectangle(mesaWidth * 0.75f, mesaHeight * 0.25f, x,
                  baseY + mesaHeight + mesaHeight * 0.125f,
                  0.50f, 0.30f, 0.18f);

    // Nour el shams 3al gamb el shmal
    drawGradientRectangle(mesaWidth * 0.22f, mesaHeight, x - mesaWidth * 0.39f,
                          baseY + mesaHeight / 2.0f,
                          0.72f, 0.47f, 0.29f,
                          0.55f, 0.34f, 0.21f);
}

// El sandworm: dahro tale3 men el raml (2oba malyana) w beyemshy bel ra7a 3ala el kseeban
void drawSandworm(float t) {
    // Beyemshy men el yemeen lel shmal w yerga3 tany
    float x = 1.1f - fmod(t * 0.025f, 2.2f);
    float baseY = farDuneTop(x) - 0.012f;

    int columns = 14;
    float colW = 0.008f;

    for (int i = -columns; i <= columns; i++) {
        float k = (float)i / columns;
        float top = baseY + 0.045f * sqrt(1.0f - k * k);

        // Kol talat 3awameed 7alaqa agham2 (zay el 7ala2at fe gesm el worm)
        float shade = (i % 3 == 0) ? 0.06f : 0.0f;

        drawGradientRectangle(colW + 0.001f, top - baseY + 0.01f, x + i * colW,
                              (top + baseY - 0.01f) / 2.0f,
                              0.58f - shade, 0.40f - shade, 0.27f - shade,
                              0.40f - shade, 0.27f - shade, 0.18f - shade);
    }

    // Raml beyetrash men 3and el atraf
    drawRectangle(0.014f, 0.008f, x - 0.125f, baseY + 0.010f, 0.97f, 0.80f, 0.55f);
    drawRectangle(0.010f, 0.008f, x - 0.140f, baseY + 0.022f, 0.97f, 0.80f, 0.55f);
    drawRectangle(0.012f, 0.008f, x + 0.125f, baseY + 0.014f, 0.97f, 0.80f, 0.55f);
}

// 7abbet raml tayra fel hawa (el reya7 bet7arakha le el shmal)
void drawDust(float t) {
    float aspect = (float)height / width;

    for (int i = 0; i < 36; i++) {
        float speed = 0.04f + 0.03f * (i % 5) / 4.0f;
        float startX = fmod(i * 0.618f, 1.0f) * 2.1f;

        float x = 1.05f - fmod(startX + t * speed, 2.1f);
        float y = 0.64f + fmod(i * 0.137f, 0.34f) + 0.01f * sin(t * 1.3f + i);

        float size = 0.004f + 0.002f * (i % 3);
        drawRectangle(size * aspect, size, x, y, 1.0f, 0.90f, 0.72f);
    }
}

// El fasel el sa5ry ben el lanes
void drawRockRidge(float y, float ridgeHeight, float t) {
    // El sa5r: fate7 fo2 (el shams) w gham2 ta7t
    drawGradientRectangle(2.0f, ridgeHeight, 0.0f, y,
                          0.64f, 0.45f, 0.30f,
                          0.33f, 0.21f, 0.14f);

    // 5att nour fo2 w dell ta7t
    drawRectangle(2.0f, 0.008f, 0.0f, y + ridgeHeight / 2.0f - 0.004f, 0.78f, 0.60f, 0.42f);
    drawRectangle(2.0f, 0.008f, 0.0f, y - ridgeHeight / 2.0f + 0.004f, 0.24f, 0.15f, 0.10f);

    // 7agar soghayar (gamb fel dell w gamb fel nour) w spice benawwar
    for (int i = 0; i < 9; i++) {
        float x = -0.88f + i * 0.22f + 0.03f * sin(i * 2.3f);

        drawRectangle(0.045f, 0.018f, x, y - ridgeHeight * 0.05f, 0.40f, 0.26f, 0.17f);
        drawRectangle(0.028f, 0.010f, x - 0.004f, y + ridgeHeight * 0.05f, 0.58f, 0.41f, 0.27f);

        if (i % 3 == 1) {
            float glow = 0.5f + 0.5f * sin(t * 2.0f + i);
            drawRectangle(0.012f, 0.012f, x + 0.08f, y + ridgeHeight * 0.1f,
                          0.90f + 0.10f * glow, 0.45f + 0.15f * glow, 0.10f + 0.10f * glow);
        }
    }
}

// Lane wa7ed: raml metgamma3 (gradient) w 3aleh mawgat raml na3ma
void drawSandLane(float laneY) {
    // Dell ta7t el lane
    drawRectangle(1.89f, 0.30f, 0.0f, laneY - 0.012f, 0.33f, 0.22f, 0.15f);

    // El raml nafso: fate7 fo2 w gham2 ta7t
    drawGradientRectangle(1.85f, 0.28f, 0.0f, laneY,
                          0.62f, 0.47f, 0.33f,
                          0.50f, 0.37f, 0.26f);

    // Dell rofayya3 ta7t el sa5r elly fo2 el lane
    drawRectangle(1.85f, 0.010f, 0.0f, laneY + 0.135f, 0.42f, 0.30f, 0.21f);

    // Mawgat el raml: 5att mawgy gham2 w foo2o 5att nour (kol wa7ed men 7etat soghayara)
    int pieces = 48;
    float pieceW = 1.80f / pieces;

    for (int row = 0; row < 3; row++) {
        float rowY = laneY + 0.07f - row * 0.07f;

        for (int i = 0; i < pieces; i++) {
            float x = -0.90f + pieceW * (i + 0.5f);

            // Fawasel ben el mawgat 3ashan mayeb2ash 5att wa7ed taweel
            if (sin(x * 3.3f + row * 1.9f + laneY * 4.0f) < -0.35f)
                continue;

            float y = rowY + 0.008f * sin(x * 9.0f + row * 1.7f + laneY * 5.0f);

            drawRectangle(pieceW + 0.002f, 0.005f, x, y, 0.52f, 0.38f, 0.27f);
            drawRectangle(pieceW + 0.002f, 0.004f, x, y + 0.006f, 0.65f, 0.50f, 0.35f);
        }
    }
}

// El arena kolaha (bnersemha awel 7aga fe kol frame)
void drawArena() {
    // El wa2t 3ashan el 7agat elly bet7arak (el worm, el raml, el spice)
    float t = (float)glfwGetTime();

    drawSky();
    drawMoons();
    drawDust(t);

    // El ard: raml men el ofo2 le ta7t
    drawRectangle(2.0f, 1.62f, 0.0f, -0.19f, 0.80f, 0.60f, 0.38f);

    // El kseeban el be3eeda, el worm, el sa5r, ba3den el 2orayyeba
    drawDuneLayer(true,
                  0.93f, 0.70f, 0.45f,
                  0.82f, 0.56f, 0.34f,
                  0.99f, 0.84f, 0.60f);
    drawSandworm(t);
    drawMesa(-0.18f, 0.62f, 0.12f, 0.12f);
    drawMesa( 0.97f, 0.62f, 0.10f, 0.08f);
    drawDuneLayer(false,
                  0.86f, 0.60f, 0.36f,
                  0.74f, 0.50f, 0.30f,
                  0.95f, 0.74f, 0.50f);

    // El fasel elly fo2 el lanes, w el fawasel elly benhom, w elly ta7t
    drawRockRidge(0.585f, 0.07f, t);
    drawRockRidge(0.22f, 0.085f, t);
    drawRockRidge(-0.16f, 0.085f, t);
    drawRockRidge(-0.54f, 0.085f, t);
    drawRockRidge(-0.92f, 0.085f, t);

    for (int i = 0; i < 4; i++)
        drawSandLane(LANE_Y[i]);
}


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
    if (!initialize()) {
        return -1;
    }

    if (!pipeline()) {
        cleanup();
        return -1;
    }

    // Nload el atlases bta3et el characters
    loadSprites();

    // Nebda2 el clock 2abl el loop
    lastFrameTime = (float)glfwGetTime();

    // Render loop
    while (!glfwWindowShouldClose(Window)) {
        updateDeltaTime();

        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(ShaderProgram);

        // El background yetresem fe makano el 3ady (mesh a7mar)
        setOffset(0.0f, 0.0f);
        setRed(0.0f);

        
        
        
        // ==========================================
        // 1. EL ARENA (sama, kseeban, sa5r, lanes)
        // ==========================================
        drawArena();


        // ==========================================
        // 11. LANE HIGHLIGHT (hover + selected)
        // ==========================================
        hoveredLane = getMouseLane();

        for (int i = 0; i < 4; i++) {
            // Selected: asfar w te5een
            if (selectedLanes[i])
                drawLaneHighlight(i, 0.02f, 1.0f, 0.85f, 0.1f);
        }

        // Hover: abyad w rofayya3 (law mesh selected aslan)
        if (hoveredLane != -1 && !selectedLanes[hoveredLane])
            drawLaneHighlight(hoveredLane, 0.01f, 1.0f, 1.0f, 1.0f);


        // ==========================================
        // 12. PLAYERS W ENEMIES
        // ==========================================
        // El game wa2ef law kesebna aw 5esrna (bnersem bas mn gher update)
        if (gameState == STATE_PLAYING) {
            updateWaves();
            updatePlayers();
            updateEnemies();
            updatePlayerAttacks();
            updateEnemyAttacks();
            updateProjectiles();
            removeDeadEntities();
            updateAnimations();
            updateCoins();
        }

        for (int i = 0; i < (int)droppedCoins.size(); i++)
            drawCoinIcon(droppedCoins[i].x, LANE_Y[droppedCoins[i].lane] - 0.07f, 0.04f);

        for (int i = 0; i < (int)players.size(); i++)
            drawPlayer(players[i]);

        for (int i = 0; i < (int)enemies.size(); i++)
            drawEnemy(enemies[i]);

        for (int i = 0; i < (int)projectiles.size(); i++)
            drawProjectile(projectiles[i]);


        // ==========================================
        // 13. UI: el cards w el coins
        // ==========================================
        drawDefensePreview();
        drawCards();
        drawCoinCounter();
        drawWaveIndicator();


        // ==========================================
        // 14. MENUS (win/lose, pause, main menu)
        // ==========================================
        if (gameState == STATE_WON || gameState == STATE_LOST)
            drawEndScreen();

        if (gameState == STATE_PAUSED)
            drawPauseMenu();

        if (gameState == STATE_MENU)
            drawMainMenu();



        
       


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    cleanup();

    return 0;
}
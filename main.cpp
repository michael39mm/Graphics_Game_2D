#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include <string>
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
};

// Kart el enemy
struct Enemy {
    int type;
    int lane;
    float x;
    int health;
    int maxHealth;
    float attackTimer;
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

// Rasm mo2akat lel player (el shakl el 7a2e2y ba3den)
void drawPlayer(Player p) {
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

// Rasm mo2akat lel enemy
void drawEnemy(Enemy e) {
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
            }
        }
        else if (players[i].type == PLAYER_RANGED) {
            // Byedrab 3ala tool 7atta law mafeesh enemies
            if (players[i].attackTimer >= RANGED_ATTACK_TIME) {
                players[i].attackTimer -= RANGED_ATTACK_TIME;
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
            }
        }
        else if (enemies[i].attackTimer >= ENEMY_SHOOT_TIME) {
            enemies[i].attackTimer -= ENEMY_SHOOT_TIME;

            int height = SHOT_LOW;
            if (enemies[i].type == ENEMY_EASY)
                height = SHOT_HIGH;

            fireProjectile(enemies[i].lane, enemies[i].x,
                           -PROJECTILE_SPEED, 1, false, height);
        }
    }
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

    // Talka men fo2 aw men ta7t
    if (s.height == SHOT_HIGH)
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

        // Soret el player
        float w, h, r, g, b;
        getPlayerLook(i, &w, &h, &r, &g, &b);
        drawCardPart(w * 0.8f, h * 0.8f, x, CARD_Y + 0.03f, r, g, b, locked);

        // El se3r ta7t
        float nr = 0.2f, ng = 0.15f, nb = 0.05f;
        if (locked) {
            nr = 0.35f; ng = 0.35f; nb = 0.35f;
        }

        drawNumber(playerCost(i), x - 0.025f, CARD_Y - 0.075f, 0.04f, nr, ng, nb);
    }
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
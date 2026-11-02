#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
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

// TEST: kol kam sanya yenzel enemy (el waves ba3den hatsheel da)
float enemySpawnTimer = 0.0f;
float ENEMY_SPAWN_TIME = 4.0f;

// Law enemy wesel lel home base
bool gameLost = false;

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

// Lama nedoos click
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        int lane = getMouseLane();

        // Click 3ala lane: law selected teb2a la2, law la2 teb2a selected
        if (lane != -1)
            selectedLanes[lane] = !selectedLanes[lane];
    }
}

void spawnPlayer(int type, int lane);
void spawnEnemy(int type, int lane);

// TEST: 1/2/3 = player fe kol el lanes el selected, E = enemy random
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS)
        return;

    for (int i = 0; i < 4; i++) {
        if (!selectedLanes[i])
            continue;

        if (key == GLFW_KEY_1) spawnPlayer(PLAYER_MELEE, i);
        if (key == GLFW_KEY_2) spawnPlayer(PLAYER_RANGED, i);
        if (key == GLFW_KEY_3) spawnPlayer(PLAYER_DEFENSE, i);
    }

    if (key == GLFW_KEY_E)
        spawnEnemy(rand() % 3, rand() % 4);

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

// Nshel ay 7ad health bta3o 5eles
// (bnemshy men el a5er lel awel 3ashan el erase maybawazsh el 3ad)
void removeDeadEntities() {
    for (int i = (int)players.size() - 1; i >= 0; i--) {
        if (players[i].health <= 0)
            players.erase(players.begin() + i);
    }

    for (int i = (int)enemies.size() - 1; i >= 0; i--) {
        if (enemies[i].health <= 0)
            enemies.erase(enemies.begin() + i);
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
    float w = 0.06f, h = 0.16f;
    float r = 0.2f, g = 0.8f, b = 0.3f;

    if (p.type == PLAYER_RANGED) {
        w = 0.05f; h = 0.18f;
        r = 0.2f; g = 0.6f; b = 1.0f;
    }
    else if (p.type == PLAYER_DEFENSE) {
        w = 0.09f; h = 0.22f;
        r = 0.6f; g = 0.6f; b = 0.6f;
    }

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

        // Wesel lel home base: 5esrna (step 13 hay3mel el ba2y)
        if (enemies[i].x < LANE_LEFT) {
            enemies[i].x = LANE_LEFT;

            if (!gameLost) {
                gameLost = true;
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

// TEST: kol ENEMY_SPAWN_TIME sanya enemy random fe lane random

void updateEnemySpawning() {
    enemySpawnTimer += deltaTime;

    if (enemySpawnTimer >= ENEMY_SPAWN_TIME) {
        enemySpawnTimer -= ENEMY_SPAWN_TIME;
        spawnEnemy(rand() % 3, rand() % 4);
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
        updateEnemySpawning();
        updatePlayers();
        updateEnemies();
        updatePlayerAttacks();
        updateEnemyAttacks();
        updateProjectiles();
        removeDeadEntities();

        for (int i = 0; i < (int)players.size(); i++)
            drawPlayer(players[i]);

        for (int i = 0; i < (int)enemies.size(); i++)
            drawEnemy(enemies[i]);

        for (int i = 0; i < (int)projectiles.size(); i++)
            drawProjectile(projectiles[i]);



        
       


        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    cleanup();

    return 0;
}
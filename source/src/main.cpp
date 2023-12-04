#include <cmath>
#include <string>
#include <random>
#include <iostream>
#include <math.h>
#include <cstdlib>

#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.hpp"
#include "Camera.hpp"
#include "PerlinNoise.hpp"

int gScreenWidth = 1920;
int gScreenHeight = 1080;
SDL_Window* window 	= nullptr;
SDL_GLContext glContext;
bool quit = false;

// Map params
float WATER_HEIGHT = 0.1;
int chunk_render_distance = 3;
int xMapChunks = 10;
int yMapChunks = 10;
int chunkWidth = 127;
int chunkHeight = 127;
int gridPosX = 0;
int gridPosY = 0;
float originX = (chunkWidth  * xMapChunks) / 2 - chunkWidth / 2;
float originY = (chunkHeight * yMapChunks) / 2 - chunkHeight / 2;

// Noise params
int octaves = 6;
float meshHeight = 32;
float noiseScale = 64;
float persistence = 0.5;
float lacunarity = 2;

// Camera
Camera camera(glm::vec3(originX, 20.0f, originY));

// Mouse
int mouseX = gScreenWidth / 2;
int mouseY = gScreenHeight / 2;

// Terrain Colors
glm::vec3 deepWater = glm::vec3(0.24, 0.37, 0.75);
glm::vec3 shallowWater = glm::vec3(0.24, 0.4, 0.75);
glm::vec3 sand = glm::vec3(0.82, 0.84, 0.5);
glm::vec3 grass1 = glm::vec3(0.37, 0.65, 0.12);
glm::vec3 grass2 = glm::vec3(0.25, 0.45, 0.08);
glm::vec3 mountain1 = glm::vec3(0.35, 0.25, 0.25);
glm::vec3 mountain2 = glm::vec3(0.3, 0.25, 0.2);
glm::vec3 snow = glm::vec3(1, 1, 1);

// Generate random permutation vector
std::vector<int> p = getPermutationVector();

struct terrainColor {
    terrainColor(float _height, glm::vec3 _color) {
        height = _height;
        color = _color;
    };
    float height;
    glm::vec3 color;
};

void render(std::vector<GLuint> &map_chunks, Shader &shader, glm::mat4 &view, glm::mat4 &model, glm::mat4 &projection, int &nIndices) {

    glClearColor(0.53, 0.8, 0.92, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Compute chunk distance from origin
    gridPosX = (int)(camera.GetEyeXPosition() - originX) / chunkWidth + xMapChunks / 2;
    gridPosY = (int)(camera.GetEyeYPosition() - originY) / chunkHeight + yMapChunks / 2;
    
    // Render map chunks that are within render distance
    for (int y = 0; y < yMapChunks; y++) {
        for (int x = 0; x < xMapChunks; x++) {
            if (std::abs(gridPosX - x) <= chunk_render_distance && (y - gridPosY) <= chunk_render_distance) {
                model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(-chunkWidth / 2.0 + (chunkWidth - 1) * x, 0.0, -chunkHeight / 2.0 + (chunkHeight - 1) * y));
                shader.SetUniformMatrix4fv("u_ModelMatrix", &model[0][0]);
                
                glBindVertexArray(map_chunks[x + y*xMapChunks]);
                glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, 0);
            }
        }
    }
}

std::vector<int> generateIndices() {
    std::vector<int> indices;
    
    for (int y = 0; y < chunkHeight; y++)
        for (int x = 0; x < chunkWidth; x++) {
            int pos = x + y*chunkWidth;
            
            if (x == chunkWidth - 1 || y == chunkHeight - 1) {
                continue;
            } else {
                indices.push_back(pos + chunkWidth);
                indices.push_back(pos);
                indices.push_back(pos + chunkWidth + 1);
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + chunkWidth);
                indices.push_back(pos);
            }
        }

    return indices;
}

std::vector<float> generateNoiseMap(int offsetX, int offsetY) {
    std::vector<float> noiseValues;
    std::vector<float> normalizedNoiseValues;
    
    float amp  = 1;
    float freq = 1;
    float maxPossibleHeight = 0;
    
    for (int i = 0; i < octaves; i++) {
        maxPossibleHeight += amp;
        amp *= persistence;
    }
    
    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            amp  = 1;
            freq = 1;
            float noiseHeight = 0;
            for (int i = 0; i < octaves; i++) {
                float xSample = (x + offsetX * (chunkWidth-1))  / noiseScale * freq;
                float ySample = (y + offsetY * (chunkHeight-1)) / noiseScale * freq;
                
                float perlinValue = perlinNoise(xSample, ySample, p);
                noiseHeight += perlinValue * amp;
                
                amp  *= persistence;
                freq *= lacunarity;
            }
            
            noiseValues.push_back(noiseHeight);
        }
    }
    
    for (int y = 0; y < chunkHeight; y++) {
        for (int x = 0; x < chunkWidth; x++) {
            normalizedNoiseValues.push_back((noiseValues[x + y*chunkWidth] + 1) / maxPossibleHeight);
        }
    }

    return normalizedNoiseValues;
}

std::vector<float> generateVertices(const std::vector<float> &noise_map) {
    std::vector<float> v;
    
    for (int y = 0; y < chunkHeight + 1; y++)
        for (int x = 0; x < chunkWidth; x++) {
            v.push_back(x);
            float easedNoise = std::pow(noise_map[x + y*chunkWidth] * 1.1, 3);
            v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
            v.push_back(y);
        }
    
    return v;
}

std::vector<float> generateNormals(const std::vector<int> &indices, const std::vector<float> &vertices) {
    int pos;
    glm::vec3 normal;
    std::vector<float> normals;
    std::vector<glm::vec3> verts;
    
    for (int i = 0; i < indices.size(); i += 3) {
        
        for (int j = 0; j < 3; j++) {
            pos = indices[i+j]*3;
            verts.push_back(glm::vec3(vertices[pos], vertices[pos+1], vertices[pos+2]));
        }
        
        glm::vec3 U = verts[i+1] - verts[i];
        glm::vec3 V = verts[i+2] - verts[i];
        
        normal = glm::normalize(-glm::cross(U, V));
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }
    
    return normals;
}

std::vector<float> generateBiome(const std::vector<float> &vertices, int xOffset, int yOffset) {
    std::vector<float> colors;
    std::vector<terrainColor> biomeColors;
    glm::vec3 color = glm::vec3(1, 1, 1);
    
    // We assign terrain based on height
    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, deepWater));
    biomeColors.push_back(terrainColor(WATER_HEIGHT, shallowWater));
    biomeColors.push_back(terrainColor(0.15, sand));
    biomeColors.push_back(terrainColor(0.30, grass1));
    biomeColors.push_back(terrainColor(0.40, grass2));
    biomeColors.push_back(terrainColor(0.50, mountain1));
    biomeColors.push_back(terrainColor(0.80, mountain2));
    biomeColors.push_back(terrainColor(1.00, snow));

    for (int i = 1; i < vertices.size(); i += 3) {
        for (int j = 0; j < biomeColors.size(); j++) {
            if (vertices[i] <= biomeColors[j].height * meshHeight) {
                color = biomeColors[j].color;
                break;
            }
        }
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    return colors;
}

// Generate all data for a chunk and send it to GPU
void generateMapChunk(GLuint &VAO, int xOffset, int yOffset) {
    std::vector<int> indices;
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;
    
    indices = generateIndices();
    noise_map = generateNoiseMap(xOffset, yOffset);
    vertices = generateVertices(noise_map);
    normals = generateNormals(indices, vertices);
    colors = generateBiome(vertices, xOffset, yOffset);
    
    GLuint VBO[3], EBO;
    
    glGenBuffers(3, VBO);
    glGenBuffers(1, &EBO);
    glGenVertexArrays(1, &VAO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);
    
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
}

// Initialize SDL and GLAD
void InitializeProgram() {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Set OpenGL version to 4.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // Create window
    window = SDL_CreateWindow("Terrain Generator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, gScreenWidth, gScreenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit(1);
    }

    // Create OpenGL context
    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
    }

    // Set viewport
    glViewport(0, 0, gScreenWidth, gScreenHeight);

    // Enable z-buffer
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);

    // Set swap interval for vertical sync
    SDL_GL_SetSwapInterval(1);
}

int main() {
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 projection;

    InitializeProgram();
    
    Shader shader;
    shader.CreateShader("./shaders/vert.glsl", "./shaders/frag.glsl");
    
    shader.Bind();
    
    shader.SetUniform3f("u_Light.lightPos", 0.0f, 10.0f, 0.0f);
    shader.SetUniform3f("u_Light.ambient", 0.4, 0.4, 0.4);
    shader.SetUniform3f("u_Light.diffuse", 0.3, 0.3, 0.3);
    shader.SetUniform3f("u_Light.specular", 0.5, 0.5, 0.5);
    
    std::vector<GLuint> map_chunks(xMapChunks * yMapChunks);
    
    for (int y = 0; y < yMapChunks; y++) {
        for (int x = 0; x < xMapChunks; x++) {
            generateMapChunk(map_chunks[x + y*xMapChunks], x, y);
        }
    }
    
    int nIndices = chunkWidth * chunkHeight * 6;

    // Main loop
    SDL_Event e;
    while (!quit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // User requests quit
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            const Uint8* state = SDL_GetKeyboardState(NULL);

            // Quit application
            if (state[SDL_SCANCODE_Q]) {
                quit = true;
            }

            // Enable wireframe mode
            if (state[SDL_SCANCODE_E]) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            // Disable wireframe mode
            if (state[SDL_SCANCODE_R]) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            // Camera movement
            if (state[SDL_SCANCODE_W]) {
                camera.MoveForward(1);
            }
            if (state[SDL_SCANCODE_S]) {
                camera.MoveBackward(1);
            }
            if (state[SDL_SCANCODE_A]) {
                camera.MoveLeft(1);
            }
            if (state[SDL_SCANCODE_D]) {
                camera.MoveRight(1);
            }

            // Mouse movement
            if (e.type == SDL_MOUSEMOTION) {
                mouseX += e.motion.xrel;
                mouseY += e.motion.yrel;
                camera.MouseLook(mouseX, mouseY);
            }
        }

        shader.Bind();
        projection = glm::perspective(glm::radians(45.0f), (float)gScreenWidth / (float)gScreenHeight, 0.1f, (float)chunkWidth * (chunk_render_distance - 1.2f));
        view = camera.GetViewMatrix();
        shader.SetUniformMatrix4fv("u_Projection", &projection[0][0]);
        shader.SetUniformMatrix4fv("u_ViewMatrix", &view[0][0]);
        shader.SetUniform3f("u_ViewPos", camera.GetEyeXPosition(), camera.GetEyeYPosition(), camera.GetEyeZPosition());
        
        render(map_chunks, shader, view, model, projection, nIndices);

        // Update window
        SDL_GL_SwapWindow(window);
    }
    
    for (int i = 0; i < map_chunks.size(); i++) {
        glDeleteVertexArrays(1, &map_chunks[i]);
    }

    shader.Unbind();
    
    // Destroy window
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
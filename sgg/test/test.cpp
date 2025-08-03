#include "api/headers/Graphics.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>

struct PyramidVertex {
    float position[3];
    float color[3];
    float normal[3];
};

std::shared_ptr<Shader> pyramidShader;
VAOId pyramidVAO, wireframeVAO;
BufferId pyramidVBO, pyramidIBO, wireframeIBO;
std::unique_ptr<BatchRenderer> batchRenderer;
UniformHandle modelMatrixUniform, viewMatrixUniform, projectionMatrixUniform;
UniformHandle timeUniform;

const PyramidVertex pyramidVertices[] = {
    {{ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // 0: front-left
    {{  0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // 1: front-right
    {{  0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}}, // 2: back-right
    {{ -0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}, // 3: back-left
    {{  0.0f,  0.5f,  0.0f}, {1.0f, 0.5f, 0.8f}, {0.0f, 1.0f, 0.0f}}   // 4: apex
};

const unsigned int pyramidIndices[] = {
    // Base (square) - 2 triangles (counter-clockwise when viewed from below)
    0, 2, 1,  // first triangle
    0, 3, 2,  // second triangle

    // Side faces - 4 triangles (counter-clockwise when viewed from outside)
    0, 1, 4,  // front face
    1, 2, 4,  // right face
    2, 3, 4,  // back face
    3, 0, 4   // left face
};

// Wireframe edges - only the structural edges of the pyramid
const unsigned int wireframeEdges[] = {
    // Base edges (square)
    0, 1,  // front edge
    1, 2,  // right edge
    2, 3,  // back edge
    3, 0,  // left edge

    // Edges from base vertices to apex
    0, 4,  // front-left to apex
    1, 4,  // front-right to apex
    2, 4,  // back-right to apex
    3, 4   // back-left to apex
};

// Vertex shader source
const char* vertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;

out vec3 fragColor;
out vec3 fragNormal;
out vec3 fragPosition;

void main() {
    // Transform position
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    fragPosition = worldPos.xyz;

    // Transform normal
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;

    // Pass color with time-based intensity variation
    float intensity = 0.8 + 0.2 * sin(uTime * 2.0);
    fragColor = aColor * intensity;

    gl_Position = uProjection * uView * worldPos;
}
)";

// Fragment shader source
const char* fragmentShaderSource = R"(
#version 330 core

in vec3 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;

uniform float uTime;

out vec4 FragColor;

void main() {
    // Simple directional lighting
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 normal = normalize(fragNormal);

    float diffuse = max(dot(normal, lightDir), 0.3); // Minimum ambient

    // Add some animated rim lighting
    vec3 viewDir = normalize(-fragPosition);
    float rim = 1.0 - max(dot(viewDir, normal), 0.0);
    rim = pow(rim, 2.0);

    // Time-based color shifting
    vec3 timeColor = vec3(
        0.5 + 0.5 * sin(uTime),
        0.5 + 0.5 * sin(uTime + 2.094),  // 2π/3
        0.5 + 0.5 * sin(uTime + 4.188)   // 4π/3
    );

    vec3 finalColor = fragColor * diffuse + rim * timeColor * 0.3;
    FragColor = vec4(finalColor, 1.0);
}
)";

void initPyramid() {
    std::cout << "Initializing advanced graphics system..." << std::endl;

    // Initialize the advanced graphics system
    if (!advanced_gfx::init()) {
        std::cerr << "Failed to initialize advanced graphics!" << std::endl;
        return;
    }

    // ---- Render Mode ----------------------------------------------------
    // Start in solid mode, let user toggle wireframe with 'W'
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);   // Enable face culling for proper rendering
    glCullFace(GL_BACK);      // Cull back faces
    glFrontFace(GL_CCW);      // Counter-clockwise winding is front-facing

    // Enable depth testing for proper wireframe rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    std::cout << "Creating pyramid shader..." << std::endl;

    // Create shader program
    pyramidShader = advanced_gfx::createShaderFromSource(vertexShaderSource, fragmentShaderSource);
    if (!pyramidShader) {
        std::cerr << "Failed to create pyramid shader!" << std::endl;
        return;
    }

    // Get uniform handles
    modelMatrixUniform = advanced_gfx::getUniform(pyramidShader, "uModel");
    viewMatrixUniform = advanced_gfx::getUniform(pyramidShader, "uView");
    projectionMatrixUniform = advanced_gfx::getUniform(pyramidShader, "uProjection");
    timeUniform = advanced_gfx::getUniform(pyramidShader, "uTime");

    std::cout << "Creating pyramid geometry buffers..." << std::endl;

    // Create vertex buffer
    pyramidVBO = advanced_gfx::createVertexBuffer(
        pyramidVertices,
        sizeof(pyramidVertices),
        BufferUsage::StaticDraw
    );

    // Create index buffer for solid rendering
    pyramidIBO = advanced_gfx::createIndexBuffer(
        pyramidIndices,
        sizeof(pyramidIndices),
        BufferUsage::StaticDraw
    );

    // Create index buffer for wireframe edges
    wireframeIBO = advanced_gfx::createIndexBuffer(
        wireframeEdges,
        sizeof(wireframeEdges),
        BufferUsage::StaticDraw
    );

    // Create VAO for solid rendering
    pyramidVAO = advanced_gfx::createVAO();

    // Create VAO for wireframe rendering
    wireframeVAO = advanced_gfx::createVAO();

    // Configure vertex attributes for solid rendering
    std::vector<VertexAttribute> attributes = {
        VertexAttribute::position(0, sizeof(PyramidVertex), offsetof(PyramidVertex, position)),
        VertexAttribute::color(1, sizeof(PyramidVertex), offsetof(PyramidVertex, color)),
        VertexAttribute::normal(2, sizeof(PyramidVertex), offsetof(PyramidVertex, normal))
    };

    // Set up solid rendering VAO
    advanced_gfx::setVAOAttributes(pyramidVAO, pyramidVBO, attributes);
    advanced_gfx::setIndexBuffer(pyramidVAO, pyramidIBO);

    // Set up wireframe rendering VAO (same vertex attributes, different indices)
    advanced_gfx::setVAOAttributes(wireframeVAO, pyramidVBO, attributes);
    advanced_gfx::setIndexBuffer(wireframeVAO, wireframeIBO);

    // Create a batch renderer for potential future use
    batchRenderer = advanced_gfx::makeDefaultBatch();

    std::cout << "Pyramid initialization complete!" << std::endl;

    // Print buffer statistics
    auto stats = advanced_gfx::getBufferStats();
    std::cout << "Buffer stats - Active buffers: " << stats.activeBuffers
              << ", Total memory: " << stats.totalMemoryUsed << " bytes" << std::endl;
}

// Forward declarations for input handling (defined later)
namespace {
    extern bool wireframeMode;
    void handleInput();
}

void drawPyramid() {
    // Accumulate delta time for continuous rotation
    static float accumulatedTime = 0.0f;
    accumulatedTime += graphics::getDeltaTime(); // milliseconds

    // Convert to seconds
    float timeSec = accumulatedTime * 0.001f;

    constexpr float spinSpeed = 5.f; // radians per second – tweak as desired
    float angle = -timeSec * spinSpeed; // negative for clockwise

    // Create transformation matrices
    glm::mat4 model = glm::mat4(1.0f);

    // Rotate around Y-axis (spinning)
    model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    // Add a slight tilt for more interesting rotation
    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // Scale the pyramid
    model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

    // View matrix (camera)
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 2.0f, 5.0f),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),  // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
    );

    // Projection matrix (perspective)
    float aspect = 800.0f / 600.0f; // TODO: query window size dynamically if needed
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

    // Use our shader
    pyramidShader->use();

    // Set uniforms using the shader's direct uniform access
    (*pyramidShader)["uModel"] = model;
    (*pyramidShader)["uView"] = view;
    (*pyramidShader)["uProjection"] = projection;
    (*pyramidShader)["uTime"] = timeSec;

    if (wireframeMode) {
        // Wireframe rendering - draw only structural edges
        glLineWidth(2.0f);
        advanced_gfx::bindVAO(wireframeVAO);

        // Draw edges as lines
        advanced_gfx::drawElements(
            DrawCommands::PrimitiveType::Lines,
            sizeof(wireframeEdges) / sizeof(wireframeEdges[0]),
            DrawCommands::IndexType::UnsignedInt
        );

        advanced_gfx::unbindVAO();
        glLineWidth(1.0f);
    } else {
        // Solid rendering - draw filled triangles
        advanced_gfx::bindVAO(pyramidVAO);

        // Draw the pyramid using indexed rendering
        advanced_gfx::drawElements(
            DrawCommands::PrimitiveType::Triangles,
            sizeof(pyramidIndices) / sizeof(pyramidIndices[0]),
            DrawCommands::IndexType::UnsignedInt
        );

        advanced_gfx::unbindVAO();
    }
}

void update(float /*deltaTime*/) {
    // Handle input each frame (toggle wireframe, exit, etc.)
    handleInput();
}

void draw() {
    graphics::Brush bgBrush;
    bgBrush.fill_color[0] = 0.1f; // Dark red
    bgBrush.fill_color[1] = 0.1f; // Dark green
    bgBrush.fill_color[2] = 0.2f; // Slightly blue
    graphics::setWindowBackground(bgBrush);

    // Draw our pyramid
    drawPyramid();

    // Reset 2D pose to default for UI overlay
    graphics::resetPose();

    // --------- Overlay FPS / frame time ----------
    float renderMs   = graphics::getRenderTime();   // CPU render/update cost
    float frameTimeMs = graphics::getFrameTime();   // full frame duration
    float fps = graphics::getFPS();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "FPS: " << fps << "  |  Frame: " << frameTimeMs << " ms  |  Render: " << renderMs << " ms  |  Press W for wireframe.";

    graphics::Brush textBrush;
    textBrush.fill_color[0] = textBrush.fill_color[1] = textBrush.fill_color[2] = 1.0f; // white
    textBrush.fill_opacity = 1.0f;

    drawText(10.0f, 20.0f, 16.0f, oss.str(), textBrush);

    // Show wireframe mode status
    if (wireframeMode) {
        drawText(10.0f, 40.0f, 14.0f, "Wireframe Mode: ON", textBrush);
    }
}

void cleanup() {
    std::cout << "Cleaning up advanced graphics resources..." << std::endl;
    advanced_gfx::shutdown();
}

int main() {
    std::cout << "Starting Spinning Pyramid Test - Advanced Graphics API" << std::endl;

    // Initialize simple graphics system for window management
    graphics::createWindow(800, 600, "Advanced Graphics - Spinning Pyramid");
    graphics::setCanvasSize(800, 600);
    graphics::setCanvasScaleMode(graphics::ScaleMode::FIT);

    // Ensure a usable font for FPS overlay
    graphics::setFont("./assets/ARIAL.TTF");

    // Initialize our pyramid
    initPyramid();

    // Set up callbacks
    graphics::setDrawFunction(draw);
    graphics::setUpdateFunction(update);

    // Set performance settings
    graphics::setTargetFPS(500);
    graphics::setVSYNC(false);

    std::cout << "Starting main loop..." << std::endl;

    // Start the main loop
    graphics::startMessageLoop();

    // Cleanup
    cleanup();
    graphics::destroyWindow();

    std::cout << "Spinning Pyramid Test completed!" << std::endl;
    return 0;
}

// Input handling with proper key state tracking to prevent spamming
namespace {
    bool wireframeMode = false;
    bool wKeyWasPressed = false;  // Track previous frame's key state

    void handleInput() {
        // Get current key state
        bool wKeyIsPressed = graphics::getKeyState(graphics::Scancode::W);

        // Toggle wireframe only on key press (not while held)
        if (wKeyIsPressed && !wKeyWasPressed) {
            wireframeMode = !wireframeMode;
            std::cout << "Wireframe mode: " << (wireframeMode ? "ON (True Edges)" : "OFF (Solid)") << std::endl;
        }

        // Update previous key state
        wKeyWasPressed = wKeyIsPressed;
    }
}
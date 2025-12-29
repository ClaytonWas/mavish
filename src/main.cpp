#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <vector>
#include <deque>
#include <algorithm>
#include <numeric>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Window mode constants
const int WINDOW_MODE_WINDOWED = 0;              // Normal resizable window
const int WINDOW_MODE_BORDERLESS = 1;            // Borderless fullscreen window
const int WINDOW_MODE_EXCLUSIVE = 2;             // True exclusive fullscreen

// Game settings structure
struct GameSettings {
    int targetFPS;          // 0 or 301 = uncapped
    float mouseSensitivity;
    float fov;
    float moveSpeed;
    bool vsync;
    bool showFPS;
    int windowMode;         // 0=Windowed, 1=Borderless, 2=Exclusive
    int defaultWidth;
    int defaultHeight;
};

// Performance monitoring structure
struct PerformanceStats {
    std::deque<float> frameTimeHistory;  // Last N frame times
    static const int HISTORY_SIZE = 120; // ~2 seconds at 60fps
    
    float currentFPS;
    float avgFPS;
    float minFPS;
    float maxFPS;
    
    float currentFrameTime;  // in ms
    float avgFrameTime;
    float minFrameTime;
    float maxFrameTime;
    
    int frameCount;
    double totalTime;
    
    int drawCalls;           // Approximate draw calls
    int collisionChecks;
    
    void Update() {
        float dt = GetFrameTime();
        currentFrameTime = dt * 1000.0f;  // Convert to ms
        currentFPS = (dt > 0) ? 1.0f / dt : 0;
        
        frameTimeHistory.push_back(currentFrameTime);
        if (frameTimeHistory.size() > HISTORY_SIZE) {
            frameTimeHistory.pop_front();
        }
        
        // Calculate stats from history
        if (!frameTimeHistory.empty()) {
            float sum = 0;
            minFrameTime = 9999.0f;
            maxFrameTime = 0.0f;
            
            for (float ft : frameTimeHistory) {
                sum += ft;
                if (ft < minFrameTime) minFrameTime = ft;
                if (ft > maxFrameTime) maxFrameTime = ft;
            }
            
            avgFrameTime = sum / frameTimeHistory.size();
            avgFPS = (avgFrameTime > 0) ? 1000.0f / avgFrameTime : 0;
            minFPS = (maxFrameTime > 0) ? 1000.0f / maxFrameTime : 0;
            maxFPS = (minFrameTime > 0) ? 1000.0f / minFrameTime : 0;
        }
        
        frameCount++;
        totalTime += dt;
    }
    
    void Reset() {
        frameTimeHistory.clear();
        frameCount = 0;
        totalTime = 0;
        collisionChecks = 0;
    }
};

// Apply window mode directly
void ApplyWindowMode(int mode, int defaultWidth, int defaultHeight) {
    TraceLog(LOG_INFO, "ApplyWindowMode called with mode: %d", mode);
    
    int monitor = GetCurrentMonitor();
    int monitorWidth = GetMonitorWidth(monitor);
    int monitorHeight = GetMonitorHeight(monitor);
    
    TraceLog(LOG_INFO, "Monitor %d: %dx%d", monitor, monitorWidth, monitorHeight);
    
    // Exit exclusive fullscreen if active
    if (IsWindowFullscreen()) {
        TraceLog(LOG_INFO, "Exiting fullscreen...");
        ToggleFullscreen();
    }
    
    // Clear special window states
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
    ClearWindowState(FLAG_WINDOW_TOPMOST);
    
    // Apply the requested mode
    switch (mode) {
        case WINDOW_MODE_WINDOWED:
            TraceLog(LOG_INFO, "Applying WINDOWED mode");
            SetWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowSize(defaultWidth, defaultHeight);
            SetWindowPosition(
                (monitorWidth - defaultWidth) / 2,
                (monitorHeight - defaultHeight) / 2
            );
            break;
            
        case WINDOW_MODE_BORDERLESS:
            TraceLog(LOG_INFO, "Applying BORDERLESS mode");
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowSize(monitorWidth, monitorHeight);
            SetWindowPosition(0, 0);
            SetWindowState(FLAG_WINDOW_UNDECORATED);
            SetWindowState(FLAG_WINDOW_TOPMOST);
            break;
            
        case WINDOW_MODE_EXCLUSIVE:
            TraceLog(LOG_INFO, "Applying EXCLUSIVE mode");
            ClearWindowState(FLAG_WINDOW_RESIZABLE);
            ToggleFullscreen();
            break;
            
        default:
            TraceLog(LOG_WARNING, "Unknown window mode: %d, falling back to windowed", mode);
            SetWindowState(FLAG_WINDOW_RESIZABLE);
            SetWindowSize(defaultWidth, defaultHeight);
            SetWindowPosition(
                (monitorWidth - defaultWidth) / 2,
                (monitorHeight - defaultHeight) / 2
            );
            break;
    }
    
    TraceLog(LOG_INFO, "Window mode applied successfully");
}

// Player state structure
struct Player {
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float pitch;
    float height;           // Player eye height
    float radius;           // Collision radius
    bool isGrounded;
    bool noclipMode;
};

// Collision box structure
struct CollisionBox {
    Vector3 position;       // Center position
    Vector3 size;           // Full size (width, height, depth)
    Color color;
    Color wireColor;
};

// Physics constants
const float GRAVITY = 20.0f;
const float JUMP_FORCE = 8.0f;
const float GROUND_LEVEL = 0.0f;

// Get bounding box from collision box
BoundingBox GetBoxBounds(const CollisionBox& box) {
    return {
        { box.position.x - box.size.x/2, box.position.y - box.size.y/2, box.position.z - box.size.z/2 },
        { box.position.x + box.size.x/2, box.position.y + box.size.y/2, box.position.z + box.size.z/2 }
    };
}

// Check collision between player (cylinder approximated as box) and a collision box
bool CheckPlayerBoxCollision(Vector3 playerPos, float radius, float height, const CollisionBox& box) {
    BoundingBox playerBox = {
        { playerPos.x - radius, playerPos.y - height, playerPos.z - radius },
        { playerPos.x + radius, playerPos.y, playerPos.z + radius }
    };
    return CheckCollisionBoxes(playerBox, GetBoxBounds(box));
}

// Check if player should have horizontal collision with box (not if standing on top)
bool ShouldApplyHorizontalCollision(Vector3 playerPos, float radius, float height, const CollisionBox& box) {
    BoundingBox boxBounds = GetBoxBounds(box);
    
    // First check if there's any horizontal overlap
    bool horizontalOverlap = 
        (playerPos.x + radius > boxBounds.min.x) && (playerPos.x - radius < boxBounds.max.x) &&
        (playerPos.z + radius > boxBounds.min.z) && (playerPos.z - radius < boxBounds.max.z);
    
    if (!horizontalOverlap) return false;
    
    // Player's feet position
    float feetY = playerPos.y - height;
    
    // If player's feet are at or above the box top, they're standing on it - no horizontal collision
    // Use a small tolerance to prevent edge cases
    if (feetY >= boxBounds.max.y - 0.1f) {
        return false;
    }
    
    // If player's head is below box bottom, no collision (shouldn't happen but safety check)
    if (playerPos.y < boxBounds.min.y) {
        return false;
    }
    
    // Player is at a height where horizontal collision should apply
    return true;
}

// Resolve collision by pushing player out of box
Vector3 ResolveCollision(Vector3 playerPos, float radius, float height, const CollisionBox& box) {
    BoundingBox boxBounds = GetBoxBounds(box);
    
    // Calculate overlap on each axis
    float overlapX1 = (playerPos.x + radius) - boxBounds.min.x;
    float overlapX2 = boxBounds.max.x - (playerPos.x - radius);
    float overlapZ1 = (playerPos.z + radius) - boxBounds.min.z;
    float overlapZ2 = boxBounds.max.z - (playerPos.z - radius);
    
    // Find minimum overlap
    float minOverlapX = (overlapX1 < overlapX2) ? -overlapX1 : overlapX2;
    float minOverlapZ = (overlapZ1 < overlapZ2) ? -overlapZ1 : overlapZ2;
    
    // Push out on axis with smallest overlap
    if (fabsf(minOverlapX) < fabsf(minOverlapZ)) {
        playerPos.x += minOverlapX;
    } else {
        playerPos.z += minOverlapZ;
    }
    
    return playerPos;
}

// Update camera look direction (shared between modes)
void UpdateCameraLook(Player* player, float mouseSensitivity) {
    Vector2 mouseDelta = GetMouseDelta();
    
    player->yaw += mouseDelta.x * mouseSensitivity;
    player->pitch -= mouseDelta.y * mouseSensitivity;
    
    // Clamp pitch to prevent camera flip
    if (player->pitch > 89.0f) player->pitch = 89.0f;
    if (player->pitch < -89.0f) player->pitch = -89.0f;
}

// Get forward direction from player angles
Vector3 GetForwardDirection(const Player* player) {
    Vector3 forward;
    forward.x = cosf(DEG2RAD * player->yaw) * cosf(DEG2RAD * player->pitch);
    forward.y = sinf(DEG2RAD * player->pitch);
    forward.z = sinf(DEG2RAD * player->yaw) * cosf(DEG2RAD * player->pitch);
    return Vector3Normalize(forward);
}

// Get flat forward direction (for walking - ignores pitch)
Vector3 GetFlatForwardDirection(const Player* player) {
    Vector3 forward;
    forward.x = cosf(DEG2RAD * player->yaw);
    forward.y = 0.0f;
    forward.z = sinf(DEG2RAD * player->yaw);
    return Vector3Normalize(forward);
}

// Noclip camera controller (flying mode)
void UpdateNoclipMode(Player* player, float moveSpeed, float mouseSensitivity) {
    float deltaTime = GetFrameTime();
    
    UpdateCameraLook(player, mouseSensitivity);
    
    Vector3 forward = GetForwardDirection(player);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
    Vector3 up = { 0.0f, 1.0f, 0.0f };
    
    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, forward);
    if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, forward);
    if (IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);
    if (IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
    if (IsKeyDown(KEY_SPACE)) moveDir = Vector3Add(moveDir, up);
    if (IsKeyDown(KEY_LEFT_SHIFT)) moveDir = Vector3Subtract(moveDir, up);
    
    float currentSpeed = moveSpeed;
    if (IsKeyDown(KEY_LEFT_CONTROL)) currentSpeed *= 2.5f;
    
    if (Vector3Length(moveDir) > 0.0f) {
        moveDir = Vector3Normalize(moveDir);
        moveDir = Vector3Scale(moveDir, currentSpeed * deltaTime);
    }
    
    player->position = Vector3Add(player->position, moveDir);
    player->velocity = { 0, 0, 0 };
    player->isGrounded = false;
}

// Walking mode with gravity and collision
void UpdateWalkingMode(Player* player, float moveSpeed, float mouseSensitivity, 
                       const std::vector<CollisionBox>& colliders) {
    float deltaTime = GetFrameTime();
    
    UpdateCameraLook(player, mouseSensitivity);
    
    Vector3 forward = GetFlatForwardDirection(player);
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
    
    // Horizontal movement input
    Vector3 moveDir = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, forward);
    if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, forward);
    if (IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);
    if (IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
    
    float currentSpeed = moveSpeed;
    if (IsKeyDown(KEY_LEFT_CONTROL)) currentSpeed *= 2.0f;
    
    if (Vector3Length(moveDir) > 0.0f) {
        moveDir = Vector3Normalize(moveDir);
    }
    
    // Apply horizontal velocity
    player->velocity.x = moveDir.x * currentSpeed;
    player->velocity.z = moveDir.z * currentSpeed;
    
    // Apply gravity
    if (!player->isGrounded) {
        player->velocity.y -= GRAVITY * deltaTime;
    }
    
    // Jump
    if (IsKeyPressed(KEY_SPACE) && player->isGrounded) {
        player->velocity.y = JUMP_FORCE;
        player->isGrounded = false;
    }
    
    // Calculate new position
    Vector3 newPos = player->position;
    newPos.x += player->velocity.x * deltaTime;
    newPos.z += player->velocity.z * deltaTime;
    
    // Check horizontal collisions (only if not standing on top of the box)
    for (const auto& box : colliders) {
        if (ShouldApplyHorizontalCollision(newPos, player->radius, player->height, box)) {
            newPos = ResolveCollision(newPos, player->radius, player->height, box);
        }
    }
    
    // Apply vertical movement
    newPos.y += player->velocity.y * deltaTime;
    
    // Reset grounded state - will be set true if we find ground below
    bool foundGround = false;
    float groundY = GROUND_LEVEL;
    
    // Check ground level first
    if (newPos.y - player->height <= GROUND_LEVEL + 0.05f) {
        foundGround = true;
        groundY = GROUND_LEVEL;
    }
    
    // Check if standing on any box
    for (const auto& box : colliders) {
        BoundingBox bounds = GetBoxBounds(box);
        // Check if player is above the box horizontally
        if (newPos.x + player->radius > bounds.min.x && newPos.x - player->radius < bounds.max.x &&
            newPos.z + player->radius > bounds.min.z && newPos.z - player->radius < bounds.max.z) {
            // Check if player's feet are at or below the box top (with small tolerance)
            float feetY = newPos.y - player->height;
            if (feetY <= bounds.max.y + 0.05f && feetY >= bounds.max.y - 0.5f) {
                // Only count as ground if we're falling or stationary vertically
                if (player->velocity.y <= 0.01f) {
                    if (bounds.max.y > groundY) {
                        groundY = bounds.max.y;
                    }
                    foundGround = true;
                }
            }
        }
    }
    
    // Apply ground detection
    if (foundGround && player->velocity.y <= 0.01f) {
        newPos.y = groundY + player->height;
        player->velocity.y = 0;
        player->isGrounded = true;
    } else {
        player->isGrounded = false;
    }
    
    player->position = newPos;
}

// Update camera from player state
void UpdateCameraFromPlayer(Camera3D* camera, const Player* player) {
    Vector3 forward = GetForwardDirection(player);
    camera->position = player->position;
    camera->target = Vector3Add(player->position, forward);
}

int main()
{
    // Window configuration
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Mavish Game - First Person 3D");
    
    // Disable ESC auto-close so we can use it for menu
    SetExitKey(KEY_NULL);
    
    // Game settings with defaults
    GameSettings settings;
    settings.targetFPS = 60;
    settings.mouseSensitivity = 0.1f;
    settings.fov = 70.0f;
    settings.moveSpeed = 7.0f;
    settings.vsync = false;
    settings.showFPS = true;
    settings.windowMode = WINDOW_MODE_WINDOWED;
    settings.defaultWidth = screenWidth;
    settings.defaultHeight = screenHeight;
    
    SetTargetFPS(settings.targetFPS);
    
    // Menu state
    bool showSettingsMenu = false;
    bool gameWasPaused = false;
    
    // Debug overlay state
    bool showDebugOverlay = false;
    PerformanceStats perfStats = {};
    
    // Lock and hide cursor for FPS controls
    DisableCursor();

    // Player setup
    Player player;
    player.position = { 0.0f, 1.8f, 10.0f };
    player.velocity = { 0.0f, 0.0f, 0.0f };
    player.yaw = -90.0f;
    player.pitch = 0.0f;
    player.height = 1.8f;
    player.radius = 0.3f;
    player.isGrounded = false;
    player.noclipMode = false;

    // Camera setup (first-person perspective)
    Camera3D camera = { 0 };
    camera.position = player.position;
    camera.target = { 0.0f, 1.8f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = settings.fov;
    camera.projection = CAMERA_PERSPECTIVE;

    // Create collision boxes for the scene
    std::vector<CollisionBox> colliders;
    
    // Main center cube
    CollisionBox centerCube;
    centerCube.position = { 0.0f, 1.0f, 0.0f };
    centerCube.size = { 2.0f, 2.0f, 2.0f };
    centerCube.color = RED;
    centerCube.wireColor = MAROON;
    colliders.push_back(centerCube);
    
    // Pillars
    for (int i = -5; i <= 5; i += 2) {
        for (int j = -5; j <= 5; j += 2) {
            if (i == 0 && j == 0) continue;
            float height = 1.0f + (float)(abs(i + j) % 3);
            CollisionBox pillar;
            pillar.position = { (float)i * 3.0f, height / 2.0f, (float)j * 3.0f };
            pillar.size = { 0.5f, height, 0.5f };
            pillar.color = BLUE;
            pillar.wireColor = DARKBLUE;
            colliders.push_back(pillar);
        }
    }
    
    // Game loop
    while (!WindowShouldClose())
    {
        // --- UPDATE ---
        
        // Update performance stats every frame
        perfStats.Update();
        
        // F3 toggles debug overlay
        if (IsKeyPressed(KEY_F3)) {
            showDebugOverlay = !showDebugOverlay;
        }
        
        // Track window mode and apply when changed
        static int appliedWindowMode = WINDOW_MODE_WINDOWED;
        
        // F11 toggles between windowed and borderless
        if (IsKeyPressed(KEY_F11)) {
            if (settings.windowMode == WINDOW_MODE_WINDOWED) {
                settings.windowMode = WINDOW_MODE_BORDERLESS;
            } else {
                settings.windowMode = WINDOW_MODE_WINDOWED;
            }
        }
        
        // Apply window mode when it changes
        if (settings.windowMode != appliedWindowMode) {
            ApplyWindowMode(settings.windowMode, settings.defaultWidth, settings.defaultHeight);
            appliedWindowMode = settings.windowMode;
        }
        
        // Toggle settings menu with Escape
        if (IsKeyPressed(KEY_ESCAPE)) {
            showSettingsMenu = !showSettingsMenu;
            if (showSettingsMenu) {
                EnableCursor();
                gameWasPaused = true;
            } else {
                DisableCursor();
                gameWasPaused = false;
            }
        }
        
        // Only update game if not in menu
        if (!showSettingsMenu) {
            // Toggle noclip with V key
            if (IsKeyPressed(KEY_V)) {
                player.noclipMode = !player.noclipMode;
                if (!player.noclipMode) {
                    // Reset vertical velocity when exiting noclip
                    player.velocity.y = 0;
                }
            }
            
            // Update player based on mode
            if (player.noclipMode) {
                UpdateNoclipMode(&player, settings.moveSpeed * 1.5f, settings.mouseSensitivity);
            } else {
                UpdateWalkingMode(&player, settings.moveSpeed, settings.mouseSensitivity, colliders);
            }
            
            // Update camera from player
            UpdateCameraFromPlayer(&camera, &player);
            
            // Toggle cursor lock with Tab
            if (IsKeyPressed(KEY_TAB)) {
                if (IsCursorHidden()) EnableCursor();
                else DisableCursor();
            }
        }
        
        // Always update camera FOV (so it updates in real-time from menu)
        camera.fovy = settings.fov;
        
        // Get current render size (works correctly in all window modes including fullscreen)
        int currentWidth = GetRenderWidth();
        int currentHeight = GetRenderHeight();

        // --- DRAW ---
        BeginDrawing();
            ClearBackground(DARKGRAY);
            
            BeginMode3D(camera);
                
                // Draw ground plane (grid)
                DrawGrid(50, 1.0f);
                
                // Draw ground plane (solid)
                DrawPlane({ 0.0f, 0.0f, 0.0f }, { 50.0f, 50.0f }, DARKGREEN);
                
                // Draw all collision boxes
                for (const auto& box : colliders) {
                    DrawCube(box.position, box.size.x, box.size.y, box.size.z, box.color);
                    DrawCubeWires(box.position, box.size.x, box.size.y, box.size.z, box.wireColor);
                }
                
                // Draw coordinate axes for reference
                DrawLine3D({ 0, 0, 0 }, { 5, 0, 0 }, RED);     // X axis
                DrawLine3D({ 0, 0, 0 }, { 0, 5, 0 }, GREEN);   // Y axis
                DrawLine3D({ 0, 0, 0 }, { 0, 0, 5 }, BLUE);    // Z axis
                
            EndMode3D();
            
            // Draw crosshair
            DrawLine(currentWidth/2 - 10, currentHeight/2, currentWidth/2 + 10, currentHeight/2, WHITE);
            DrawLine(currentWidth/2, currentHeight/2 - 10, currentWidth/2, currentHeight/2 + 10, WHITE);
            
            // Draw HUD / instructions
            DrawRectangle(10, 10, 340, 175, Fade(BLACK, 0.5f));
            
            // Mode indicator
            if (player.noclipMode) {
                DrawText("MODE: NOCLIP (Flying)", 20, 20, 18, YELLOW);
                DrawText("WASD - Fly horizontally", 20, 45, 16, LIGHTGRAY);
                DrawText("Space/Shift - Fly up/down", 20, 65, 16, LIGHTGRAY);
            } else {
                DrawText("MODE: WALKING", 20, 20, 18, GREEN);
                DrawText("WASD - Walk", 20, 45, 16, LIGHTGRAY);
                DrawText("Space - Jump", 20, 65, 16, LIGHTGRAY);
            }
            
            DrawText("Mouse - Look around", 20, 85, 16, LIGHTGRAY);
            DrawText("Ctrl - Sprint", 20, 105, 16, LIGHTGRAY);
            DrawText("V - Toggle noclip", 20, 125, 16, ORANGE);
            DrawText("Tab - Toggle mouse lock", 20, 145, 16, LIGHTGRAY);
            DrawText("ESC - Settings | F3 - Debug", 20, 165, 16, YELLOW);
            
            // Player status
            DrawRectangle(10, currentHeight - 60, 280, 50, Fade(BLACK, 0.5f));
            DrawText(TextFormat("Position: (%.1f, %.1f, %.1f)", 
                     player.position.x, player.position.y, player.position.z), 
                     20, currentHeight - 50, 16, WHITE);
            DrawText(TextFormat("Grounded: %s | Vel Y: %.1f", 
                     player.isGrounded ? "Yes" : "No", player.velocity.y),
                     20, currentHeight - 30, 16, LIGHTGRAY);
            
            if (settings.showFPS) DrawFPS(currentWidth - 100, 10);
            
            // --- DEBUG OVERLAY (F3) ---
            if (showDebugOverlay) {
                int debugX = currentWidth - 320;
                int debugY = 40;
                int lineHeight = 18;
                
                // Background panel
                DrawRectangle(debugX - 10, debugY - 10, 320, 340, Fade(BLACK, 0.8f));
                DrawRectangleLines(debugX - 10, debugY - 10, 320, 340, LIME);
                
                // Title
                DrawText("DEBUG / PERFORMANCE", debugX, debugY, 18, LIME);
                debugY += lineHeight + 10;
                
                // Frame timing section
                DrawText("-- Frame Timing --", debugX, debugY, 16, YELLOW);
                debugY += lineHeight;
                
                DrawText(TextFormat("Current: %.2f ms (%.0f FPS)", 
                         perfStats.currentFrameTime, perfStats.currentFPS), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Average: %.2f ms (%.0f FPS)", 
                         perfStats.avgFrameTime, perfStats.avgFPS), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Min: %.2f ms (%.0f FPS)", 
                         perfStats.minFrameTime, perfStats.maxFPS), 
                         debugX, debugY, 14, GREEN);
                debugY += lineHeight;
                
                DrawText(TextFormat("Max: %.2f ms (%.0f FPS)", 
                         perfStats.maxFrameTime, perfStats.minFPS), 
                         debugX, debugY, 14, RED);
                debugY += lineHeight + 5;
                
                // Frame time graph
                DrawText("Frame Time Graph (last 120 frames):", debugX, debugY, 14, YELLOW);
                debugY += lineHeight;
                
                int graphWidth = 280;
                int graphHeight = 50;
                DrawRectangle(debugX, debugY, graphWidth, graphHeight, Fade(DARKGRAY, 0.5f));
                DrawRectangleLines(debugX, debugY, graphWidth, graphHeight, GRAY);
                
                // Draw graph bars
                if (!perfStats.frameTimeHistory.empty()) {
                    int barCount = (int)perfStats.frameTimeHistory.size();
                    float barWidth = (float)graphWidth / PerformanceStats::HISTORY_SIZE;
                    float maxDisplayTime = 33.33f;  // Cap at ~30fps for visual scale
                    
                    for (int i = 0; i < barCount; i++) {
                        float ft = perfStats.frameTimeHistory[i];
                        float normalizedHeight = (ft / maxDisplayTime) * graphHeight;
                        if (normalizedHeight > graphHeight) normalizedHeight = graphHeight;
                        
                        Color barColor = GREEN;
                        if (ft > 16.67f) barColor = YELLOW;  // Below 60fps
                        if (ft > 33.33f) barColor = RED;     // Below 30fps
                        
                        DrawRectangle(
                            debugX + (int)(i * barWidth),
                            debugY + graphHeight - (int)normalizedHeight,
                            (int)barWidth + 1,
                            (int)normalizedHeight,
                            barColor
                        );
                    }
                }
                debugY += graphHeight + 10;
                
                // Player info section
                DrawText("-- Player State --", debugX, debugY, 16, YELLOW);
                debugY += lineHeight;
                
                DrawText(TextFormat("Pos: (%.2f, %.2f, %.2f)", 
                         player.position.x, player.position.y, player.position.z), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Vel: (%.2f, %.2f, %.2f)", 
                         player.velocity.x, player.velocity.y, player.velocity.z), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Yaw: %.1f  Pitch: %.1f", player.yaw, player.pitch), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Grounded: %s  Noclip: %s", 
                         player.isGrounded ? "YES" : "NO",
                         player.noclipMode ? "YES" : "NO"), 
                         debugX, debugY, 14, player.isGrounded ? GREEN : RED);
                debugY += lineHeight + 5;
                
                // System info
                DrawText("-- System --", debugX, debugY, 16, YELLOW);
                debugY += lineHeight;
                
                DrawText(TextFormat("Window: %dx%d", currentWidth, currentHeight), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Colliders: %d  Target FPS: %s", 
                         (int)colliders.size(),
                         settings.targetFPS == 301 ? "Uncapped" : TextFormat("%d", settings.targetFPS)), 
                         debugX, debugY, 14, WHITE);
                debugY += lineHeight;
                
                DrawText(TextFormat("Total Frames: %d  Time: %.1fs", 
                         perfStats.frameCount, perfStats.totalTime), 
                         debugX, debugY, 14, GRAY);
            }
            
            // --- SETTINGS MENU ---
            if (showSettingsMenu) {
                // Darken background
                DrawRectangle(0, 0, currentWidth, currentHeight, Fade(BLACK, 0.7f));
                
                // Menu panel
                int panelWidth = 400;
                int panelHeight = 570;
                int panelX = (currentWidth - panelWidth) / 2;
                int panelY = (currentHeight - panelHeight) / 2;
                
                DrawRectangleRounded({ (float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight }, 0.03f, 10, Fade(DARKGRAY, 0.95f));
                DrawRectangleRoundedLinesEx({ (float)panelX, (float)panelY, (float)panelWidth, (float)panelHeight }, 0.03f, 10, 2, LIGHTGRAY);
                
                // Title
                DrawText("SETTINGS", panelX + (panelWidth - MeasureText("SETTINGS", 30)) / 2, panelY + 20, 30, WHITE);
                
                int controlX = panelX + 20;
                int controlWidth = panelWidth - 40;
                int yPos = panelY + 70;
                int spacing = 50;
                
                // FPS Slider (30-300, or 301 = Uncapped)
                DrawText("Target FPS:", controlX, yPos, 16, LIGHTGRAY);
                if (settings.targetFPS == 301) {
                    DrawText("Uncapped", controlX + controlWidth - 70, yPos, 16, YELLOW);
                } else {
                    DrawText(TextFormat("%d", settings.targetFPS), controlX + controlWidth - 40, yPos, 16, WHITE);
                }
                yPos += 22;
                float fpsValue = (float)settings.targetFPS;
                int previousFPS = settings.targetFPS;
                GuiSlider({ (float)controlX, (float)yPos, (float)controlWidth, 20 }, "30", "Max", &fpsValue, 30, 301);
                settings.targetFPS = (int)fpsValue;
                // Only update FPS when value changes
                if (settings.targetFPS != previousFPS) {
                    if (settings.targetFPS == 301) {
                        SetTargetFPS(0);  // 0 = uncapped in raylib
                    } else {
                        SetTargetFPS(settings.targetFPS);
                    }
                }
                yPos += spacing;
                
                // Mouse Sensitivity
                DrawText("Mouse Sensitivity:", controlX, yPos, 16, LIGHTGRAY);
                DrawText(TextFormat("%.2f", settings.mouseSensitivity), controlX + controlWidth - 50, yPos, 16, WHITE);
                yPos += 22;
                GuiSlider({ (float)controlX, (float)yPos, (float)controlWidth, 20 }, NULL, NULL, &settings.mouseSensitivity, 0.01f, 0.5f);
                yPos += spacing;
                
                // FOV
                DrawText("Field of View:", controlX, yPos, 16, LIGHTGRAY);
                DrawText(TextFormat("%.0f", settings.fov), controlX + controlWidth - 40, yPos, 16, WHITE);
                yPos += 22;
                GuiSlider({ (float)controlX, (float)yPos, (float)controlWidth, 20 }, NULL, NULL, &settings.fov, 50.0f, 120.0f);
                yPos += spacing;
                
                // Move Speed
                DrawText("Move Speed:", controlX, yPos, 16, LIGHTGRAY);
                DrawText(TextFormat("%.1f", settings.moveSpeed), controlX + controlWidth - 40, yPos, 16, WHITE);
                yPos += 22;
                GuiSlider({ (float)controlX, (float)yPos, (float)controlWidth, 20 }, NULL, NULL, &settings.moveSpeed, 1.0f, 20.0f);
                yPos += spacing;
                
                // Show FPS Toggle
                GuiCheckBox({ (float)controlX, (float)yPos, 20, 20 }, "Show FPS Counter", &settings.showFPS);
                yPos += 40;
                
                // Window Mode label
                int dropdownY = yPos;
                DrawText("Window Mode (F11):", controlX, yPos, 16, LIGHTGRAY);
                yPos += 35;
                
                // Resume button - draw FIRST (behind dropdown)
                // Only accept clicks if dropdown is closed
                static bool windowModeDropdownOpen = false;
                if (!windowModeDropdownOpen) {
                    if (GuiButton({ (float)controlX, (float)(panelY + panelHeight - 110), (float)controlWidth, 40 }, "Resume Game (ESC)")) {
                        showSettingsMenu = false;
                        DisableCursor();
                        gameWasPaused = false;
                    }
                    // Exit Game button
                    if (GuiButton({ (float)controlX, (float)(panelY + panelHeight - 60), (float)controlWidth, 40 }, "Exit Game")) {
                        CloseWindow();
                        return 0;
                    }
                } else {
                    // Just draw the buttons visually when dropdown is open
                    GuiDisable();
                    GuiButton({ (float)controlX, (float)(panelY + panelHeight - 110), (float)controlWidth, 40 }, "Resume Game (ESC)");
                    GuiButton({ (float)controlX, (float)(panelY + panelHeight - 60), (float)controlWidth, 40 }, "Exit Game");
                    GuiEnable();
                }
                
                // Window Mode Dropdown - draw LAST so it appears on top
                int previousMode = settings.windowMode;
                if (GuiDropdownBox(
                    { (float)controlX, (float)(dropdownY + 22), (float)controlWidth, 25 },
                    "Windowed;Borderless Fullscreen;Exclusive Fullscreen",
                    &settings.windowMode,
                    windowModeDropdownOpen
                )) {
                    windowModeDropdownOpen = !windowModeDropdownOpen;
                }
                
                // Debug: Show if mode changed
                if (settings.windowMode != previousMode) {
                    TraceLog(LOG_INFO, "Window mode changed from %d to %d", previousMode, settings.windowMode);
                }
            }
            
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

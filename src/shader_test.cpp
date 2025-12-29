// shader_test.cpp - Unified shader testing with multiple levels
// Noclip movement, shader toggles on keys T-P, debug overlay

#include "raylib.h"
#include "raymath.h"
#include <cmath>
#include <deque>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Performance stats
struct PerfStats {
    std::deque<float> history;
    static const int SIZE = 120;
    float fps, avgFps, minFps, maxFps;
    float ms, avgMs, minMs, maxMs;
    int frames;
    
    void Update() {
        float dt = GetFrameTime();
        ms = dt * 1000.0f;
        fps = (dt > 0) ? 1.0f / dt : 0;
        history.push_back(ms);
        if (history.size() > SIZE) history.pop_front();
        
        if (!history.empty()) {
            float sum = 0; minMs = 9999; maxMs = 0;
            for (float f : history) {
                sum += f;
                if (f < minMs) minMs = f;
                if (f > maxMs) maxMs = f;
            }
            avgMs = sum / history.size();
            avgFps = (avgMs > 0) ? 1000.0f / avgMs : 0;
            minFps = (maxMs > 0) ? 1000.0f / maxMs : 0;
            maxFps = (minMs > 0) ? 1000.0f / minMs : 0;
        }
        frames++;
    }
};

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Shader Test");
    SetExitKey(KEY_NULL);
    SetTargetFPS(0);
    
    // Player state
    Vector3 position = { 8.0f, 6.0f, 8.0f };
    float yaw = -135.0f, pitch = -15.0f;
    float moveSpeed = 10.0f, mouseSens = 0.1f, fov = 70.0f;
    
    Camera3D camera = { 0 };
    camera.position = position;
    camera.target = { 0, 0, 0 };
    camera.up = { 0, 1, 0 };
    camera.fovy = fov;
    camera.projection = CAMERA_PERSPECTIVE;
    
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    
    // --- SHADERS ---
    Shader waterShader = LoadShader("resources/shaders/water.vs", "resources/shaders/water.fs");
    Shader moebiusShader = LoadShader("resources/shaders/moebius.vs", "resources/shaders/moebius.fs");
    
    int waterTimeLoc = GetShaderLocation(waterShader, "time");
    int waterViewPosLoc = GetShaderLocation(waterShader, "viewPos");
    int moebiusResLoc = GetShaderLocation(moebiusShader, "resolution");
    int moebiusTimeLoc = GetShaderLocation(moebiusShader, "time");
    
    // --- LEVEL 1: Island ---
    Model terrain1 = LoadModelFromMesh(GenMeshCube(6.0f, 1.0f, 6.0f));
    Model water1 = LoadModelFromMesh(GenMeshPlane(20.0f, 20.0f, 32, 32));
    Model water1_plain = LoadModelFromMesh(GenMeshPlane(20.0f, 20.0f, 32, 32));
    Model rock1a = LoadModelFromMesh(GenMeshSphere(0.8f, 8, 8));
    Model rock1b = LoadModelFromMesh(GenMeshSphere(0.5f, 8, 8));
    Model tree1 = LoadModelFromMesh(GenMeshCylinder(0.3f, 2.0f, 8));
    Model foliage1 = LoadModelFromMesh(GenMeshSphere(1.2f, 8, 8));
    
    terrain1.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 180, 140, 100, 255 };
    water1.materials[0].shader = waterShader;
    water1.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 150, 200, 255 };
    water1_plain.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 150, 200, 255 };
    rock1a.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 100, 110, 255 };
    rock1b.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 90, 85, 95, 255 };
    tree1.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 70, 50, 255 };
    foliage1.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 80, 150, 80, 255 };
    
    // --- LEVEL 2: Ruins ---
    Model terrain2 = LoadModelFromMesh(GenMeshCube(8.0f, 1.5f, 8.0f));
    Model water2 = LoadModelFromMesh(GenMeshPlane(25.0f, 25.0f, 32, 32));
    Model water2_plain = LoadModelFromMesh(GenMeshPlane(25.0f, 25.0f, 32, 32));
    Model pillar1 = LoadModelFromMesh(GenMeshCylinder(0.5f, 4.0f, 8));
    Model pillar2 = LoadModelFromMesh(GenMeshCylinder(0.5f, 3.5f, 8));
    Model pillar3 = LoadModelFromMesh(GenMeshCylinder(0.4f, 3.0f, 8));
    Model pillar4 = LoadModelFromMesh(GenMeshCylinder(0.45f, 2.5f, 8));
    Model orb = LoadModelFromMesh(GenMeshSphere(0.8f, 16, 16));
    Model altar = LoadModelFromMesh(GenMeshCube(2.0f, 0.5f, 2.0f));
    
    terrain2.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 160, 130, 100, 255 };
    water2.materials[0].shader = waterShader;
    water2.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 150, 200, 255 };
    water2_plain.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 100, 150, 200, 255 };
    pillar1.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 200, 180, 160, 255 };
    pillar2.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 190, 170, 150, 255 };
    pillar3.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 180, 160, 140, 255 };
    pillar4.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 185, 165, 145, 255 };
    orb.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 220, 180, 80, 255 };
    altar.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 120, 110, 100, 255 };
    
    // --- LEVEL 3: Stress Test (demanding scene) ---
    // Use a knot mesh as "teapot" stand-in (OBJ loading crashes)
    Model teapot = LoadModelFromMesh(GenMeshKnot(1.0f, 0.4f, 128, 64));
    teapot.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 200, 160, 120, 255 };
    
    Model water3 = LoadModelFromMesh(GenMeshPlane(60.0f, 60.0f, 64, 64));  // Bigger, more detailed water
    Model water3_plain = LoadModelFromMesh(GenMeshPlane(60.0f, 60.0f, 64, 64));
    water3.materials[0].shader = waterShader;
    water3.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 80, 130, 180, 255 };
    water3_plain.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 80, 130, 180, 255 };
    
    // Ground platforms
    Model platform3 = LoadModelFromMesh(GenMeshCube(15.0f, 2.0f, 15.0f));
    platform3.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 140, 120, 100, 255 };
    
    // Many spheres for stress
    const int NUM_SPHERES = 50;
    Model spheres3[NUM_SPHERES];
    Vector3 spherePos3[NUM_SPHERES];
    float sphereSpeed3[NUM_SPHERES];
    float spherePhase3[NUM_SPHERES];
    for (int i = 0; i < NUM_SPHERES; i++) {
        float radius = 0.3f + (float)(i % 5) * 0.15f;
        spheres3[i] = LoadModelFromMesh(GenMeshSphere(radius, 16, 16));
        spheres3[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 
            (unsigned char)(100 + i * 3), 
            (unsigned char)(150 - i * 2), 
            (unsigned char)(200 - i), 
            255 
        };
        // Spread around in a circle
        float angle = (float)i / NUM_SPHERES * PI * 2.0f;
        float dist = 8.0f + (i % 3) * 3.0f;
        spherePos3[i] = { cosf(angle) * dist, 2.0f + (i % 4), sinf(angle) * dist };
        sphereSpeed3[i] = 0.5f + (float)(i % 10) * 0.2f;
        spherePhase3[i] = (float)i * 0.5f;
    }
    
    // Many cubes
    const int NUM_CUBES = 40;
    Model cubes3[NUM_CUBES];
    Vector3 cubePos3[NUM_CUBES];
    float cubeRotSpeed3[NUM_CUBES];
    for (int i = 0; i < NUM_CUBES; i++) {
        float size = 0.5f + (float)(i % 4) * 0.3f;
        cubes3[i] = LoadModelFromMesh(GenMeshCube(size, size, size));
        cubes3[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 
            (unsigned char)(200 - i * 2), 
            (unsigned char)(100 + i * 2), 
            (unsigned char)(80 + i), 
            255 
        };
        float angle = (float)i / NUM_CUBES * PI * 2.0f + PI / 4.0f;
        float dist = 5.0f + (i % 5) * 2.0f;
        cubePos3[i] = { cosf(angle) * dist, 3.0f + (i % 3) * 2.0f, sinf(angle) * dist };
        cubeRotSpeed3[i] = 20.0f + (float)(i % 8) * 15.0f;
    }
    
    // Pillars/columns
    const int NUM_PILLARS3 = 16;
    Model pillars3[NUM_PILLARS3];
    Vector3 pillarPos3[NUM_PILLARS3];
    for (int i = 0; i < NUM_PILLARS3; i++) {
        float height = 4.0f + (float)(i % 3) * 2.0f;
        pillars3[i] = LoadModelFromMesh(GenMeshCylinder(0.6f, height, 12));
        pillars3[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 180, 170, 160, 255 };
        float angle = (float)i / NUM_PILLARS3 * PI * 2.0f;
        pillarPos3[i] = { cosf(angle) * 18.0f, height / 2.0f + 1.0f, sinf(angle) * 18.0f };
    }
    
    // Torus rings
    const int NUM_TORUS = 8;
    Model torus3[NUM_TORUS];
    Vector3 torusPos3[NUM_TORUS];
    for (int i = 0; i < NUM_TORUS; i++) {
        torus3[i] = LoadModelFromMesh(GenMeshTorus(0.3f, 1.2f + (float)(i % 3) * 0.3f, 16, 16));
        torus3[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 
            (unsigned char)(220 - i * 10), 
            (unsigned char)(180 + i * 5), 
            (unsigned char)(100), 
            255 
        };
        float angle = (float)i / NUM_TORUS * PI * 2.0f;
        torusPos3[i] = { cosf(angle) * 12.0f, 5.0f + sinf((float)i) * 2.0f, sinf(angle) * 12.0f };
    }
    
    // Cones
    const int NUM_CONES = 12;
    Model cones3[NUM_CONES];
    Vector3 conePos3[NUM_CONES];
    for (int i = 0; i < NUM_CONES; i++) {
        cones3[i] = LoadModelFromMesh(GenMeshCone(0.5f + (float)(i % 3) * 0.2f, 1.5f, 8));
        cones3[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 
            (unsigned char)(150 + i * 5), 
            (unsigned char)(80 + i * 3), 
            (unsigned char)(60 + i * 4), 
            255 
        };
        float angle = (float)i / NUM_CONES * PI * 2.0f + 0.3f;
        float dist = 10.0f + (i % 4) * 1.5f;
        conePos3[i] = { cosf(angle) * dist, 1.75f, sinf(angle) * dist };
    }
    
    DisableCursor();
    
    // State
    bool showMenu = false;
    bool showDebug = true;
    bool showFps = true;
    int currentLevel = 1;
    float time = 0.0f;
    PerfStats perf = {};
    
    // Shader toggles: T, Y, U, I, O, P (6 slots for future shaders)
    bool waterEnabled = true;    // T
    bool moebiusEnabled = true;  // Y
    bool shader3 = false;        // U - placeholder
    bool shader4 = false;        // I - placeholder
    bool shader5 = false;        // O - placeholder
    bool shader6 = false;        // P - placeholder
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        time += dt;
        perf.Update();
        
        int w = GetRenderWidth();
        int h = GetRenderHeight();
        
        if (target.texture.width != w || target.texture.height != h) {
            UnloadRenderTexture(target);
            target = LoadRenderTexture(w, h);
        }
        
        // --- INPUT ---
        if (IsKeyPressed(KEY_ESCAPE)) {
            showMenu = !showMenu;
            if (showMenu) EnableCursor(); else DisableCursor();
        }
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;
        if (IsKeyPressed(KEY_ONE)) currentLevel = 1;
        if (IsKeyPressed(KEY_TWO)) currentLevel = 2;
        if (IsKeyPressed(KEY_THREE)) currentLevel = 3;
        
        // Shader toggles T-P
        if (IsKeyPressed(KEY_T)) waterEnabled = !waterEnabled;
        if (IsKeyPressed(KEY_Y)) moebiusEnabled = !moebiusEnabled;
        if (IsKeyPressed(KEY_U)) shader3 = !shader3;
        if (IsKeyPressed(KEY_I)) shader4 = !shader4;
        if (IsKeyPressed(KEY_O)) shader5 = !shader5;
        if (IsKeyPressed(KEY_P)) shader6 = !shader6;
        
        // Hot reload
        if (IsKeyPressed(KEY_R)) {
            UnloadShader(waterShader);
            UnloadShader(moebiusShader);
            waterShader = LoadShader("resources/shaders/water.vs", "resources/shaders/water.fs");
            moebiusShader = LoadShader("resources/shaders/moebius.vs", "resources/shaders/moebius.fs");
            water1.materials[0].shader = waterShader;
            water2.materials[0].shader = waterShader;
            water3.materials[0].shader = waterShader;
            waterTimeLoc = GetShaderLocation(waterShader, "time");
            waterViewPosLoc = GetShaderLocation(waterShader, "viewPos");
            moebiusResLoc = GetShaderLocation(moebiusShader, "resolution");
            moebiusTimeLoc = GetShaderLocation(moebiusShader, "time");
        }
        
        // --- NOCLIP MOVEMENT ---
        if (!showMenu) {
            Vector2 mouse = GetMouseDelta();
            yaw += mouse.x * mouseSens;
            pitch -= mouse.y * mouseSens;
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
            
            Vector3 forward = {
                cosf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch),
                sinf(DEG2RAD * pitch),
                sinf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch)
            };
            forward = Vector3Normalize(forward);
            Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, {0, 1, 0}));
            Vector3 up = { 0, 1, 0 };
            
            Vector3 move = { 0, 0, 0 };
            if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
            if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
            if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
            if (IsKeyDown(KEY_E)) move = Vector3Add(move, up);
            if (IsKeyDown(KEY_Q)) move = Vector3Subtract(move, up);
            
            float speed = moveSpeed;
            if (IsKeyDown(KEY_LEFT_SHIFT)) speed *= 2.5f;
            
            if (Vector3Length(move) > 0) {
                move = Vector3Normalize(move);
                position = Vector3Add(position, Vector3Scale(move, speed * dt));
            }
            
            camera.position = position;
            camera.target = Vector3Add(position, forward);
        }
        
        camera.fovy = fov;
        
        // --- UPDATE SHADERS ---
        SetShaderValue(waterShader, waterTimeLoc, &time, SHADER_UNIFORM_FLOAT);
        float camPos[3] = { position.x, position.y, position.z };
        SetShaderValue(waterShader, waterViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
        float res[2] = { (float)w, (float)h };
        SetShaderValue(moebiusShader, moebiusResLoc, res, SHADER_UNIFORM_VEC2);
        SetShaderValue(moebiusShader, moebiusTimeLoc, &time, SHADER_UNIFORM_FLOAT);
        
        // --- RENDER TO TEXTURE ---
        BeginTextureMode(target);
            ClearBackground((Color){ 180, 210, 240, 255 });
            
            BeginMode3D(camera);
                if (currentLevel == 1) {
                    DrawModel(terrain1, (Vector3){ 0, 0.5f, 0 }, 1.0f, WHITE);
                    DrawModel(rock1a, (Vector3){ -1.5f, 1.0f, 1.0f }, 1.0f, WHITE);
                    DrawModel(rock1b, (Vector3){ 2.0f, 1.0f, -1.5f }, 1.0f, WHITE);
                    DrawModel(tree1, (Vector3){ 0.5f, 2.0f, 0.5f }, 1.0f, WHITE);
                    DrawModel(foliage1, (Vector3){ 0.5f, 3.5f, 0.5f }, 1.0f, WHITE);
                    // Draw water - shader version or plain
                    if (waterEnabled)
                        DrawModel(water1, (Vector3){ 0, -0.2f, 0 }, 1.0f, WHITE);
                    else
                        DrawModel(water1_plain, (Vector3){ 0, -0.2f, 0 }, 1.0f, WHITE);
                } else if (currentLevel == 2) {
                    DrawModel(terrain2, (Vector3){ 0, 0.25f, 0 }, 1.0f, WHITE);
                    DrawModel(pillar1, (Vector3){ -2.5f, 3.0f, -2.5f }, 1.0f, WHITE);
                    DrawModel(pillar2, (Vector3){ 2.5f, 2.75f, -2.5f }, 1.0f, WHITE);
                    DrawModel(pillar3, (Vector3){ -2.5f, 2.5f, 2.5f }, 1.0f, WHITE);
                    DrawModel(pillar4, (Vector3){ 2.5f, 2.25f, 2.5f }, 1.0f, WHITE);
                    DrawModel(altar, (Vector3){ 0, 1.25f, 0 }, 1.0f, WHITE);
                    float bob = sinf(time * 2.0f) * 0.3f;
                    DrawModel(orb, (Vector3){ 0, 3.0f + bob, 0 }, 1.0f, WHITE);
                    // Draw water - shader version or plain
                    if (waterEnabled)
                        DrawModel(water2, (Vector3){ 0, -0.3f, 0 }, 1.0f, WHITE);
                    else
                        DrawModel(water2_plain, (Vector3){ 0, -0.3f, 0 }, 1.0f, WHITE);
                } else if (currentLevel == 3) {
                    // --- STRESS TEST SCENE ---
                    DrawModel(platform3, (Vector3){ 0, 0, 0 }, 1.0f, WHITE);
                    
                    // Central spinning teapot
                    DrawModelEx(teapot, (Vector3){ 0, 3.0f, 0 }, (Vector3){ 0, 1, 0 }, time * 30.0f, (Vector3){ 2.0f, 2.0f, 2.0f }, WHITE);
                    
                    // Orbiting teapots
                    for (int i = 0; i < 6; i++) {
                        float angle = time * 0.5f + (float)i * PI / 3.0f;
                        float dist = 6.0f;
                        Vector3 pos = { cosf(angle) * dist, 2.5f + sinf(time * 2.0f + i) * 0.5f, sinf(angle) * dist };
                        DrawModelEx(teapot, pos, (Vector3){ 0, 1, 0 }, -time * 45.0f, (Vector3){ 1.0f, 1.0f, 1.0f }, WHITE);
                    }
                    
                    // Bouncing spheres
                    for (int i = 0; i < NUM_SPHERES; i++) {
                        float bounce = fabsf(sinf(time * sphereSpeed3[i] + spherePhase3[i])) * 2.0f;
                        Vector3 pos = spherePos3[i];
                        pos.y += bounce;
                        DrawModel(spheres3[i], pos, 1.0f, WHITE);
                    }
                    
                    // Rotating cubes
                    for (int i = 0; i < NUM_CUBES; i++) {
                        DrawModelEx(cubes3[i], cubePos3[i], (Vector3){ 1, 1, 0 }, time * cubeRotSpeed3[i], (Vector3){ 1, 1, 1 }, WHITE);
                    }
                    
                    // Static pillars
                    for (int i = 0; i < NUM_PILLARS3; i++) {
                        DrawModel(pillars3[i], pillarPos3[i], 1.0f, WHITE);
                    }
                    
                    // Spinning torus rings
                    for (int i = 0; i < NUM_TORUS; i++) {
                        Vector3 pos = torusPos3[i];
                        pos.y += sinf(time * 1.5f + (float)i) * 1.0f;
                        DrawModelEx(torus3[i], pos, (Vector3){ 1, 0, 0 }, time * 60.0f + i * 45.0f, (Vector3){ 1, 1, 1 }, WHITE);
                    }
                    
                    // Static cones
                    for (int i = 0; i < NUM_CONES; i++) {
                        DrawModel(cones3[i], conePos3[i], 1.0f, WHITE);
                    }
                    
                    // Water
                    if (waterEnabled)
                        DrawModel(water3, (Vector3){ 0, -0.5f, 0 }, 1.0f, WHITE);
                    else
                        DrawModel(water3_plain, (Vector3){ 0, -0.5f, 0 }, 1.0f, WHITE);
                }
                DrawGrid(20, 1.0f);
            EndMode3D();
        EndTextureMode();
        
        // --- RENDER TO SCREEN ---
        BeginDrawing();
            ClearBackground(BLACK);
            
            if (moebiusEnabled) BeginShaderMode(moebiusShader);
            DrawTextureRec(target.texture, 
                (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height },
                (Vector2){ 0, 0 }, WHITE);
            if (moebiusEnabled) EndShaderMode();
            
            // --- FPS ---
            if (showFps) DrawFPS(w - 100, 10);
            
            // --- DEBUG OVERLAY (F3) ---
            if (showDebug) {
                int dx = w - 300, dy = 40, lh = 16;
                
                DrawRectangle(dx - 10, dy - 10, 300, 280, Fade(BLACK, 0.75f));
                DrawRectangleLines(dx - 10, dy - 10, 300, 280, LIME);
                
                DrawText("DEBUG", dx, dy, 16, LIME); dy += lh + 8;
                
                DrawText(TextFormat("%.1f ms (%.0f FPS)", perf.ms, perf.fps), dx, dy, 14, WHITE); dy += lh;
                DrawText(TextFormat("Avg: %.1f ms (%.0f FPS)", perf.avgMs, perf.avgFps), dx, dy, 14, GRAY); dy += lh;
                DrawText(TextFormat("Min: %.1f ms  Max: %.1f ms", perf.minMs, perf.maxMs), dx, dy, 14, GRAY); dy += lh + 8;
                
                // Frame graph
                int gw = 260, gh = 40;
                DrawRectangle(dx, dy, gw, gh, Fade(DARKGRAY, 0.5f));
                if (!perf.history.empty()) {
                    float bw = (float)gw / PerfStats::SIZE;
                    for (int i = 0; i < (int)perf.history.size(); i++) {
                        float h = (perf.history[i] / 33.33f) * gh;
                        if (h > gh) h = gh;
                        Color c = perf.history[i] < 16.67f ? GREEN : (perf.history[i] < 33.33f ? YELLOW : RED);
                        DrawRectangle(dx + (int)(i * bw), dy + gh - (int)h, (int)bw + 1, (int)h, c);
                    }
                }
                dy += gh + 10;
                
                DrawText(TextFormat("Pos: %.1f, %.1f, %.1f", position.x, position.y, position.z), dx, dy, 14, WHITE); dy += lh;
                DrawText(TextFormat("Yaw: %.1f  Pitch: %.1f", yaw, pitch), dx, dy, 14, GRAY); dy += lh + 8;
                
                DrawText("Shaders (T-P to toggle):", dx, dy, 14, YELLOW); dy += lh;
                DrawText(TextFormat("T Water: %s", waterEnabled ? "ON" : "OFF"), dx, dy, 14, waterEnabled ? GREEN : RED); dy += lh;
                DrawText(TextFormat("Y Moebius: %s", moebiusEnabled ? "ON" : "OFF"), dx, dy, 14, moebiusEnabled ? GREEN : RED); dy += lh;
                DrawText(TextFormat("U Slot3: %s", shader3 ? "ON" : "OFF"), dx, dy, 14, shader3 ? GREEN : DARKGRAY); dy += lh;
                DrawText(TextFormat("I Slot4: %s", shader4 ? "ON" : "OFF"), dx, dy, 14, shader4 ? GREEN : DARKGRAY); dy += lh;
                DrawText(TextFormat("O Slot5: %s", shader5 ? "ON" : "OFF"), dx, dy, 14, shader5 ? GREEN : DARKGRAY); dy += lh;
                DrawText(TextFormat("P Slot6: %s", shader6 ? "ON" : "OFF"), dx, dy, 14, shader6 ? GREEN : DARKGRAY);
            }
            
            // --- MINIMAL HUD ---
            DrawRectangle(10, 10, 220, 50, Fade(BLACK, 0.5f));
            DrawText(TextFormat("Level %d%s", currentLevel, currentLevel == 3 ? " (STRESS)" : ""), 20, 18, 18, WHITE);
            DrawText("1/2/3 Level | ESC Menu | F3 Debug", 20, 40, 10, GRAY);
            
            // --- SETTINGS MENU ---
            if (showMenu) {
                DrawRectangle(0, 0, w, h, Fade(BLACK, 0.7f));
                
                int pw = 350, ph = 400;
                int px = (w - pw) / 2, py = (h - ph) / 2;
                
                DrawRectangleRounded({ (float)px, (float)py, (float)pw, (float)ph }, 0.03f, 10, Fade(DARKGRAY, 0.95f));
                DrawRectangleRoundedLinesEx({ (float)px, (float)py, (float)pw, (float)ph }, 0.03f, 10, 2, LIGHTGRAY);
                
                DrawText("SETTINGS", px + (pw - MeasureText("SETTINGS", 24)) / 2, py + 15, 24, WHITE);
                
                int cx = px + 20, cw = pw - 40, yp = py + 55;
                
                // Mouse sensitivity
                DrawText("Mouse Sens:", cx, yp, 14, LIGHTGRAY);
                DrawText(TextFormat("%.2f", mouseSens), cx + cw - 40, yp, 14, WHITE);
                yp += 18;
                GuiSlider({ (float)cx, (float)yp, (float)cw, 18 }, NULL, NULL, &mouseSens, 0.01f, 0.5f);
                yp += 30;
                
                // FOV
                DrawText("FOV:", cx, yp, 14, LIGHTGRAY);
                DrawText(TextFormat("%.0f", fov), cx + cw - 30, yp, 14, WHITE);
                yp += 18;
                GuiSlider({ (float)cx, (float)yp, (float)cw, 18 }, NULL, NULL, &fov, 50.0f, 120.0f);
                yp += 30;
                
                // Move speed
                DrawText("Move Speed:", cx, yp, 14, LIGHTGRAY);
                DrawText(TextFormat("%.1f", moveSpeed), cx + cw - 30, yp, 14, WHITE);
                yp += 18;
                GuiSlider({ (float)cx, (float)yp, (float)cw, 18 }, NULL, NULL, &moveSpeed, 1.0f, 30.0f);
                yp += 35;
                
                // Toggles
                GuiCheckBox({ (float)cx, (float)yp, 18, 18 }, "Show FPS", &showFps); yp += 25;
                GuiCheckBox({ (float)cx, (float)yp, 18, 18 }, "Show Debug (F3)", &showDebug); yp += 35;
                
                DrawText("Shader Toggles:", cx, yp, 14, YELLOW); yp += 20;
                GuiCheckBox({ (float)cx, (float)yp, 18, 18 }, "T - Water", &waterEnabled); yp += 22;
                GuiCheckBox({ (float)cx, (float)yp, 18, 18 }, "Y - Moebius", &moebiusEnabled); yp += 30;
                
                if (GuiButton({ (float)cx, (float)(py + ph - 90), (float)cw, 35 }, "Resume (ESC)")) {
                    showMenu = false;
                    DisableCursor();
                }
                if (GuiButton({ (float)cx, (float)(py + ph - 50), (float)cw, 35 }, "Exit")) {
                    break;
                }
            }
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadModel(terrain1); UnloadModel(water1); UnloadModel(water1_plain); UnloadModel(rock1a); UnloadModel(rock1b);
    UnloadModel(tree1); UnloadModel(foliage1);
    UnloadModel(terrain2); UnloadModel(water2); UnloadModel(water2_plain); UnloadModel(pillar1); UnloadModel(pillar2);
    UnloadModel(pillar3); UnloadModel(pillar4); UnloadModel(orb); UnloadModel(altar);
    
    // Level 3 cleanup
    UnloadModel(teapot); UnloadModel(water3); UnloadModel(water3_plain); UnloadModel(platform3);
    for (int i = 0; i < NUM_SPHERES; i++) UnloadModel(spheres3[i]);
    for (int i = 0; i < NUM_CUBES; i++) UnloadModel(cubes3[i]);
    for (int i = 0; i < NUM_PILLARS3; i++) UnloadModel(pillars3[i]);
    for (int i = 0; i < NUM_TORUS; i++) UnloadModel(torus3[i]);
    for (int i = 0; i < NUM_CONES; i++) UnloadModel(cones3[i]);
    
    UnloadShader(waterShader); UnloadShader(moebiusShader);
    UnloadRenderTexture(target);
    CloseWindow();
    
    return 0;
}

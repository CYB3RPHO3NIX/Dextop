#include <windows.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// If using a different backend (e.g., DirectX), adjust includes and init accordingly.

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // Initialize GLFW
    if (!glfwInit())
        return -1;

    // Set OpenGL version (ImGui default is 3.0+)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // Remove OS window decorations
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE); // Start maximized
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // Uncomment to prevent resizing

    // Create window with OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "Dextop", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Set global font to Consolas
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 16.0f);

    // Setup ImGui style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_TitleBg]        = ImVec4(0.15f, 0.15f, 0.15f, 1.0f); // dark grey for inactive window title bar
    style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.10f, 0.40f, 0.80f, 1.0f); // blue for active window title bar
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f); // dark for collapsed window title bar

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool show_window = true;

    // Main loop
    while (show_window && !glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Set window background to black by clearing the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Get main viewport once for this frame
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        // Draw fixed menu bar at the top
        ImVec2 menubar_pos = ImVec2(viewport->Pos.x, viewport->Pos.y);
        ImVec2 menubar_size = ImVec2(viewport->Size.x, 28.0f); // 28px height for menu bar
        ImGui::SetNextWindowPos(menubar_pos);
        ImGui::SetNextWindowSize(menubar_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGuiWindowFlags menubar_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                         ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar;
        ImGui::Begin("##fixedmenubar", nullptr, menubar_flags);
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) { /* handle New */ }
                if (ImGui::MenuItem("Open")) { /* handle Open */ }
                if (ImGui::BeginMenu("Recent")) {
                    if (ImGui::MenuItem("File1.txt")) { /* handle File1 */ }
                    if (ImGui::MenuItem("File2.txt")) { /* handle File2 */ }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo")) { /* handle Undo */ }
                if (ImGui::MenuItem("Redo")) { /* handle Redo */ }
                ImGui::EndMenu();
            }
            // Add Settings menu
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::MenuItem("Preferences")) { /* handle Preferences */ }
                if (ImGui::MenuItem("Themes")) { /* handle Themes */ }
                ImGui::EndMenu();
            }
            // Add Exit button to the extreme right with custom colors
            float button_width = 60.0f;
            float margin = 8.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - button_width - margin);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));         // light red
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // red
            if (ImGui::Button("Exit", ImVec2(button_width, 0))) {
                show_window = false;
            }
            ImGui::PopStyleColor(2);
            ImGui::EndMenuBar();
        }
        ImGui::End();

        ImGui::Begin("Hello, world!");
        ImGui::Text("This is a black window using ImGui via vcpkg.");
        ImGui::End();

        // Draw fixed status bar at the bottom
        ImVec2 statusbar_size = ImVec2(viewport->Size.x, 24.0f); // 24px height for status bar
        ImVec2 statusbar_pos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - statusbar_size.y);
        ImGui::SetNextWindowPos(statusbar_pos);
        ImGui::SetNextWindowSize(statusbar_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGuiWindowFlags statusbar_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##statusbar", nullptr, statusbar_flags);
        ImGui::TextUnformatted("Status: Ready");
        ImGui::End();

        // Draw fixed Directory Tree window on the left
        float sidebar_width = 240.0f;
        float sidebar_top = menubar_pos.y + menubar_size.y;
        float sidebar_height = viewport->Size.y - menubar_size.y - 24.0f; // 24px for status bar
        ImVec2 sidebar_pos = ImVec2(viewport->Pos.x, sidebar_top);
        ImVec2 sidebar_size = ImVec2(sidebar_width, sidebar_height);
        ImGui::SetNextWindowPos(sidebar_pos);
        ImGui::SetNextWindowSize(sidebar_size);
        ImGuiWindowFlags sidebar_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Directory Tree", nullptr, sidebar_flags);
        ImGui::Text("Directory Tree Content");
        ImGui::End();

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

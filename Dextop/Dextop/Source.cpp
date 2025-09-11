#include <windows.h>
#include <cstdio>
#include <cstring>
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
        ImVec2 menubar_pos = ImVec2(viewport->Pos.x, viewport->Pos.y);
        ImVec2 menubar_size = ImVec2(viewport->Size.x, 28.0f);
        ImVec2 statusbar_size = ImVec2(viewport->Size.x, 24.0f);
        ImVec2 statusbar_pos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - statusbar_size.y);
        float sidebar_width = 240.0f;
        float sidebar_top = menubar_pos.y + menubar_size.y;
        float sidebar_height = viewport->Size.y - menubar_size.y - statusbar_size.y;
        ImVec2 sidebar_pos = ImVec2(viewport->Pos.x, sidebar_top);
        ImVec2 sidebar_size = ImVec2(sidebar_width, sidebar_height);
        float center_left = sidebar_pos.x + sidebar_size.x;
        float center_top = menubar_pos.y + menubar_size.y;
        float center_width = viewport->Size.x - sidebar_size.x;
        float center_height = viewport->Size.y - menubar_size.y - statusbar_size.y;
        ImVec2 center_pos = ImVec2(center_left, center_top);
        ImVec2 center_size = ImVec2(center_width, center_height);

        // Draw fixed menu bar at the top
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

        // Draw fixed Directory Tree window on the left
        ImGui::SetNextWindowPos(sidebar_pos);
        ImGui::SetNextWindowSize(sidebar_size);
        ImGuiWindowFlags sidebar_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Directory Tree", nullptr, sidebar_flags);

        // Enumerate drives
        char drives[256];
        DWORD drive_count = GetLogicalDriveStringsA(sizeof(drives), drives);
        for (char* drive = drives; *drive; drive += strlen(drive) + 1) {
            if (ImGui::TreeNode(drive)) {
                // Enumerate folders in this drive
                char search_path[MAX_PATH];
                wsprintfA(search_path, "%s*", drive);
                WIN32_FIND_DATAA find_data;
                HANDLE hFind = FindFirstFileA(search_path, &find_data);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                            strcmp(find_data.cFileName, ".") != 0 &&
                            strcmp(find_data.cFileName, "..") != 0) {
                            // Build full path for subfolder
                            char subfolder_path[MAX_PATH];
                            wsprintfA(subfolder_path, "%s%s\\", drive, find_data.cFileName);
                            if (ImGui::TreeNode(find_data.cFileName)) {
                                // Enumerate subfolders (one level deep)
                                char sub_search[MAX_PATH];
                                wsprintfA(sub_search, "%s*", subfolder_path);
                                WIN32_FIND_DATAA sub_find;
                                HANDLE hSubFind = FindFirstFileA(sub_search, &sub_find);
                                if (hSubFind != INVALID_HANDLE_VALUE) {
                                    do {
                                        if ((sub_find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                                            strcmp(sub_find.cFileName, ".") != 0 &&
                                            strcmp(sub_find.cFileName, "..") != 0) {
                                            ImGui::BulletText("%s", sub_find.cFileName);
                                        }
                                    } while (FindNextFileA(hSubFind, &sub_find));
                                    FindClose(hSubFind);
                                }
                                ImGui::TreePop();
                            }
                        }
                    } while (FindNextFileA(hFind, &find_data));
                    FindClose(hFind);
                }
                ImGui::TreePop();
            }
        }

        ImGui::End();

        // Draw fixed central window
        ImGui::SetNextWindowPos(center_pos);
        ImGui::SetNextWindowSize(center_size);
        ImGuiWindowFlags center_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Central Window", nullptr, center_flags);
        ImGui::Text("Central Window Content");
        ImGui::End();

        // Draw fixed status bar at the bottom
        ImGui::SetNextWindowPos(statusbar_pos);
        ImGui::SetNextWindowSize(statusbar_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGuiWindowFlags statusbar_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##statusbar", nullptr, statusbar_flags);
        ImGui::TextUnformatted("Status: Ready");
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

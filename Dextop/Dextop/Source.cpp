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
    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui Black Window", nullptr, nullptr);
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

        // Draw smaller red close button at absolute top right with curved corners and centered X
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 button_size = ImVec2(24, 24);
        ImVec2 button_pos = ImVec2(viewport->Pos.x + viewport->Size.x - button_size.x - 4, viewport->Pos.y + 4);
        ImGui::SetNextWindowPos(button_pos);
        ImGui::SetNextWindowSize(button_size);
        ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::Begin("##closebtn", nullptr, flags);
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 30, 30, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 60, 60, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 0, 0, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f); // Curved corners
        ImGui::SetCursorPos(ImVec2(0, 0)); // Top-left of the window
        if (ImGui::Button("X", button_size))
        {
            show_window = false;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        ImGui::End();

        ImGui::Begin("Hello, world!");
        ImGui::Text("This is a black window using ImGui via vcpkg.");
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

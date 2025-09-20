#include <windows.h>
#include <cstdio>
#include <cstring>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>

// Helper struct for folder stats
struct FolderStats {
    ULONGLONG size = 0;
    int files = 0; // total files (recursive)
};

// Recursively compute folder stats (size and file count only)
void GetFolderStatsRecursive(const std::string& folder, FolderStats& stats, std::atomic<bool>* cancel_flag = nullptr) {
    if (cancel_flag && *cancel_flag) return;
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s*", folder.c_str());
    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (cancel_flag && *cancel_flag) break;
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                continue;
            bool is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (is_dir) {
                std::string subfolder = folder + find_data.cFileName + "\\";
                GetFolderStatsRecursive(subfolder, stats, cancel_flag);
            } else {
                stats.files++;
                LARGE_INTEGER fsize;
                fsize.HighPart = find_data.nFileSizeHigh;
                fsize.LowPart = find_data.nFileSizeLow;
                stats.size += fsize.QuadPart;
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
}

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
    style.FrameRounding = 2.0f; // Reduced curve for all buttons except Exit

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool show_window = true;
    static bool show_preferences = false; // Controls Preferences window visibility
    // Async folder stats state
    static std::thread folder_stats_thread;
    static std::atomic<bool> folder_stats_running = false;
    static std::atomic<bool> folder_stats_cancel = false;
    static std::string folder_stats_path;
    static FolderStats folder_stats_result;

    // Track current directory for central window
    std::string current_dir = "C:\\"; // Start at C:\
    std::string selected_side_path = current_dir; // For side panel selection sync
    std::string selected_item_name; // For main panel selection
    bool selected_item_is_dir = false;
    std::string selected_item_fullpath;

    std::vector<std::string> dir_stack; // For going up

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
                if (ImGui::MenuItem("Preferences")) { show_preferences = true; }
                if (ImGui::MenuItem("Themes")) { /* handle Themes */ }
                ImGui::EndMenu();
            }
            // Add Exit button to the extreme right with custom colors
            float button_width = 60.0f;
            float margin = 8.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - button_width - margin);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.5f, 0.5f, 1.0f));         // light red
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));  // red
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f); // No rounding for Exit button
            if (ImGui::Button("Exit", ImVec2(button_width, 0))) {
                show_window = false;
            }
            ImGui::PopStyleVar();
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
            std::string drive_str = drive;
            bool drive_selected = (current_dir == drive_str);
            ImGuiTreeNodeFlags drive_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
            if (drive_selected) drive_flags |= ImGuiTreeNodeFlags_Selected;
            bool drive_open = ImGui::TreeNodeEx(drive, drive_flags);
            // Click to select drive
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                current_dir = drive_str;
            }
            if (drive_open) {
                // Enumerate folders and files in this drive
                char search_path[MAX_PATH];
                wsprintfA(search_path, "%s*", drive);
                WIN32_FIND_DATAA find_data;
                HANDLE hFind = FindFirstFileA(search_path, &find_data);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                            continue;
                        bool is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                        std::string folder_path = drive_str + std::string(find_data.cFileName) + "\\";
                        std::string label = std::string(is_dir ? "[+] " : "[-] ") + find_data.cFileName;
                        if (is_dir) {
                            ImGuiTreeNodeFlags folder_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
                            if (current_dir == folder_path) folder_flags |= ImGuiTreeNodeFlags_Selected;
                            bool folder_open = ImGui::TreeNodeEx(label.c_str(), folder_flags);
                            // Click to select folder
                            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                                current_dir = folder_path;
                            }
                            if (folder_open) {
                                // Enumerate subfolders and files (one level deep)
                                char subfolder_path[MAX_PATH];
                                wsprintfA(subfolder_path, "%s%s\\", drive, find_data.cFileName);
                                char sub_search[MAX_PATH];
                                wsprintfA(sub_search, "%s*", subfolder_path);
                                WIN32_FIND_DATAA sub_find;
                                HANDLE hSubFind = FindFirstFileA(sub_search, &sub_find);
                                if (hSubFind != INVALID_HANDLE_VALUE) {
                                    do {
                                        if (strcmp(sub_find.cFileName, ".") == 0 || strcmp(sub_find.cFileName, "..") == 0)
                                            continue;
                                        bool sub_is_dir = (sub_find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                                        std::string sub_label = std::string(sub_is_dir ? "[+] " : "[-] ") + sub_find.cFileName;
                                        std::string sub_path = folder_path + sub_find.cFileName + "\\";
                                        ImGuiTreeNodeFlags sub_flags = 0;
                                        if (current_dir == sub_path) sub_flags |= ImGuiTreeNodeFlags_Selected;
                                        if (ImGui::Selectable(sub_label.c_str(), current_dir == sub_path, sub_flags)) {
                                            if (sub_is_dir) {
                                                current_dir = sub_path;
                                            }
                                        }
                                    } while (FindNextFileA(hSubFind, &sub_find));
                                    FindClose(hSubFind);
                                }
                                ImGui::TreePop();
                            }
                        } else {
                            ImGui::BulletText("%s", label.c_str());
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
        // Always show Back button, only navigate if not at root
        if (ImGui::Button("Back")) {
            if (current_dir != "C:\\") {
                size_t pos = current_dir.find_last_of("\\/", current_dir.length() - 2);
                if (pos != std::string::npos) {
                    current_dir = current_dir.substr(0, pos + 1);
                }
            }
        }
        ImGui::SameLine();
        ImGui::Text("Current Directory: %s", current_dir.c_str());
        ImGui::Separator();

        // List folders and files with type column
        int folder_count = 0;
        int file_count = 0;
        static std::string selected_item_name;
        static bool selected_item_is_dir = false;
        static std::string selected_item_fullpath;
        static ULONGLONG selected_file_size = 0;
        static int selected_folder_subfolders = 0; // immediate subfolders (non-recursive)
        static int selected_folder_files = 0;      // immediate files (non-recursive)
        static ULONGLONG selected_folder_size = 0;
        ImGui::Columns(2, nullptr, true);
        ImGui::Text("Name"); ImGui::NextColumn();
        ImGui::Text("Type"); ImGui::NextColumn();
        ImGui::Separator();

        char search_path[MAX_PATH];
        wsprintfA(search_path, "%s*", current_dir.c_str());
        WIN32_FIND_DATAA find_data;
        HANDLE hFind = FindFirstFileA(search_path, &find_data);
        int item_index = 0;
        int selected_index = -1;
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                    continue;
                bool is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if (is_dir) folder_count++; else file_count++;
                std::string item_name = find_data.cFileName;
                std::string label = std::string(is_dir ? "[+] " : "[-] ") + item_name;
                std::string full_path = current_dir + item_name + (is_dir ? "\\" : "");
                bool is_selected = (selected_item_fullpath == full_path);
                if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
                    // Single click: select item
                    selected_item_name = item_name;
                    selected_item_is_dir = is_dir;
                    selected_item_fullpath = full_path;
                    selected_file_size = 0;
                    selected_folder_subfolders = 0;
                    selected_folder_files = 0;
                    selected_folder_size = 0;
                    if (!is_dir) {
                        // Get file size
                        HANDLE hFile = CreateFileA(full_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            LARGE_INTEGER size;
                            if (GetFileSizeEx(hFile, &size)) {
                                selected_file_size = size.QuadPart;
                            }
                            CloseHandle(hFile);
                        }
                // Cancel any running folder stats thread
                if (folder_stats_running) {
                    folder_stats_cancel = true;
                    if (folder_stats_thread.joinable()) folder_stats_thread.join();
                    folder_stats_running = false;
                }
                    } else {
                        // Cancel any running folder stats thread
                        if (folder_stats_running) {
                            folder_stats_cancel = true;
                            if (folder_stats_thread.joinable()) folder_stats_thread.join();
                            folder_stats_running = false;
                        }
                        // Count immediate subfolders and files (non-recursive)
                        char sub_search[MAX_PATH];
                        wsprintfA(sub_search, "%s*", full_path.c_str());
                        WIN32_FIND_DATAA sub_find;
                        HANDLE hSubFind = FindFirstFileA(sub_search, &sub_find);
                        if (hSubFind != INVALID_HANDLE_VALUE) {
                            do {
                                if (strcmp(sub_find.cFileName, ".") == 0 || strcmp(sub_find.cFileName, "..") == 0)
                                    continue;
                                if (sub_find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                                    selected_folder_subfolders++;
                                else
                                    selected_folder_files++;
                            } while (FindNextFileA(hSubFind, &sub_find));
                            FindClose(hSubFind);
                        }
                        // Start async folder stats computation (recursive size and file count)
                        folder_stats_path = full_path;
                        folder_stats_result = FolderStats();
                        folder_stats_cancel = false;
                        folder_stats_running = true;
                        folder_stats_thread = std::thread([full_path]() {
                            FolderStats stats;
                            GetFolderStatsRecursive(full_path, stats, &folder_stats_cancel);
                            if (!folder_stats_cancel) {
                                folder_stats_result = stats;
                            }
                            folder_stats_running = false;
                        });
                        if (folder_stats_thread.joinable()) folder_stats_thread.detach();
                    }
                }
                // Double click: open folder
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && is_dir) {
                    current_dir = full_path;
                    selected_item_fullpath.clear();
                    // Cancel any running folder stats thread
                    if (folder_stats_running) {
                        folder_stats_cancel = true;
                        if (folder_stats_thread.joinable()) folder_stats_thread.join();
                        folder_stats_running = false;
                    }
                }
                ImGui::NextColumn();
                if (is_dir) {
                    ImGui::TextUnformatted("folder");
                } else {
                    const char* ext = strrchr(item_name.c_str(), '.');
                    if (ext && ext != item_name.c_str()) {
                        ImGui::TextUnformatted(ext + 1);
                    } else {
                        ImGui::TextUnformatted("file");
                    }
                }
                ImGui::NextColumn();
                item_index++;
            } while (FindNextFileA(hFind, &find_data));
            FindClose(hFind);
        }
        ImGui::Columns(1);
        ImGui::End();

        // Draw fixed status bar at the bottom
        ImGui::SetNextWindowPos(statusbar_pos);
        ImGui::SetNextWindowSize(statusbar_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGuiWindowFlags statusbar_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##statusbar", nullptr, statusbar_flags);
        if (!selected_item_fullpath.empty()) {
            if (selected_item_is_dir) {
                if (folder_stats_running && folder_stats_path == selected_item_fullpath) {
                    ImGui::Text("Folder: %s | Subfolders: %d | Files: %d | Size: Calculating...", selected_item_name.c_str(), selected_folder_subfolders, selected_folder_files);
                } else if (folder_stats_path == selected_item_fullpath) {
                    // Format folder size dynamically
                    char size_str[64];
                    double size = (double)folder_stats_result.size;
                    if (size < 1024) {
                        snprintf(size_str, sizeof(size_str), "%llu bytes", folder_stats_result.size);
                    } else if (size < 1024 * 1024) {
                        snprintf(size_str, sizeof(size_str), "%.2f KB (%llu bytes)", size / 1024.0, folder_stats_result.size);
                    } else if (size < 1024 * 1024 * 1024) {
                        snprintf(size_str, sizeof(size_str), "%.2f MB (%llu bytes)", size / (1024.0 * 1024.0), folder_stats_result.size);
                    } else {
                        snprintf(size_str, sizeof(size_str), "%.2f GB (%llu bytes)", size / (1024.0 * 1024.0 * 1024.0), folder_stats_result.size);
                    }
                    ImGui::Text("Folder: %s | Subfolders: %d | Files: %d | Size: %s", selected_item_name.c_str(), selected_folder_subfolders, selected_folder_files, size_str);
                } else {
                    ImGui::Text("Folder: %s | Subfolders: %d | Files: %d", selected_item_name.c_str(), selected_folder_subfolders, selected_folder_files);
                }
            } else {
                // Format file size dynamically
                char size_str[64];
                double size = (double)selected_file_size;
                if (size < 1024) {
                    snprintf(size_str, sizeof(size_str), "%llu bytes", selected_file_size);
                } else if (size < 1024 * 1024) {
                    snprintf(size_str, sizeof(size_str), "%.2f KB (%llu bytes)", size / 1024.0, selected_file_size);
                } else if (size < 1024 * 1024 * 1024) {
                    snprintf(size_str, sizeof(size_str), "%.2f MB (%llu bytes)", size / (1024.0 * 1024.0), selected_file_size);
                } else {
                    snprintf(size_str, sizeof(size_str), "%.2f GB (%llu bytes)", size / (1024.0 * 1024.0 * 1024.0), selected_file_size);
                }
                ImGui::Text("File: %s | Type: %s | Size: %s", selected_item_name.c_str(),
                    (strrchr(selected_item_name.c_str(), '.') ? strrchr(selected_item_name.c_str(), '.') + 1 : "file"),
                    size_str);
            }
        } else {
            ImGui::Text("Folders: %d | Files: %d", folder_count, file_count);
        }
        ImGui::End();

        // Preferences window
        if (show_preferences) {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Preferences", &show_preferences, ImGuiWindowFlags_NoCollapse)) {
                ImGui::Text("Preferences");
                ImGui::Separator();
                ImGui::Text("(Add your preferences UI here)");
            }
            ImGui::End();
        }

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

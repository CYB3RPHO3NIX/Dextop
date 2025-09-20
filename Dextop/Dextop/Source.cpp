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
#include <unordered_map>
#include <mutex>
#include <map>
#include "json_utils.hpp"
#include <commdlg.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>

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

// --- File type mapping function ---
std::string GetFileTypeName(const std::string& ext) {
    static const std::map<std::string, std::string> type_map = {
        {"pdf", "Portable Document Format"},
        {"swf", "Shockwave Flash Format"},
        {"txt", "Text Document"},
        {"doc", "Microsoft Word Document"},
        {"docx", "Microsoft Word Document"},
        {"xls", "Microsoft Excel Spreadsheet"},
        {"xlsx", "Microsoft Excel Spreadsheet"},
        {"ppt", "Microsoft PowerPoint Presentation"},
        {"pptx", "Microsoft PowerPoint Presentation"},
        {"jpg", "JPEG Image"},
        {"jpeg", "JPEG Image"},
        {"png", "Portable Network Graphics"},
        {"gif", "Graphics Interchange Format"},
        {"bmp", "Bitmap Image"},
        {"tiff", "Tagged Image File Format"},
        {"svg", "Scalable Vector Graphics"},
        {"mp3", "MP3 Audio"},
        {"wav", "Waveform Audio"},
        {"ogg", "Ogg Vorbis Audio"},
        {"flac", "FLAC Audio"},
        {"mp4", "MPEG-4 Video"},
        {"avi", "AVI Video"},
        {"mov", "QuickTime Movie"},
        {"wmv", "Windows Media Video"},
        {"mkv", "Matroska Video"},
        {"zip", "ZIP Archive"},
        {"rar", "RAR Archive"},
        {"7z", "7-Zip Archive"},
        {"tar", "TAR Archive"},
        {"gz", "GZIP Archive"},
        {"exe", "Windows Executable"},
        {"dll", "Dynamic Link Library"},
        {"bat", "Batch File"},
        {"cmd", "Command Script"},
        {"cpp", "C++ Source File"},
        {"h", "C/C++ Header File"},
        {"c", "C Source File"},
        {"js", "JavaScript File"},
        {"json", "JSON File"},
        {"xml", "XML File"},
        {"html", "HTML Document"},
        {"htm", "HTML Document"},
        {"css", "Cascading Style Sheet"},
        {"py", "Python Script"},
        {"java", "Java Source File"},
        {"php", "PHP Script"},
        {"rb", "Ruby Script"},
        {"go", "Go Source File"},
        {"sh", "Shell Script"},
        {"md", "Markdown Document"},
        {"ini", "Configuration File"},
        {"log", "Log File"},
        {"iso", "ISO Disk Image"},
        {"apk", "Android Package"},
        {"db", "Database File"},
        {"sqlite", "SQLite Database"},
        {"csv", "Comma-Separated Values"},
        {"tsv", "Tab-Separated Values"},
        {"yml", "YAML File"},
        {"yaml", "YAML File"},
        {"psd", "Photoshop Document"},
        {"ai", "Adobe Illustrator File"},
        {"eps", "Encapsulated PostScript"},
        {"rtf", "Rich Text Format"},
        {"odt", "OpenDocument Text"},
        {"ods", "OpenDocument Spreadsheet"},
        {"odp", "OpenDocument Presentation"},
        {"apk", "Android Package"},
        {"bat", "Batch File"},
        {"com", "DOS Command File"},
        {"msi", "Windows Installer Package"},
        {"sys", "System File"},
        {"tmp", "Temporary File"},
        {"bak", "Backup File"},
        {"torrent", "BitTorrent File"},
        {"eml", "Email Message"},
        {"msg", "Outlook Mail Message"},
        {"ics", "iCalendar File"},
        {"vcf", "vCard File"},
        {"dat", "Data File"},
        {"bin", "Binary File"},
        {"apk", "Android Package"},
        {"app", "Application Bundle"},
        {"dmg", "Apple Disk Image"},
        {"pkg", "Package File"},
        {"deb", "Debian Package"},
        {"rpm", "Red Hat Package"},
        {"vbs", "VBScript File"},
        {"wsf", "Windows Script File"},
        {"asp", "Active Server Page"},
        {"aspx", "Active Server Page Extended"},
        {"jsp", "Java Server Page"},
        {"cfm", "ColdFusion Markup"},
        {"pl", "Perl Script"},
        {"lua", "Lua Script"},
        {"swift", "Swift Source File"},
        {"kt", "Kotlin Source File"},
        {"dart", "Dart Source File"},
        {"scala", "Scala Source File"},
        {"rs", "Rust Source File"},
        {"m", "Objective-C Source File"},
        {"mm", "Objective-C++ Source File"},
        {"vb", "Visual Basic File"},
        {"fs", "F# Source File"},
        {"cs", "C# Source File"},
        {"sln", "Visual Studio Solution"},
        {"vcxproj", "Visual C++ Project"},
        {"xcodeproj", "Xcode Project"},
        {"pro", "Qt Project File"},
        {"cmake", "CMake File"},
        {"makefile", "Makefile"},
        {"gradle", "Gradle Build File"},
        {"pom", "Maven Project Object Model"},
        {"lock", "Lock File"},
        {"manifest", "Manifest File"},
        {"resx", ".NET Resource File"},
        {"dll", "Dynamic Link Library"},
        {"lib", "Static Library"},
        {"obj", "Object File"},
        {"pdb", "Program Database"},
        {"suo", "Solution User Options"},
        {"user", "User Options File"},
        {"nupkg", "NuGet Package"},
        {"nuspec", "NuGet Specification"},
        {"vsix", "Visual Studio Extension"},
        {"xaml", "XAML File"},
        {"ps1", "PowerShell Script"},
        {"reg", "Registry File"},
        {"scr", "Screensaver File"},
        {"lnk", "Shortcut File"},
        {"url", "Internet Shortcut"},
        {"desktop", "Desktop Entry"},
        {"cfg", "Configuration File"},
        {"conf", "Configuration File"},
        {"properties", "Properties File"},
        {"env", "Environment File"},
        {"rc", "Run Commands File"},
        {"service", "Systemd Service File"},
        {"plist", "Property List"},
        {"dbf", "Database File"},
        {"mdb", "Microsoft Access Database"},
        {"accdb", "Microsoft Access Database"},
        {"sql", "SQL File"},
        {"bak", "Backup File"},
        {"tmp", "Temporary File"},
        {"swf", "Shockwave Flash Format"}
    };
    std::string lower_ext = ext;
    for (auto& c : lower_ext) c = tolower(c);
    auto it = type_map.find(lower_ext);
    if (it != type_map.end()) return it->second;
    return ext;
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
    std::string current_dir = ""; // Start with no selection so UI shows all drives
    std::string selected_side_path = current_dir; // For side panel selection sync
    std::string selected_item_name; // For main panel selection
    bool selected_item_is_dir = false;
    std::string selected_item_fullpath;

    std::vector<std::string> dir_stack; // For going up

    // Add a global variable for the preference
    static bool pref_show_item_checkboxes = false;
    static bool pref_show_side_panel = true;
    static std::vector<std::string> checked_items; // store full paths
    // Buffers and state for New->File/Folder/Shortcut dialogs
    static char new_file_name[260] = "";
    static char new_folder_name[260] = "";
    static char new_shortcut_name[260] = "";
    static char new_shortcut_target[MAX_PATH] = "";
    static bool open_new_file_popup = false;
    static bool open_new_folder_popup = false;
    static bool open_new_shortcut_popup = false;
    // Custom selector state for choosing target path (replaces Windows dialogs)
    static bool open_select_target_popup = false;
    static bool select_target_mode_file = true; // true==file, false==folder
    static std::string select_target_current = ""; // current folder shown in selector
    static std::string select_target_selected = ""; // selected item (file or folder)
    static char select_preview[MAX_PATH] = "";
     // Persisted selected section for preferences (moved out so we can load/save it)
     static int selected_section = 0;

    // Load preferences from config.json if available
    {
        nlohmann::json cfg;
        // Default preferences
        nlohmann::json default_cfg = {
            {"show_item_checkboxes", false},
            {"selected_section", 0},
            {"show_side_panel", true}
        };

        bool loaded = read_json_file("config.json", cfg);
        if (!loaded) {
            // If reading failed (missing or invalid), create a new config file with defaults
            cfg = default_cfg;
            write_json_file("config.json", cfg);
        }

        // Merge loaded config with defaults to ensure all keys exist
        // If a key is missing or of wrong type, set it to default and mark to persist
        bool need_persist = false;
        if (cfg.contains("show_item_checkboxes") && cfg["show_item_checkboxes"].is_boolean()) {
            pref_show_item_checkboxes = cfg["show_item_checkboxes"].get<bool>();
        } else {
            cfg["show_item_checkboxes"] = default_cfg["show_item_checkboxes"];
            need_persist = true;
        }
        if (cfg.contains("selected_section") && cfg["selected_section"].is_number_integer()) {
            selected_section = cfg["selected_section"].get<int>();
        } else {
            cfg["selected_section"] = default_cfg["selected_section"];
            need_persist = true;
        }
        if (cfg.contains("show_side_panel") && cfg["show_side_panel"].is_boolean()) {
            pref_show_side_panel = cfg["show_side_panel"].get<bool>();
        } else {
            cfg["show_side_panel"] = default_cfg["show_side_panel"];
            need_persist = true;
        }

        if (need_persist) {
            write_json_file("config.json", cfg);
        }
    }

    // --- Folder size cache and async state for checkboxes ---
    static std::unordered_map<std::string, ULONGLONG> checked_folder_size_cache;
    static std::unordered_map<std::string, std::atomic<bool>> checked_folder_size_running;
    static std::mutex checked_folder_size_mutex;

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
        // Hide sidebar when preference disabled
        float effective_sidebar_width = pref_show_side_panel ? sidebar_width : 0.0f;
        ImVec2 sidebar_size = ImVec2(effective_sidebar_width, sidebar_height);
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
                if (ImGui::BeginMenu("New")) {
                    if (ImGui::MenuItem("File")) { open_new_file_popup = true; }
                    if (ImGui::MenuItem("Folder")) { open_new_folder_popup = true; }
                    if (ImGui::MenuItem("Shortcut")) { open_new_shortcut_popup = true; }
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Cut")) { /* handle Cut */ }
                if (ImGui::MenuItem("Copy")) { /* handle Copy */ }
                if (ImGui::MenuItem("Delete")) { /* handle Delete */ }
                ImGui::Separator();
                if (ImGui::MenuItem("Rename")) { /* handle Rename */ }
                if (ImGui::MenuItem("Share")) { /* handle Share */ }
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

        // Draw fixed Directory Tree window on the left (optional)
        if (pref_show_side_panel) {
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
                    checked_items.clear();
                    checked_folder_size_cache.clear();
                    checked_folder_size_running.clear();
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
                                    checked_items.clear();
                                    checked_folder_size_cache.clear();
                                    checked_folder_size_running.clear();
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
                                                    checked_items.clear();
                                                    checked_folder_size_cache.clear();
                                                    checked_folder_size_running.clear();
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
        }

        // Draw fixed central window
        ImGui::SetNextWindowPos(center_pos);
        ImGui::SetNextWindowSize(center_size);
        ImGuiWindowFlags center_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("Central Window", nullptr, center_flags);
        // Back button: when at drive root or empty, goes back to drive listing
        if (ImGui::Button("Back")) {
            if (current_dir.empty()) {
                // already at drives listing; nothing to do
            } else if (current_dir.size() == 3 && current_dir[1] == ':' && (current_dir[2] == '\\' || current_dir[2] == '/')) {
                // drive root -> go back to drives listing
                current_dir.clear();
                checked_items.clear();
                checked_folder_size_cache.clear();
                checked_folder_size_running.clear();
            } else {
                size_t pos = current_dir.find_last_of("\\/", current_dir.length() - 2);
                if (pos != std::string::npos) {
                    current_dir = current_dir.substr(0, pos + 1);
                    checked_items.clear();
                    checked_folder_size_cache.clear();
                    checked_folder_size_running.clear();
                }
            }
        }
        ImGui::SameLine();
        if (current_dir.empty()) {
            ImGui::Text("Drives");
        } else {
            ImGui::Text("Current Directory: %s", current_dir.c_str());
        }
        ImGui::Separator();

        // List folders and files with type column (or show drives when no current_dir)
        int folder_count = 0;
        int file_count = 0;
        static std::string selected_item_name;
        static bool selected_item_is_dir = false;
        static std::string selected_item_fullpath;
        static ULONGLONG selected_file_size = 0;
        static int selected_folder_subfolders = 0;
        static int selected_folder_files = 0;
        static ULONGLONG selected_folder_size = 0;
        static ULONGLONG checked_total_size = 0;
        int checked_count = 0;
        checked_total_size = 0;
        if (current_dir.empty()) {
            // Show drives listing in central panel when no current_dir
            ImGui::Columns(2, nullptr, true);
            ImGui::Text("Name"); ImGui::NextColumn();
            ImGui::Text("Type"); ImGui::NextColumn();
            ImGui::Separator();
            char drives[256];
            DWORD drive_count = GetLogicalDriveStringsA(sizeof(drives), drives);
            int drive_index = 0;
            for (char* drive = drives; *drive; drive += strlen(drive) + 1) {
                std::string drive_str = drive;
                if (ImGui::Selectable(drive_str.c_str(), false, ImGuiSelectableFlags_SpanAllColumns)) {
                    current_dir = drive_str;
                    checked_items.clear();
                    checked_folder_size_cache.clear();
                    checked_folder_size_running.clear();
                }
                ImGui::NextColumn();
                ImGui::TextUnformatted("drive");
                ImGui::NextColumn();
                drive_index++;
            }
            ImGui::Columns(1);
        } else {
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
                    // --- Checkbox logic ---
                    bool checked = false;
                    if (pref_show_item_checkboxes) {
                        auto it = std::find(checked_items.begin(), checked_items.end(), full_path);
                        checked = (it != checked_items.end());
                        ImGui::PushID(item_index);
                        if (ImGui::Checkbox("", &checked)) {
                            if (checked) {
                                checked_items.push_back(full_path);
                                // If it's a folder and not already being calculated, start async size calc
                                if (is_dir && checked_folder_size_cache.find(full_path) == checked_folder_size_cache.end() && checked_folder_size_running.find(full_path) == checked_folder_size_running.end()) {
                                    checked_folder_size_running[full_path] = true;
                                    std::thread([full_path]() {
                                        FolderStats stats;
                                        GetFolderStatsRecursive(full_path, stats);
                                        std::lock_guard<std::mutex> lock(checked_folder_size_mutex);
                                        checked_folder_size_cache[full_path] = stats.size;
                                        checked_folder_size_running[full_path] = false;
                                    }).detach();
                                }
                            } else {
                                checked_items.erase(std::remove(checked_items.begin(), checked_items.end(), full_path), checked_items.end());
                                // Remove from cache and running
                                std::lock_guard<std::mutex> lock(checked_folder_size_mutex);
                                checked_folder_size_cache.erase(full_path);
                                checked_folder_size_running.erase(full_path);
                            }
                        }
                        ImGui::PopID();
                        ImGui::SameLine();
                    }
                    // --- End Checkbox logic ---
                    // --- Selectable with double-click folder open ---
                    if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_SpanAllColumns)) {
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
                    // --- Double-click to open folder ---
                    if (is_dir && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                        current_dir = full_path;
                        selected_item_fullpath.clear();
                        checked_items.clear();
                        checked_folder_size_cache.clear();
                        checked_folder_size_running.clear();
                        // Cancel any running folder stats thread
                        if (folder_stats_running) {
                            folder_stats_cancel = true;
                            if (folder_stats_thread.joinable()) folder_stats_thread.join();
                            folder_stats_running = false;
                        }
                    }
                    // --- End double-click logic ---
                    // --- Calculate checked size/count ---
                    if (pref_show_item_checkboxes && checked) {
                        checked_count++;
                        if (is_dir) {
                            std::lock_guard<std::mutex> lock(checked_folder_size_mutex);
                            auto it = checked_folder_size_cache.find(full_path);
                            if (it != checked_folder_size_cache.end()) {
                                checked_total_size += it->second;
                            }
                        } else {
                            HANDLE hFile = CreateFileA(full_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                            if (hFile != INVALID_HANDLE_VALUE) {
                                LARGE_INTEGER size;
                                if (GetFileSizeEx(hFile, &size)) {
                                    checked_total_size += size.QuadPart;
                                }
                                CloseHandle(hFile);
                            }
                        }
                    }
                    // --- End checked size/count ---
                    ImGui::NextColumn();
                    if (is_dir) {
                        ImGui::TextUnformatted("folder");
                    } else {
                        const char* ext = strrchr(item_name.c_str(), '.');
                        if (ext && ext != item_name.c_str()) {
                            std::string ext_str = ext + 1;
                            ImGui::TextUnformatted(GetFileTypeName(ext_str).c_str());
                        } else {
                            ImGui::TextUnformatted("file");
                        }
                    }
                    ImGui::NextColumn();
                    item_index++;
                } while (FindNextFileA(hFind, &find_data));
                FindClose(hFind);
            }
        }
        ImGui::Columns(1);
        // --- Show checked summary ---
        // (Removed: do not show selected item count and size here)
        // --- End checked summary ---
        ImGui::End();

        // Draw fixed status bar at the bottom
        ImGui::SetNextWindowPos(statusbar_pos);
        ImGui::SetNextWindowSize(statusbar_size);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGuiWindowFlags statusbar_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("##statusbar", nullptr, statusbar_flags);
        if (pref_show_item_checkboxes && checked_count > 0) {
            char checked_info[256];
            double size = (double)checked_total_size;
            int calculating = 0;
            for (const auto& path : checked_items) {
                if (checked_folder_size_running.count(path) && checked_folder_size_running[path]) calculating++;
            }
            if (size < 1024) {
                snprintf(checked_info, sizeof(checked_info), "Selected: %d | Size: %llu bytes%s", checked_count, checked_total_size, calculating ? " (Calculating...)" : "");
            } else if (size < 1024 * 1024) {
                snprintf(checked_info, sizeof(checked_info), "Selected: %d | Size: %.2f KB%s", checked_count, size / 1024.0, calculating ? " (Calculating...)" : "");
            } else if (size < 1024 * 1024 * 1024) {
                snprintf(checked_info, sizeof(checked_info), "Selected: %d | Size: %.2f MB%s", checked_count, size / (1024.0 * 1024.0), calculating ? " (Calculating...)" : "");
            } else {
                snprintf(checked_info, sizeof(checked_info), "Selected: %d | Size: %.2f GB%s", checked_count, size / (1024.0 * 1024.0 * 1024.0), calculating ? " (Calculating...)" : "");
            }
            ImGui::Text("%s", checked_info);
        } else if (!selected_item_fullpath.empty()) {
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
            ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Preferences", &show_preferences, ImGuiWindowFlags_NoCollapse)) {
                static const char* sections[] = { "Show", "General", "Appearance", "Shortcuts" };
                ImGui::BeginChild("##prefs_sidebar", ImVec2(150, 0), true, ImGuiWindowFlags_NoMove);
                for (int i = 0; i < IM_ARRAYSIZE(sections); ++i) {
                    if (ImGui::Selectable(sections[i], selected_section == i)) {
                        selected_section = i;
                    }
                }
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::BeginChild("##prefs_mainpanel", ImVec2(0, -50), false, ImGuiWindowFlags_NoMove);
                if (selected_section == 0) {
                    ImGui::Text("Show Settings");
                    ImGui::Separator();
                    ImGui::Checkbox("Item Checkboxes", &pref_show_item_checkboxes);
                    ImGui::Checkbox("Side Navigation Panel", &pref_show_side_panel);
                } else if (selected_section == 1) {
                    ImGui::Text("General Settings");
                    ImGui::Separator();
                    ImGui::Text("(Add general settings here)");
                } else if (selected_section == 2) {
                    ImGui::Text("Appearance Settings");
                    ImGui::Separator();
                    ImGui::Text("(Add appearance settings here)");
                } else if (selected_section == 3) {
                    ImGui::Text("Shortcuts");
                    ImGui::Separator();
                    ImGui::Text("(Add shortcut settings here)");
                }
                ImGui::EndChild();
                // --- BUTTONS AT BOTTOM RIGHT ---
                float button_width = 80.0f;
                float spacing = 10.0f;
                ImVec2 button_size(button_width, 0);
                float window_w = ImGui::GetWindowWidth();
                float button_area_w = button_width * 2 + spacing;
                ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 40);
                ImGui::SetCursorPosX(window_w - button_area_w - ImGui::GetStyle().WindowPadding.x);
                if (ImGui::Button("OK", button_size)) {
                    ImGui::SaveIniSettingsToDisk("imgui.ini");
                    // Save preferences to config.json
                    nlohmann::json cfg;
                    cfg["show_item_checkboxes"] = pref_show_item_checkboxes;
                    cfg["show_side_panel"] = pref_show_side_panel;
                    cfg["selected_section"] = selected_section;
                    // write to disk (returns true on success)
                    if (!write_json_file("config.json", cfg)) {
                        // Could log an error or show notification - omitted for brevity
                    }
                    show_preferences = false;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("Cancel", button_size)) {
                    show_preferences = false;
                }
                ImGui::PopStyleColor(2);
                // --- END BUTTONS ---
            }
            ImGui::End();
        }

        // Ensure popups are opened when requested
        if (open_new_file_popup) { ImGui::OpenPopup("New File"); open_new_file_popup = false; }
        if (open_new_folder_popup) { ImGui::OpenPopup("New Folder"); open_new_folder_popup = false; }
        if (open_new_shortcut_popup) { ImGui::OpenPopup("New Shortcut"); open_new_shortcut_popup = false; }

        // New File modal
        if (ImGui::BeginPopupModal("New File", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("File name (with extension)", new_file_name, sizeof(new_file_name));
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120,0))) {
                // Determine target directory
                std::string target_dir = current_dir;
                if (target_dir.empty()) {
                    // fallback to first drive
                    char drives[256];
                    if (GetLogicalDriveStringsA(sizeof(drives), drives) > 0) {
                        target_dir = std::string(drives);
                    }
                }
                if (!target_dir.empty() && new_file_name[0] != '\0') {
                    std::string path = target_dir + new_file_name;
                    HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
                }
                // reset and close
                new_file_name[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120,0))) {
                new_file_name[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // New Folder modal
        if (ImGui::BeginPopupModal("New Folder", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Folder name", new_folder_name, sizeof(new_folder_name));
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120,0))) {
                std::string target_dir = current_dir;
                if (target_dir.empty()) {
                    char drives[256];
                    if (GetLogicalDriveStringsA(sizeof(drives), drives) > 0) {
                        target_dir = std::string(drives);
                    }
                }
                if (!target_dir.empty() && new_folder_name[0] != '\0') {
                    std::string path = target_dir + new_folder_name;
                    // Ensure trailing backslash not duplicated
                    CreateDirectoryA(path.c_str(), NULL);
                }
                new_folder_name[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120,0))) {
                new_folder_name[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // New Shortcut modal
        if (ImGui::BeginPopupModal("New Shortcut", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Shortcut name", new_shortcut_name, sizeof(new_shortcut_name));
            ImGui::InputText("Target path", new_shortcut_target, sizeof(new_shortcut_target));
            // Ensure the select popup is opened if requested
            if (open_select_target_popup) {
                ImGui::OpenPopup("Select Target");
                open_select_target_popup = false;
                // initialize selection preview
                select_target_selected.clear();
                select_preview[0] = '\0';
            }
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120,0))) {
                std::string target_dir = current_dir;
                if (target_dir.empty()) {
                    char drives[256];
                    if (GetLogicalDriveStringsA(sizeof(drives), drives) > 0) {
                        target_dir = std::string(drives);
                    }
                }
                if (!target_dir.empty() && new_shortcut_name[0] != '\0' && new_shortcut_target[0] != '\0') {
                    std::string shortcut_path = target_dir + std::string(new_shortcut_name) + ".lnk";
                    // Create .lnk via COM
                    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
                    if (SUCCEEDED(hr)) {
                        IShellLinkW* psl = NULL;
                        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&psl);
                        if (SUCCEEDED(hr) && psl) {
                            // set the target
                            WCHAR wtarget[MAX_PATH];
                            MultiByteToWideChar(CP_UTF8, 0, new_shortcut_target, -1, wtarget, MAX_PATH);
                            psl->SetPath(wtarget);
                            // persist to disk
                            IPersistFile* ppf = NULL;
                            hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
                            if (SUCCEEDED(hr) && ppf) {
                                WCHAR wpath[MAX_PATH];
                                MultiByteToWideChar(CP_UTF8, 0, shortcut_path.c_str(), -1, wpath, MAX_PATH);
                                ppf->Save(wpath, TRUE);
                                ppf->Release();
                            }
                            psl->Release();
                        }
                        CoUninitialize();
                    }
                }
                new_shortcut_name[0] = '\0';
                new_shortcut_target[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120,0))) {
                new_shortcut_name[0] = '\0';
                new_shortcut_target[0] = '\0';
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        // Custom Select Target modal (drive/folder/file browser)
        if (ImGui::BeginPopupModal("Select Target", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text(select_target_mode_file ? "Select File" : "Select Folder");
            ImGui::Separator();
            // Breadcrumb / current path
            ImGui::TextWrapped("Current: %s", select_target_current.empty() ? "(Drives)" : select_target_current.c_str());

            ImGui::BeginChild("##select_target_list", ImVec2(500,300), true);
            if (select_target_current.empty()) {
                // show drives
                char drives[256];
                DWORD ret = GetLogicalDriveStringsA(sizeof(drives), drives);
                if (ret > 0) {
                    for (char* d = drives; *d; d += strlen(d) + 1) {
                        std::string drive_str = d;
                        if (ImGui::Selectable(drive_str.c_str(), select_target_selected == drive_str)) {
                            select_target_current = drive_str;
                            select_target_selected = "";
                        }
                    }
                }
            } else {
                // list parent directory '..'
                if (ImGui::Selectable("..", false)) {
                    if (select_target_current.size() > 3) {
                        size_t pos = select_target_current.find_last_of("\\/", select_target_current.length() - 2);
                        if (pos != std::string::npos) select_target_current = select_target_current.substr(0, pos + 1);
                        else select_target_current.clear();
                        select_target_selected.clear();
                    } else {
                        // at drive root -> go to drives
                        select_target_current.clear();
                        select_target_selected.clear();
                    }
                }
                // enumerate entries
                char search[MAX_PATH];
                wsprintfA(search, "%s*", select_target_current.c_str());
                WIN32_FIND_DATAA fd;
                HANDLE h = FindFirstFileA(search, &fd);
                if (h != INVALID_HANDLE_VALUE) {
                    do {
                        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) continue;
                        bool is_dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                        std::string name = fd.cFileName;
                        std::string label = is_dir ? ("[D] " + name) : name;
                        std::string full = select_target_current + name + (is_dir ? "\\" : "");
                        if (ImGui::Selectable(label.c_str(), select_target_selected == full)) {
                            select_target_selected = full;
                            strncpy_s(select_preview, sizeof(select_preview), full.c_str(), _TRUNCATE);
                        }
                        if (is_dir && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                            select_target_current = full;
                            select_target_selected.clear();
                            select_preview[0] = '\0';
                        }
                    } while (FindNextFileA(h, &fd));
                    FindClose(h);
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            ImGui::InputText("Selected", select_preview, sizeof(select_preview));
            ImGui::Separator();
            if (ImGui::Button("OK", ImVec2(120,0))) {
                if (select_target_mode_file) {
                    if (select_target_selected.empty()) {
                        // if no selection but current is a file path? ignore
                    } else {
                        // ensure selected is a file (not ending with backslash)
                        std::string sel = select_target_selected;
                        if (!sel.empty() && sel.back() != '\\') {
                            strncpy_s(new_shortcut_target, sizeof(new_shortcut_target), sel.c_str(), _TRUNCATE);
                        }
                    }
                } else {
                    // folder mode: if selected is a folder or current is folder
                    std::string sel = select_target_selected.empty() ? select_target_current : select_target_selected;
                    if (!sel.empty()) {
                        // strip trailing backslash for consistency
                        if (sel.back() == '\\') sel.pop_back();
                        strncpy_s(new_shortcut_target, sizeof(new_shortcut_target), sel.c_str(), _TRUNCATE);
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120,0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
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

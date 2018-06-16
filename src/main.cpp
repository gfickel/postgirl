#include <thread>
#include <atomic>
#include <stdio.h>
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include "GL/gl3w.h"    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include "pgstring.h"
#include "pgvector.h"
#include "rapidjson/document.h"
#include "dirent_portable.h"
#include "requests.h"




void processRequest(std::thread& thread, const char* buf, 
                    pg::Vector<History>& history, const pg::Vector<Argument>& args, 
                    const pg::Vector<Argument>& headers, int request_type, 
                    const pg::String& contentType, const pg::String& inputJson,
                    std::atomic<ThreadStatus>& thread_status)
{
    if (thread_status == RUNNING)
        return;

    History hist;
    hist.url = pg::String(buf);
    hist.args = args;
    hist.headers = headers;
    hist.result = pg::String("Processing");
    pg::String url(buf);
    history.push_back(hist);

    
    thread_status = RUNNING;

    switch(request_type) { 
        case GET:
            thread = std::thread(threadRequestGet, std::ref(thread_status), url, args, headers, contentType, std::ref(history.back().result), std::ref(hist.response_code));
            break;
        case POST:
            thread = std::thread(threadRequestPost, std::ref(thread_status), url, args, headers, contentType, inputJson, std::ref(history.back().result), std::ref(hist.response_code));
            break;
        default:
            hist.result = pg::String("Invalid request type selected!");
            thread_status = FINISHED;
    }

}

int compareSize(const void* A, const void* B)
{
    return strcmp(((pg::String*)A)->buf_, ((pg::String*)B)->buf_);
}


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int main(int argc, char* argv[])
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Postgirl", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    gl3wInit();

    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui_ImplGlfwGL3_Init(window, true);
    ImGui::StyleColorsLight();

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::atomic<ThreadStatus> thread_status(IDLE); // syncronize thread to call join only when it finishes
    std::thread thread;

    // Such ugly code... Can I do something like this?
    // char** arg_types[] = { {"Text", "File"}, "Text" }; 
    pg::Vector<const char**> arg_types;
    const char* post_types[] = {"Text", "File"};
    const char* get_types[] = {"Text"};
    arg_types.push_back(get_types);
    arg_types.push_back(post_types);
    pg::Vector<int> num_arg_types;
    num_arg_types.push_back(1);
    num_arg_types.push_back(2);

    bool picking_file = false;
    bool show_history = true;
    int curr_arg_file = 0;
    
    pg::String input_json(1024*32); // 32KB static string should be reasonable

    pg::Vector<History> history;
    curl_global_init(CURL_GLOBAL_ALL);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        static pg::Vector<Argument> args;

        {
            ImGui::Begin("Postgirl");//, NULL, ImGuiWindowFlags_NoMove);

            static const char* items[] = {"GET", "POST"};
            static const char* ct_post[] = {"multipart/form-data", "application/json", "<NONE>"};
            static int request_type = 0;
            static int content_type = 0;
            static pg::String content_type_str;
            static pg::Vector<Argument> headers;
            static pg::String result;
           

            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.125);
            if (ImGui::BeginCombo("##request_type", items[request_type])) {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
                    if (ImGui::Selectable(items[n])) {
                        request_type = n;
                        content_type = 0;
                        content_type_str = "";
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();

            switch (request_type) {
                case 0: break;
                case 1:
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.25);
                    if (ImGui::BeginCombo("##content_type", ct_post[content_type])) {
                        for (int n = 0; n < IM_ARRAYSIZE(ct_post); n++) {
                            if (ImGui::Selectable(ct_post[n])) {
                                content_type = n;
                                content_type_str = ct_post[n];
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SameLine();
                    break;
            }
            
            static char buf[4098] = "http://localhost:5000/test_route";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            if (ImGui::InputText("URL", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue) ) {
                ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
                processRequest(thread, buf, history, args, headers, request_type, content_type_str, input_json, thread_status);
            }


            static pg::Vector<int> delete_arg_btn;
            for (int i=0; i<(int)headers.size(); i++) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                char arg_name[32];
                sprintf(arg_name, "Name##header arg name%d", i);
                if (ImGui::InputText(arg_name, &headers[i].name[0], headers[i].name.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, content_type_str, input_json, thread_status);
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.4);
                sprintf(arg_name, "Value##header arg value%d", i);
                if (ImGui::InputText(arg_name, &headers[i].value[0], headers[i].value.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, content_type_str, input_json, thread_status);
                ImGui::SameLine();
                char btn_name[32];
                sprintf(btn_name, "Delete##header arg delete%d", i);
                if (ImGui::Button(btn_name)) {
                    delete_arg_btn.push_back(i);
                }
            }
            
            // delete headers
            for (int i=(int)delete_arg_btn.size(); i>0; i--) {
                headers.erase(headers.begin()+delete_arg_btn[i-1]);
            }
            delete_arg_btn.clear();

            if (ImGui::Button("Add Header Arg")) {
                headers.push_back(Argument());
            }

            for (int i=0; i<(int)args.size(); i++) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                ImGui::Text("request_type: %d", request_type);
                ImGui::Combo("", &args[i].arg_type, arg_types[request_type], num_arg_types[request_type]);
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                char arg_name[32];
                sprintf(arg_name, "Name##arg name%d", i);
                if (ImGui::InputText(arg_name, &args[i].name[0], args[i].name.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, content_type_str, input_json, thread_status);
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.6);
                sprintf(arg_name, "Value##arg name%d", i);
                if (ImGui::InputText(arg_name, &args[i].value[0], args[i].value.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, content_type_str, input_json, thread_status);
                ImGui::SameLine();
                if (args[i].arg_type == 1) {
                    sprintf(arg_name, "File##arg name%d", i);
                    if (ImGui::Button(arg_name)) {
                        picking_file = true;
                        curr_arg_file = i;
                    }
                }
                ImGui::SameLine();
                char btn_name[32];
                sprintf(btn_name, "Delete##arg delete%d", i);
                if (ImGui::Button(btn_name)) {
                    if (curr_arg_file == i) {
                        curr_arg_file = -1;
                        picking_file = false;
                    }
                    delete_arg_btn.push_back(i);
                }
            }
            
            if (ImGui::Button("Add Argument")) {
                Argument arg;
                arg.arg_type = 0;
                args.push_back(arg);
            }
            ImGui::SameLine();
            
            // delete the args
            if (thread_status != RUNNING) {
                for (int i=(int)delete_arg_btn.size(); i>0; i--) {
                    args.erase(args.begin()+delete_arg_btn[i-1]);
                }
            }
            delete_arg_btn.clear();
            if (ImGui::Button("Delete all args") && thread_status != RUNNING) {
                args.clear();
            }

            if (thread_status == FINISHED) {
                thread.join();
                thread_status = IDLE;
            }
            
            if (request_type == 1 && content_type == 1) {
                ImGui::Text("Input JSON");
                rapidjson::Document d;
                if (d.Parse(input_json.buf_).HasParseError() && input_json.length() > 0) {
                    ImGui::SameLine();
                    ImGui::Text("Problems with JSON");
                }
                int block_height = ImGui::GetContentRegionAvail()[1];
                block_height /= 2;
                ImGui::InputTextMultiline("##input_json", &input_json[0], input_json.capacity(), ImVec2(-1.0f, block_height), ImGuiInputTextFlags_AllowTabInput);
            }

            ImGui::Text("Result");
            if (history.size() > 0)
                ImGui::InputTextMultiline("##source", &history.back().result[0], history.back().result.capacity(), ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
            else {
                char blank[] = "";
                ImGui::InputTextMultiline("##source", blank, 0, ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
            }


            ImGui::End();
        }
        if (picking_file) {
            ImGui::Begin("File Selector", &picking_file);
            static pg::Vector<pg::String> curr_files;
            static pg::Vector<pg::String> curr_folders;
            static pg::String curr_dir(".");
            if (curr_files.size() == 0 && curr_folders.size()==0) {
                DIR *dir;
                dirent *pdir;
                dir=opendir(curr_dir.buf_);
                while((pdir=readdir(dir))) {
                    if (pdir->d_type == DT_DIR) {
                        curr_folders.push_back(pdir->d_name);
                    } else if (pdir->d_type == DT_REG) {
                        curr_files.push_back(pdir->d_name);
                    }
                }
                pg::String aux_dir = curr_dir;
                if (curr_dir.capacity_ < 2048) curr_dir.realloc(2048);
                realpath(aux_dir.buf_, curr_dir.buf_);
                closedir(dir);

                qsort(curr_folders.begin(), curr_folders.size(), sizeof(*curr_folders.begin()), compareSize);
                qsort(curr_files.begin(), curr_files.size(), sizeof(*curr_files.begin()), compareSize);
            }

            
            // ImGui::Text(curr_dir.buf_);
            ImGui::Button(curr_dir.buf_);
            for (int i=0; i<curr_folders.size(); i++) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + ImGui::GetContentRegionAvail()[0], pos.y + ImGui::GetTextLineHeight()), IM_COL32(100,0,255,50));
                if (ImGui::MenuItem(curr_folders[i].buf_, NULL)) {
                    curr_dir.append("/");
                    curr_dir.append(curr_folders[i]);
                    curr_files.clear();
                    curr_folders.clear();
                }
            }
            for (int i=0; i<curr_files.size(); i++) {
                ImVec2 pos = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + ImGui::GetContentRegionAvail()[0], pos.y + ImGui::GetTextLineHeight()), IM_COL32(100,100,0,50));
                if (ImGui::MenuItem(curr_files[i].buf_, NULL)) {
                    if (curr_arg_file >= 0 && curr_arg_file < args.size()) {
                        pg::String filename = curr_dir;
                        filename.append("/");
                        filename.append(curr_files[i]);
                        args[curr_arg_file].value = filename;
                        printf("args[%d].value = %s\n", curr_arg_file, filename.buf_);
                    }                    

                    picking_file = false;
                    curr_arg_file = -1;
                }
            }
            ImGui::End();
        }
        if (show_history) {
            ImGui::Begin("History", &show_history);
            static int selected  = -1;
            for (int i=0; i<history.size(); i++) {
                char select_name[2048];
                sprintf(select_name, "(%s) %s##%d", RequestTypeToString(history[i].req_type).buf_, history[i].url.buf_, i);
                if (ImGui::Selectable(select_name, selected==i)) {
                    selected = i;
                }
            }
            ImGui::End();
        }
        
        
        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate(); 
    
    return 0;
}

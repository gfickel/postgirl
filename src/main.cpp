#include <thread>
#include <atomic>
#include <stdio.h>
#include "json/json.h"
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include "GL/gl3w.h"    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include "pgstring.h"
#include "pgvector.h"

typedef enum ThreadStatus {
    IDLE     = 0,
    RUNNING  = 1,
    FINISHED = 2
} ThreadStatus;

typedef struct Argument { 
    pg::String name;
    pg::String value;
    int arg_type; // TODO: transform this to an ENUM soon!!!!
} Argument; 


typedef struct History {
    pg::String url;
    pg::Vector<Argument> args;
    pg::Vector<pg::String> headers;
    pg::String result;
    int response_code;
} History;



// void removeHistory(pg::Vector<History>& history, int idx) {
//     for (int i=0; i<(int)history[idx].args.size(); i++) {
//         free(history[idx].args[i].value_ptr); 
//     }
//     history.erase(history.begin() + idx);
// }


// template <typename T>
// Argument createArgument(pg::String name, const T& val, int argType) {
//     Argument arg;
//     arg.name = name;
//     arg.value_ptr = (void*)malloc(sizeof(T));
//     arg.arg_type = argType;
// 
//     return arg;
// } 

typedef struct MemoryStruct {
    MemoryStruct() { memory = (char*)malloc(1); size=0; };
    
    char *memory;
    size_t size;
} MemoryStruct;
 
static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}


void threadRequestGet(std::atomic<ThreadStatus>& thread_status, pg::String url, 
                      pg::Vector<Argument> args, pg::Vector<Argument> headers, 
                      pg::String& thread_result, int& response_code) 
{ 
    
    CURLcode res;
    CURL* curl;
    curl = curl_easy_init();

    MemoryStruct chunk;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    
    if (args.size() > 0) url.append("?");
    for (int i=0; i<(int)args.size(); i++) {
        char* escaped_name = curl_easy_escape(curl , args[i].name.buf_, args[i].name.length());
        url.append(escaped_name);
        url.append("=");
        char* escaped_value = curl_easy_escape(curl , args[i].value.buf_, args[i].value.length());
        url.append(escaped_value);
        if (i < (int)args.size()-1) url.append("&");
    }

    struct curl_slist *header_chunk = NULL;
    for (int i=0; i<(int)headers.size(); i++) {
        char* escaped_name = curl_easy_escape(curl , headers[i].name.buf_, headers[i].name.length());
        char* escaped_value = curl_easy_escape(curl , headers[i].value.buf_, headers[i].value.length());
        pg::String header(escaped_name);
        if (headers[i].name.length() > 0) header.append(": ");
        header.append(escaped_value);
        header_chunk = curl_slist_append(header_chunk, header.buf_);
    }
    if (header_chunk) {
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
        if (res != CURLE_OK) {
            thread_result = "Problem setting header!";
            thread_status = FINISHED;
            curl_easy_cleanup(curl);
            return;
        }
    }
    
    
    curl_easy_setopt(curl, CURLOPT_URL, url.buf_);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    res = curl_easy_perform(curl);
    pg::String response_body;
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    response_code = (int)resp_code;
    if(res != CURLE_OK) {
        thread_result = pg::String(curl_easy_strerror(res));
        if(chunk.size > 0) thread_result = pg::String(chunk.memory); 
    } else {
        thread_result = pg::String("All ok");
        if(chunk.size > 0) thread_result = pg::String(chunk.memory); 
    }
    thread_status = FINISHED;
    curl_easy_cleanup(curl);
}


void processRequest(std::thread& thread, const char* buf, 
                    pg::Vector<History>& history, const pg::Vector<Argument>& args, 
                    const pg::Vector<Argument>& headers,int request_type, 
                    std::atomic<ThreadStatus>& thread_status)
{
    if (thread_status == RUNNING)
        return;
    History hist;
    hist.url = pg::String(buf);
    hist.args = args;
    hist.result = pg::String("Processing");
    pg::String url(buf);
    history.push_back(hist);

    switch(request_type) { 
        case 0:
            thread_status = RUNNING;
            thread = std::thread(threadRequestGet, std::ref(thread_status), url, args, headers, std::ref(history.back().result), std::ref(hist.response_code));
            break;
        default:
            hist.result = pg::String("Invalid request type selected!");
            thread_status = FINISHED;
    }

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
    // POST
    const char* post_types[] = {"Text", "File"};
    // GET
    const char* get_types[] = {"Text"};
    arg_types.push_back(post_types);
    arg_types.push_back(get_types);

    pg::Vector<History> history;
    curl_global_init(CURL_GLOBAL_ALL);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        {
            ImGui::Begin("Postgirl");//, NULL, ImGuiWindowFlags_NoMove);

            const char* items[] = {"GET", "POST"};
            static int request_type = 0;
            static pg::Vector<Argument> args;
            static pg::Vector<Argument> headers;
            static pg::String result;
            
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
            ImGui::Combo("", &request_type, items, IM_ARRAYSIZE(items));
            ImGui::SameLine();

            static char buf[4098] = "http://localhost:5000/test_route";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            if (ImGui::InputText("URL", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue) ) {
                ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
                processRequest(thread, buf, history, args, headers, request_type, thread_status);
            }


            static pg::Vector<int> delete_arg_btn;
            for (int i=0; i<(int)headers.size(); i++) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                char arg_name[32];
                sprintf(arg_name, "Name##header arg name%d", i);
                if (ImGui::InputText(arg_name, &headers[i].name[0], headers[i].name.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, thread_status);
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.4);
                sprintf(arg_name, "Value##header arg value%d", i);
                if (ImGui::InputText(arg_name, &headers[i].value[0], headers[i].value.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, thread_status);
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
                ImGui::Combo("", &args[i].arg_type, arg_types[request_type], IM_ARRAYSIZE(arg_types[request_type])); 
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                char arg_name[32];
                sprintf(arg_name, "Name##arg name%d", i);
                if (ImGui::InputText(arg_name, &args[i].name[0], args[i].name.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, thread_status);
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.4);
                sprintf(arg_name, "Value##arg name%d", i);
                if (ImGui::InputText(arg_name, &args[i].value[0], args[i].value.capacity(), ImGuiInputTextFlags_EnterReturnsTrue))
                    processRequest(thread, buf, history, args, headers, request_type, thread_status);
                ImGui::SameLine();
                char btn_name[32];
                sprintf(btn_name, "Delete##arg delete%d", i);
                if (ImGui::Button(btn_name)) {
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

            ImGui::Text("Result");
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 500);
            if (thread_status == FINISHED) {
                thread.join();
                thread_status = IDLE;
            }
            if (history.size() > 0)
                ImGui::InputTextMultiline("##source", &history.back().result[0], history.back().result.capacity(), ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
            else {
                char blank[] = "";
                ImGui::InputTextMultiline("##source", blank, 0, ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
            }

            ImGui::PopTextWrapPos();

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

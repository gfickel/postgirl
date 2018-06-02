#include <thread>
#include <atomic>
#include <stdio.h>
#include <vector>
#include "json/json.h"
#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"
#include "GL/gl3w.h"    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include "pgstring.h"


typedef struct Argument { 
    pg::String name;
    pg::String value;
    int arg_type; // TODO: transform this to an ENUM soon!!!!
} Argument; 


typedef struct History {
    pg::String url;
    std::vector<Argument> args;
    pg::String result;
    int response_code;
} History;



// void removeHistory(std::vector<History>& history, int idx) {
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


void threadRequestGet(std::atomic<bool>& request_done, CURL* curl, CURLcode& res, const char* buf, pg::String& thread_result, int& response_code) { 
    MemoryStruct chunk;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, buf);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
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
    request_done = true;
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

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    CURL *curl;
    CURLcode res;
    std::atomic<bool> request_done(false);
    pg::String thread_result;
    int response_code;
    std::thread thread;

    // Such ugly code... Can I do something like this?
    // char** arg_types[] = { {"Text", "File"}, "Text" }; 
    std::vector<const char**> arg_types;
    // POST
    const char* post_types[] = {"Text", "File"};
    // GET
    const char* get_types[] = {"Text"};
    arg_types.push_back(post_types);
    arg_types.push_back(get_types);

    std::vector<History> history;

    /* In windows, this will init the winsock stuff */ 
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
   
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        {
            ImGui::Begin("Postgirl");//, NULL, ImGuiWindowFlags_NoMove);

            const char* items[] = {"GET", "POST"};
            static int request_type = 0;
            static std::vector<Argument> args;
            
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
            ImGui::Combo("", &request_type, items, IM_ARRAYSIZE(items));
            ImGui::SameLine();

            static char buf[4098] = "http://dev.meerkat.com.br:2000/version";
            static bool process_request = false;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            if (ImGui::InputText("URL", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue) ) {
                ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
                process_request = true; 
            }

            static pg::String result;
            if (process_request) {
                if (curl) {
                    thread_result = pg::String("Processing...");
                    result = thread_result;
                    History hist;
                    hist.url = pg::String(buf);
                    hist.args = args;
                    history.push_back(hist);
                    switch(request_type) { 
                        case 0: 
                            thread = std::thread(threadRequestGet, std::ref(request_done), curl, std::ref(res), buf, std::ref(thread_result), std::ref(response_code));
                            break;
                        case 1:
                            thread = std::thread(threadRequestGet, std::ref(request_done), curl, std::ref(res), buf, std::ref(thread_result), std::ref(response_code));
                            break;
                        default:
                            result = thread_result = pg::String("Invalid request type selected!");
                            request_done = true;
                    }
                }
             
                process_request = false;
            }

            static std::vector<int> delete_arg_btn;
            for (int i=0; i<(int)args.size(); i++) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                ImGui::Combo("", &args[i].arg_type, arg_types[request_type], IM_ARRAYSIZE(arg_types[request_type])); 
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.8);
                char arg_name[32];
                sprintf(arg_name, "Arg %d", i);
                ImGui::InputText(arg_name, &args[i].value[0], args[i].value.capacity());
                ImGui::SameLine();
                char btn_name[32];
                sprintf(btn_name, "Delete %d", i);
                if (ImGui::Button(btn_name)) {
                    delete_arg_btn.push_back(i);
                }
            }
            
            // delete the args
            for (int i=(int)delete_arg_btn.size(); i>0; i--) {
                args.erase(args.begin()+delete_arg_btn[i-1]);
            }
            delete_arg_btn.clear();

            if (ImGui::Button("Add Argument")) {
                Argument arg;
                arg.arg_type = 0;
                args.push_back(arg);
            }
            ImGui::SameLine();
            if (ImGui::Button("Delete all args")) {
                args.clear();
            }

            ImGui::Text("Result");
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 500);
            if (request_done) {
                result = thread_result;
                history.back().result = result;
                history.back().response_code = response_code;
                thread.join();
                request_done = false;
            }
            ImGui::InputTextMultiline("##source", &result[0], result.capacity(), ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
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
    
    curl_easy_cleanup(curl);
    
    return 0;
}

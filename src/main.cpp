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


typedef struct Buffer {
    char buf[4098];
} Buffer;

typedef struct Argument { 
    std::string name;
    void* value_ptr;
    int arg_type; // TODO: transform this to an ENUM soon!!!!
} Argument; 


typedef struct History {
    std::string url;
    std::vector<Argument> args;
    std::string result;
    int response_code;
} History;




void removeHistory(std::vector<History>& history, int idx) {
    for (int i=0; i<(int)history[idx].args.size(); i++) {
        free(history[idx].args[i].value_ptr); 
    }
    history.erase(history.begin() + idx);
}


template <typename T>
Argument createArgument(std::string name, const T& val, int argType) {
    Argument arg;
    arg.name = name;
    arg.value_ptr = (void*)malloc(sizeof(T));
    arg.arg_type = argType;

    return arg;
}


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


void threadRequestGet(std::atomic<bool>& request_done, CURL* curl, CURLcode& res, const char* buf, std::string& thread_result, int& response_code) { 
    MemoryStruct chunk;
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, buf);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    //curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
    res = curl_easy_perform(curl);
    std::string response_body;
    long resp_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code);
    response_code = (int)resp_code;
    if(res != CURLE_OK) {
        thread_result = std::string(curl_easy_strerror(res));
        if(chunk.size > 0) thread_result = std::string(chunk.memory); 
    } else {
        thread_result = std::string("All ok");
        if(chunk.size > 0) thread_result = std::string(chunk.memory); 
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
    std::string thread_result;
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
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
            ImGui::Combo("", &request_type, items, IM_ARRAYSIZE(items));
            ImGui::SameLine();

            static char buf[4098] = "";
            static bool process_request = false;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
            if (ImGui::InputText("URL", buf, IM_ARRAYSIZE(buf), ImGuiInputTextFlags_EnterReturnsTrue) ) {
                ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
                process_request = true; 
            }

            static std::string result;
            if (process_request) {
                if (curl) {
                    result = thread_result = std::string("Processing...");
                    History hist;
                    hist.url = std::string(buf);
                    hist.result = std::string("");
                    //hist.args =
                    history.push_back(hist);
                    switch(request_type) { 
                        case 0: 
                            thread = std::thread(threadRequestGet, std::ref(request_done), curl, std::ref(res), buf, std::ref(thread_result), std::ref(response_code));
                            break;
                        case 1:
                            thread = std::thread(threadRequestGet, std::ref(request_done), curl, std::ref(res), buf, std::ref(thread_result), std::ref(response_code));
                            break;
                        default:
                            result = thread_result = std::string("Invalid request type selected!");
                            request_done = true;
                    }
                }
             
                process_request = false;
            }

            static std::vector<Argument> curr_args;
            static std::vector<Buffer> args_buffers;


            if (curr_args.size() == 0) {
                Argument arg = createArgument(std::string("TestArgument"), std::string("Test"), 0);
                curr_args.push_back(arg);
                Buffer b;
                b.buf[0] = '\0';
                args_buffers.push_back(b);
            }

            for (int i=0; i<(int)curr_args.size(); i++) {
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth()*0.2);
                ImGui::Combo("", &curr_args[i].arg_type, arg_types[request_type], IM_ARRAYSIZE(arg_types[request_type])); 
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
                ImGui::InputText("Argument", args_buffers[i].buf, IM_ARRAYSIZE(args_buffers[i].buf));
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
            ImGui::InputTextMultiline("##source", &result[0], result.size(), ImVec2(-1.0f, ImGui::GetContentRegionAvail()[1]), ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_ReadOnly);
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

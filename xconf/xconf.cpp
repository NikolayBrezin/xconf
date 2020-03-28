// XConf.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <any>
#include <functional>

#include "json.h"

// dear imgui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include "gl3w/gl3w.h"            // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>            // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>          // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h>  // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE       // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h>// Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif



static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

struct TreeItem;
using TreeItemPtr = std::shared_ptr<TreeItem>;
struct TreeItem
{
    std::string _key;
    std::string _name;
    std::string _info;
    std::string _type;
    std::string _unit;

    std::any _value;
    std::any _min_v;
    std::any _max_v;
    std::any _val_default;

    bool _array_flag = false;
    std::vector<TreeItemPtr> _children;
    TreeItem(){}
    TreeItem(const std::string& name): _name(name) {}
    TreeItem(const std::string& name, const std::string& info) : _name(name), _info(info) {}
    void add_child(TreeItemPtr item)
    {
        _children.emplace_back(std::move(item));
    }
    
    template<typename T> void draw_control(std::any& value_, std::any& min_v, std::any& max_v, const char*fmt
        , std::function<bool (T*, T, T)> fun)
    {
        T value = std::any_cast<T>(value_);
        T v_min = std::any_cast<T>(min_v);
        T v_max = std::any_cast<T>(max_v);
        ImGui::Text(fmt, v_min); 
        ImGui::SameLine();
        if (fun(&value, v_min, v_max))
        {
            value_ = value;
        }
        ImGui::SameLine();
        ImGui::Text(fmt, v_max);
    }
    
    void draw()
    {
        if (!_children.empty())
        {
            if (ImGui::TreeNodeEx(_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                for (const auto& item : _children)
                {
                    item->draw();
                }
                ImGui::TreePop();
            }
        }
        else
        {
            ImGui::PushItemWidth(50);
            ImGui::Text("%-30s:", _name.c_str() );            
            ImGui::SameLine();
            ImGui::PushID(_key.c_str());
            if (_type == "uint32_t")
            {
                draw_control<int>(_value, _min_v, _max_v, "%5d", [](int* value, int v_min, int v_max)
                    {
                        return ImGui::DragInt("", value, (v_max - v_min) / 100, v_min, v_max);
                    });
            }
            else if (_type == "float")
            {
                draw_control<float>(_value, _min_v, _max_v, "%5.2f", [](float* value, float v_min, float v_max)
                    {
                        return ImGui::DragFloat("", value, (v_max - v_min) / 100, v_min, v_max);
                    });
            }
            else
                assert(0 && "unknown type");
            ImGui::PopID();
            ImGui::PopItemWidth();
            ImGui::SameLine(); HelpMarker(_info.c_str());
        }
    };
    void serialize(FILE* f, std::string tabs = "")
    {
        if (!tabs.empty())
            fprintf(f, "%s\"%s\": \n", tabs.c_str(), _key.c_str());
        if (!_children.empty())
        {
            fprintf(f, "%s%s\n", tabs.c_str(), _array_flag ? "[" : "{");
            {
                std::string tabs_ = tabs + "    ";
                if (!_name.empty())
                    fprintf(f, "%s\"name\": \"%s\",\n", tabs_.c_str(), _name.c_str());
                //for (const auto& item : _children)
                for (int i = 0; i < _children.size(); ++i)
                {
                    const auto& item = _children[i];
                    item->serialize(f, tabs_);
                    if (i != _children.size() - 1)
                        fprintf(f, ",\n");
                    else
                        fprintf(f, "\n");
                }
            }
            fprintf(f, "%s%s", tabs.c_str(), _array_flag ? "]" : "}");
        }
        else
        {            
            fprintf(f, "%s{\n", tabs.c_str());
            {
                std::string tabs_ = tabs + "    ";
                fprintf(f, "%s\"name\": \"%s\",\n",      tabs_.c_str(), _name.c_str());
                fprintf(f, "%s\"info\": \"%s\",\n",      tabs_.c_str(), _info.c_str());
                fprintf(f, "%s\"typeof\": \"%s\",\n",    tabs_.c_str(), _type.c_str());
                fprintf(f, "%s\"unit\": \"%s\",\n",      tabs_.c_str(), _unit.c_str());
                if (_type == "float")
                {
                    fprintf(f, "%s\"val\": %f,\n",           tabs_.c_str(), std::any_cast<float>(_value));
                    fprintf(f, "%s\"val_min\": %f,\n",       tabs_.c_str(), std::any_cast<float>(_min_v));
                    fprintf(f, "%s\"val_max\": %f,\n",       tabs_.c_str(), std::any_cast<float>(_max_v));
                    fprintf(f, "%s\"val_default\": %f\n",    tabs_.c_str(), std::any_cast<float>(_val_default));
                }
                else if (_type == "uint32_t")
                {
                    fprintf(f, "%s\"val\": %d,\n",           tabs_.c_str(), std::any_cast<int>(_value));
                    fprintf(f, "%s\"val_min\": %d,\n",       tabs_.c_str(), std::any_cast<int>(_min_v));
                    fprintf(f, "%s\"val_max\": %d,\n",       tabs_.c_str(), std::any_cast<int>(_max_v));
                    fprintf(f, "%s\"val_default\": %d\n",   tabs_.c_str(), std::any_cast<int>(_val_default));
                }
            }
            fprintf(f, "%s}", tabs.c_str());
        }
    }
};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

const char* glsl_version = "#version 130";
GLFWwindow* window = nullptr;

void init_style2()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    style->WindowRounding = 2.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    style->ScrollbarRounding = 3.0f;             // Radius of grab corners rounding for scrollbar
    style->GrabRounding = 2.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    style->AntiAliasedLines = true;
    style->AntiAliasedFill = true;
    style->WindowRounding = 2;
    style->ChildRounding = 2;
    style->ScrollbarSize = 16;
    style->ScrollbarRounding = 3;
    style->GrabRounding = 2;
    style->ItemSpacing.x = 10;
    style->ItemSpacing.y = 4;
    style->IndentSpacing = 22;
    style->FramePadding.x = 6;
    style->FramePadding.y = 4;
    style->Alpha = 1.0f;
    style->FrameRounding = 3.0f;

    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    //colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.93f, 0.93f, 0.93f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.71f, 0.71f, 0.71f, 0.08f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.71f, 0.71f, 0.71f, 0.55f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.94f, 0.94f, 0.55f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.71f, 0.78f, 0.69f, 0.98f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.82f, 0.78f, 0.78f, 0.51f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.61f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.90f, 0.90f, 0.90f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.92f, 0.92f, 0.92f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.184f, 0.407f, 0.193f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.71f, 0.78f, 0.69f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.725f, 0.805f, 0.702f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.793f, 0.900f, 0.836f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.71f, 0.78f, 0.69f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.71f, 0.78f, 0.69f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.71f, 0.78f, 0.69f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
}

void init_style(bool bStyleDark_, float alpha_)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

    if (bStyleDark_)
    {
        for (int i = 0; i <= ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            float H, S, V;
            ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, H, S, V);

            if (S < 0.1f)
            {
                V = 1.0f - V;
            }
            ImGui::ColorConvertHSVtoRGB(H, S, V, col.x, col.y, col.z);
            if (col.w < 1.00f)
            {
                col.w *= alpha_;
            }
        }
    }
    else
    {
        for (int i = 0; i <= ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            if (col.w < 1.00f)
            {
                col.x *= alpha_;
                col.y *= alpha_;
                col.z *= alpha_;
                col.w *= alpha_;
            }
        }
    }
}

int init(GLFWwindow*& window)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions

    // GL 3.0 + GLSL 130
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    window = glfwCreateWindow(1280, 1024, "XConf", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char* name) { return (glbinding::ProcAddress)glfwGetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    return 0;
}

std::vector<TreeItemPtr> nodes_stack;
std::string last_key = "root";

template<typename T> void set_any(T value, std::any& any)
{
    any = value;
    T value_check = std::any_cast<T>(any);
}

int parse_callback(void* userdata, int type, const char* data, uint32_t length)
{
    switch (type) {
    case JSON_OBJECT_BEGIN:
    case JSON_ARRAY_BEGIN:
    {
        //printf("entering %s\n", (type == JSON_ARRAY_BEGIN) ? "array" : "object");
        auto node = std::make_shared<TreeItem>();
        node->_key = last_key;
        if (type == JSON_ARRAY_BEGIN)
            node->_array_flag = true;
        if (!nodes_stack.empty())
        {
            nodes_stack.back()->add_child(node);
        }
        nodes_stack.emplace_back(node);
    }
    break;
    case JSON_OBJECT_END:
    case JSON_ARRAY_END:
    {
        //printf("leaving %s\n", (type == JSON_ARRAY_END) ? "array" : "object");
        if (nodes_stack.size() != 1)
            nodes_stack.pop_back();
    }
    break;
    case JSON_KEY:
    {
        last_key = data;
    }
    break;
    case JSON_STRING:
    {
        //printf("string value %*s\n", length, data);
        if (last_key == "name")
        {
            nodes_stack.back()->_name = data;
        }
        else if (last_key == "info")
        {
            nodes_stack.back()->_info = data;
        }
        else if (last_key == "typeof")
        {
            nodes_stack.back()->_type = data;
        }
        else if (last_key == "unit")
        {
            nodes_stack.back()->_unit = data;
        }
        else
        {
            //assert(0 && "unsupported record");
        }
    }
        break;
    case JSON_INT:
        {
            auto& node = nodes_stack.back();
            if (last_key == "val")
            {
                if      (node->_type == "float")    set_any((float) std::atof(data), node->_value);
                else if (node->_type == "uint32_t") set_any((int)   std::atoi(data), node->_value);
                else assert(0);
            }
            else if (last_key == "val_min")
            {
                if      (node->_type == "float")    set_any((float) std::atof(data), node->_min_v);
                else if (node->_type == "uint32_t") set_any((int)   std::atoi(data), node->_min_v);
                else assert(0);
            }
            else if (last_key == "val_max")
            {
                if      (node->_type == "float")    set_any((float) std::atof(data), node->_max_v);
                else if (node->_type == "uint32_t") set_any((int)   std::atoi(data), node->_max_v);
                else assert(0);
            }
            else if (last_key == "val_default")
            {
                if      (node->_type == "float")    set_any((float) std::atof(data), node->_val_default);
                else if (node->_type == "uint32_t") set_any((int)   std::atoi(data), node->_val_default);
                else assert(0);
            }
        }
        break;
    case JSON_FLOAT:
    {
        auto& node = nodes_stack.back();
        if (last_key == "val")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_value);
            else assert(0);
        }
        else if (last_key == "val_min")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_min_v);
            else assert(0);
        }
        else if (last_key == "val_max")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_max_v);
            else assert(0);
        }
        else if (last_key == "val_default")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_val_default);
            else assert(0);
        }
    }
        break;
    case JSON_NULL:
        //printf("constant null\n"); 
        break;
    case JSON_TRUE:
        //printf("constant true\n"); 
        break;
    case JSON_FALSE:
        //printf("constant false\n"); 
        break;
    }
    return 0;
}

TreeItemPtr root;

void init_xconf()
{
    void* my_callback_data = nullptr;
    json_parser parser;

    if (json_parser_init(&parser, NULL, parse_callback, &root)) 
    {
        fprintf(stderr, "something wrong happened during init\n");
    }

    std::ifstream input(".\\data\\fadec.json.data");
    while (input)
    {
        std::string line;
        std::getline(input, line);
        auto ret = json_parser_string(&parser, line.c_str(), line.size(), nullptr);
        if (ret) 
        {
            /* error happened : print a message or something */
            break;
        }
    }
    json_parser_free(&parser);
    root = nodes_stack.back();
}

void render_xconf_window()
{
    ImGui::Begin("XConf");
    if (ImGui::Button("Load"))
    {
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        FILE* f = fopen(".\\data\\out.json", "w");
        assert(f);
        root->serialize(f);
        fclose(f);
    }
    root->draw();
    ImGui::End();
}

int main(int, char**)
{
    init(window);

    init_xconf();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    //ImFont* font = io.Fonts->AddFontFromFileTTF("courier.ttf", 13);
    ImFont* font = io.Fonts->AddFontFromFileTTF(".\\data\\fonts\\DejaVuLGCSansMono.ttf", 13.0f, NULL, io.Fonts->GetGlyphRangesCyrillic());
    //init_style(true, 0.5f);
    init_style2();


    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        render_xconf_window();
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
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

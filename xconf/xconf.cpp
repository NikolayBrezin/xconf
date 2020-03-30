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
#include <algorithm>
#include <sstream>
#include <stdio.h>

#include "json.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "gl3w/gl3w.h"            // Initialize with gl3wInit()
#include <GLFW/glfw3.h>
#include <libserialport.h>

// [Win32] Our example includes a copy of glfw3.lib pre-compiled with VS2010 to maximize ease of testing and compatibility with old VS compilers.
// To link with VS2010-era libraries, VS2015+ requires linking with legacy_stdio_definitions.lib, which we do using this pragma.
// Your own project should not be affected, as you are likely to link with a newer binary of GLFW that is adequate for your version of Visual Studio.
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

const char* glsl_version = "#version 130";

std::string test_json_file_name = "./data/test.json";
const char* device_read_file_name = "./data/device_read.json";
const char* device_write_file_name = "./data/device_write.json";
GLFWwindow* window = nullptr;
static int current_font = 5;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
bool show_demo_window = false;

struct TreeItem;
using TreeItemPtr = std::shared_ptr<TreeItem>;
TreeItemPtr root;

static bool read_serial(std::string& json)
{
  /* A pointer to a null-terminated array of pointers to
   * struct sp_port, which will contain the ports found.*/
  struct sp_port **port_list;

  std::cout<<"Getting port list." << std::endl;

  /* Call sp_list_ports() to get the ports. The port_list
   * pointer will be updated to refer to the array created. */
  enum sp_return result = sp_list_ports(&port_list);

  if (result != SP_OK)
    {
        std::cout<<"sp_list_ports() failed!" << std::endl;
  	return 0;
    }

  /* Iterate through the ports. When port_list[i] is NULL
   * this indicates the end of the list. */
  int i;
  for (i = 0; port_list[i] != NULL; i++)
    {
  	struct sp_port *port = port_list[i];

  	std::cout<<"port: " << sp_get_port_name(port) << std::endl;
  	std::cout<<"\t: " << sp_get_port_description(port) << std::endl;


	/* Identify the transport which this port is connected through,
	 * e.g. native port, USB or Bluetooth. */
	enum sp_transport transport = sp_get_port_transport(port);

	if (transport == SP_TRANSPORT_NATIVE) {
		/* This is a "native" port, usually directly connected
		 * to the system rather than some external interface. */
		std::cout<<"\t: type: native" << std::endl;
	} else if (transport == SP_TRANSPORT_USB) {
		/* This is a USB to serial converter of some kind. */
		std::cout<<"\t: type: usb" << std::endl;

		/* Display string information from the USB descriptors. */
		std::string serial = sp_get_port_usb_serial(port) ;

        auto man = sp_get_port_usb_manufacturer(port);
        auto prod = sp_get_port_usb_product(port);
		std::cout<<"\t: manufacturer:\t" << (man ? man : "unknown") << std::endl;
		std::cout<<"\t: product:\t"      << (prod ? prod : "unknown") << std::endl;
		std::cout<<"\t: serial:\t"       << serial << std::endl;

		/* Display USB vendor and product IDs. */
		int usb_vid, usb_pid;
		sp_get_port_usb_vid_pid(port, &usb_vid, &usb_pid);

		std::cout << "\t vid:" << std::hex << usb_vid << "\tpid:" << usb_pid << std::dec << std::endl;

		/* Display bus and address. */
		int usb_bus, usb_address;
		sp_get_port_usb_bus_address(port, &usb_bus, &usb_address);
		std::cout << "\t bus:" << std::hex << usb_bus << "\taddress:" << usb_address << std::dec << std::endl;


		// проверяемся по серийному номеру девайса
		if
#ifdef __linux__
	( serial == std::string("348F377A8540"))
#else
	(true)
#endif		    
		{
		    sp_return spr ;
		    std::cout << "fadec found" << std::endl ;
		    spr = sp_open(port, SP_MODE_READ_WRITE);
		    constexpr char device_read_config_command[] = "rc\r" ;

		    spr = sp_blocking_write(port, device_read_config_command , strlen(device_read_config_command) , 1000);

		    constexpr size_t size = 32*1024;
		    char buff[size] ;
		    spr = sp_blocking_read(port, buff, size, 1000);
		    if ( !spr )
		      std::cout << "fadec read timeout" << std::endl ;
		    else
		      {
		        std::cout << buff+strlen(device_read_config_command) << std::endl ;
		        json = ( std::string ( buff )).substr ( strlen(device_read_config_command), 17809 ) ;



		      }

		    spr = sp_close(port) ;
		    (void)spr ;
		    return true ;
		}


	} else if (transport == SP_TRANSPORT_BLUETOOTH) {
		/* This is a Bluetooth serial port. */
		std::cout<<"\t: type: bluetooth" << std::endl;

		/* Display Bluetooth MAC address. */
		std::cout<<"\t: mac: " << sp_get_port_bluetooth_address(port) << std::endl;
	}
    }

   std::cout<<"Found "<< i << " ports"<< std::endl;

   /* Free the array created by sp_list_ports(). */
   sp_free_port_list(port_list);

   /* Note that this will also free all the sp_port structures
    * it points to. If you want to keep one of them (e.g. to
    * use that port in the rest of your program), take a copy
    * of it first using sp_copy_port(). */


   return false ;

}

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
static void TextWithTooltip(const char* fmt, const char* desc, const char* tooltip)
{
    ImGui::TextColored(ImVec4(1, 1, 0, 1), fmt, desc, tooltip);
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tooltip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

struct DeviceInfo
{
    std::string _hw_ver;
    std::string _fw_ver;
    std::string _ser_num;
};
DeviceInfo device_info;

struct TreeItem
{
    std::string _key;
    std::string _name;
    std::string _info;
    std::string _type;
    std::string _unit;
    std::string _id;

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
        , std::function<bool (T*, T, T)> fun, const char* unit)
    {
        T value = std::any_cast<T>(value_);
        T v_min = std::any_cast<T>(min_v);
        T v_max = std::any_cast<T>(max_v);
        ImGui::Text(fmt, v_min); 
        ImGui::SameLine();
        ImGui::PushItemWidth(75);
        if (fun(&value, v_min, v_max))
        {
            if (v_min == v_max)
                value_ = value;
            else
                value_ = std::clamp(value, v_min, v_max);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text(fmt, v_max);
        ImGui::SameLine();
        ImGui::Text(unit);
    }

    void process_children()
    {
        for (const auto& item : _children)
        {
            item->draw();
        }
    }
    void draw()
    {
        if (!_children.empty())
        {
            if (!_name.empty())
            {
                if (ImGui::TreeNodeEx(_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    process_children();
                    ImGui::TreePop();
                }
            }
            else
                process_children();
        }
        else
        {
            if (!_type.empty())
            {
                TextWithTooltip("%s:", _name.c_str(), _info.c_str());
                ImGui::SameLine();
                ImGui::SetCursorPosX(500);
                ImGui::PushID(_key.c_str());
                if (_type == "uint32_t")
                {
                    draw_control<int>(_value, _min_v, _max_v, "%5d", [](int* value, int v_min, int v_max)
                        {
                            auto res = ImGui::DragInt("", value, float(v_max - v_min) / 100, v_min, v_max);
                            return res;
                        }, _unit.c_str());

                }
                else if (_type == "float")
                {
                    draw_control<float>(_value, _min_v, _max_v, "%5.2f", [](float* value, float v_min, float v_max)
                        {
                            auto res = ImGui::DragFloat("", value, (v_max - v_min) / 100, v_min, v_max);
                            return res;
                        }, _unit.c_str());
                }
                else if (_type == "const_string")
                {
                    std::string value = std::any_cast<std::string>(_value);
                    std::string v_min = std::any_cast<std::string>(_min_v);
                    std::string v_max = std::any_cast<std::string>(_max_v);
                    ImGui::Text(v_min.c_str());
                    ImGui::SameLine();
                    ImGui::PushItemWidth(75);
                    ImGui::Text("%s", value.c_str());
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::Text(v_max.c_str());
                    ImGui::SameLine();
                    ImGui::Text(_unit.c_str());
                }
                else
                {
                    //assert(0 && "unknown type");
                }
                ImGui::PopID();
            }
        }
    };
    template<typename T> void out_data(std::stringstream& s, std::string tabs = "", bool braces = false)
    {
        s << tabs << "\"val\": "            << (braces ? "\"" : "") << std::any_cast<T>(_value)        << (braces ? "\"" : "") << ", \n";
        s << tabs << "\"val_min\": "        << (braces ? "\"" : "") << std::any_cast<T>(_min_v)        << (braces ? "\"" : "") << ", \n";
        s << tabs << "\"val_max\": "        << (braces ? "\"" : "") << std::any_cast<T>(_max_v)        << (braces ? "\"" : "") << ", \n";
        s << tabs << "\"val_default\": "    << (braces ? "\"" : "") << std::any_cast<T>(_val_default)  << (braces ? "\"" : "") << "  \n";
    }
    void serialize(std::stringstream& s, std::string tabs = "")
    {
        if (!tabs.empty())
            s << tabs << "\"" << _key << "\": \n";
        if (!_children.empty())
        {
            if (!tabs.empty())
                s << tabs.c_str() << (_array_flag ? "[" : "{") << "\n";
            {
                std::string tabs_ = tabs + "    ";
                if (!_name.empty())
                    s << tabs_ << "\"name\": \"" << _name << "\",\n";
                for (size_t i = 0; i < _children.size(); ++i)
                {
                    const auto& item = _children[i];
                    item->serialize(s, tabs_);
                    if (i != _children.size() - 1)
                        s << ",\n";
                    else
                        s << "\n";
                }
            }
            if (!tabs.empty())
                s << tabs << (_array_flag ? "]" : "}");
        }
        else
        {            
            s << tabs << "{\n";
            {
                std::string tabs_ = tabs + "    ";
                s << tabs_ << "\"name\": \""    << _name << "\",\n";
                s << tabs_ << "\"info\": \""    << _info << "\",\n";
                s << tabs_ << "\"typeof\": \""  << _type << "\",\n";
                s << tabs_ << "\"unit\": \""    << _unit << "\",\n";
                if (_type == "float")
                {
                    out_data<float>(s, tabs_);
                }
                else if (_type == "uint32_t")
                {
                    out_data<int>(s, tabs_);
                }
                else if (_type == "const_string")
                {
                    out_data<std::string>(s, tabs_, true);
                }
		s << tabs_ << "\"id\": \""    << _id << "\",\n";
            }
            s << tabs << "}";
        }
    }
};

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

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
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    window = glfwCreateWindow(1280, 1024, "XConf", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    bool err = gl3wInit() != 0;
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    for (int i = 10; i <= 20; ++i)
        io.Fonts->AddFontFromFileTTF("./data/fonts/DejaVuLGCSansMono.ttf", i, NULL, io.Fonts->GetGlyphRangesCyrillic());

    ImFont* font = io.Fonts->Fonts[current_font];
    io.FontDefault = font;
    //init_style(true, 0.5f);
    //init_style2();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    return 0;
}

std::vector<TreeItemPtr> nodes_stack;
std::string last_key = "root";

template<typename T> void set_any(T value, std::any& any)
{
    any = value;
}

void set_data(TreeItemPtr& node, const char* data, std::any& field)
{
    if (node->_type == "float")    set_any((float)std::atof(data), field);
    else if (node->_type == "uint32_t") set_any((int)std::atoi(data), field);
    else if (node->_type == "const_string") set_any((std::string)(data), field);
    else std::cout << "error rading data: unknown type [" << node->_type << "]\n";
}

int parse_callback(void* userdata, int type, const char* data, uint32_t length)
{
    switch (type) {
    case JSON_OBJECT_BEGIN:
    case JSON_ARRAY_BEGIN:
    {
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
        else if (last_key == "val")
        {
            nodes_stack.back()->_value = std::string(data);
        }
        else if (last_key == "val_min")
        {
            nodes_stack.back()->_min_v = std::string(data);
        }
        else if (last_key == "val_max")
        {
            nodes_stack.back()->_max_v = std::string(data);
        }
        else if (last_key == "val_default")
        {
            nodes_stack.back()->_val_default = std::string(data);
        }
	else if (last_key == "id")
	{
	    nodes_stack.back()->_id = std::string(data);
	}
        else
        {
            std::cout << "error parsing unknown last_key: " << last_key << "\n";
        }
    }
        break;
    case JSON_INT:
        {
            auto& node = nodes_stack.back();
            if (last_key == "val")
            {
                set_data(node, data, node->_value);
            }
            else if (last_key == "val_min")
            {
                set_data(node, data, node->_min_v);
            }
            else if (last_key == "val_max")
            {
                set_data(node, data, node->_max_v);
            }
            else if (last_key == "val_default")
            {
                set_data(node, data, node->_val_default);
            }
            else
                std::cout << "error parsing unknown last_key: " << last_key << "\n";
        }
        break;
    case JSON_FLOAT:
    {
        auto& node = nodes_stack.back();
        if (last_key == "val")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_value);
            else std::cout << "error parsing unknown type: " << node->_type << "\n";
        }
        else if (last_key == "val_min")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_min_v);
            else std::cout << "error parsing unknown type: " << node->_type << "\n";
        }
        else if (last_key == "val_max")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_max_v);
            else std::cout << "error parsing unknown type: " << node->_type << "\n";
        }
        else if (last_key == "val_default")
        {
            if (node->_type == "float")    set_any((float)std::atof(data), node->_val_default);
            else std::cout << "error parsing unknown type: " << node->_type << "\n";
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

void load_data(TreeItemPtr& root)
{
    json_parser parser;

    if (json_parser_init(&parser, NULL, parse_callback, &root)) 
    {
        fprintf(stderr, "something wrong happened during init\n");
    }
#if 0
    std::cout << "load JSON config from file " << test_json_file_name << "\n";
    std::ifstream input(test_json_file_name);
    if (!input)
        std::cout << "error opening file " << test_json_file_name << " for reading \n";
    if(!input.is_open())
        std::cout << "error opening file " << test_json_file_name << " for reading \n";

    int counter = 1;
    while (input)
    {
        std::string line;
        std::getline(input, line);
        auto ret = json_parser_string(&parser, line.c_str(), (uint32_t)line.size(), nullptr);
        if (ret) 
        {
            std::cout << "error parsing line " << counter << ": "  << line << "\n";
            exit(-1);
        }
        ++counter;
    }
#else
    std::string json;

    if (read_serial(json))
    {
        // сохранение ответа девайса
	std::cout << "save readed device JSON config to file " << device_read_file_name << "\n";
	std::ofstream output(device_read_file_name);
	if(output.is_open())
	{
	  output<<json ;
	  output.close(); 
 	}
	else
	{
	  std::cout << "error opening file " << device_read_file_name << " for writing \n";
	}

        if (json_parser_string(&parser, json.c_str(), (uint32_t)json.size(), nullptr))
        {
            std::cout << "device json data parsing error, see dump /data/in.json" << std::endl;
            exit(-1);
        }
    }
#endif

    json_parser_free(&parser);
    if (!nodes_stack.empty())
        root = nodes_stack.back();
}

void save_data(TreeItemPtr& root)
{
    if (root)
    {
        std::stringstream s;
        s << "{\n";
        root->serialize(s);
        s << "}\n";
        
        // сохранения отредактированого JSON девайса
        std::cout << "save writed device JSON config to file " << device_write_file_name << "\n";
        std::ofstream output(device_write_file_name);
        if(output.is_open())
	{
	  output << s.str();
          output.close();
	}
	else
	{
	   std::cout << "error opening file " << device_write_file_name << " for writing \n";
	}

    }
}

void render_xconf_window()
{
    ImGui::Begin("XConf");

    ImGui::Text("Device info");
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        load_data(root);
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        if (root)
        {
            save_data(root);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Font size:");
    ImGui::SameLine();
    if (ImGui::Button("-"))
    {
        ImGuiIO& io = ImGui::GetIO();
        int font_id = current_font - 1;
        if (font_id >= 0)
        {
            ImFont* font = io.Fonts->Fonts[font_id];
            io.FontDefault = font;
            current_font = font_id;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("+"))
    {
        ImGuiIO& io = ImGui::GetIO();
        int font_id = current_font + 1;
        if (font_id < io.Fonts->Fonts.Size)
        {
            ImFont* font = io.Fonts->Fonts[font_id];
            io.FontDefault = font;
            current_font = font_id;
        }
    }
    if (root)
        root->draw();
    ImGui::End();
}

int main(int, char**)
{
    init(window);

    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

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

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

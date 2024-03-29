#pragma once

#include <imgui.h>

class Theme {
public:
    static constexpr auto error_colour = ImVec4(0.749f, 0.380f, 0.415f, 1.0f);
    static constexpr auto text_prominent = ImVec4(0.752f, 0.505f, 0.674f, 1.0f);

    static void SetupImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.300000011920929f;
        style.WindowPadding = ImVec2(6.5f, 6.5f);
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.WindowMinSize = ImVec2(20.0f, 32.0f);
        style.WindowTitleAlign = ImVec2(0.0f, 0.6000000238418579f);
        style.WindowMenuButtonPosition = ImGuiDir_None;
        style.ChildRounding = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(7.5f, 5.5f);
        style.FrameRounding = 0.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(4.400000095367432f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.599999904632568f, 3.599999904632568f);
        style.CellPadding = ImVec2(3.099999904632568f, 3.5f);
        style.IndentSpacing = 4.400000095367432f;
        style.ColumnsMinSpacing = 5.400000095367432f;
        style.ScrollbarSize = 12.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabMinSize = 9.399999618530273f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.TabBorderSize = 0.0f;
        style.TabMinWidthForCloseButton = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

        style.Colors[ImGuiCol_Text] = ImVec4(0.9254902005195618f, 0.9372549057006836f, 0.95686274766922f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.3333333432674408f, 0.3607843220233917f, 0.4039215743541718f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1803921610116959f, 0.2039215713739395f, 0.250980406999588f, 0.9411764740943909f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2313725501298904f, 0.2588235437870026f, 0.321568638086319f, 0.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1725490242242813f, 0.1921568661928177f, 0.2352941185235977f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.2313725501298904f, 0.2588235437870026f, 0.321568638086319f, 0.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2627451121807098f, 0.2980392277240753f, 0.3686274588108063f, 0.5411764979362488f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 0.4000000059604645f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.3058823645114899f, 0.3490196168422699f, 0.4313725531101227f, 0.6705882549285889f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2078431397676468f, 0.2352941185235977f, 0.294117659330368f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2627451121807098f, 0.2980392277240753f, 0.3686274588108063f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.156f, 0.180f, 0.219, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2313725501298904f, 0.2588235437870026f, 0.321568638086319f, 0.529411792755127f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.321568638086319f, 0.3568627536296844f, 0.4431372582912445f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.3686274588108063f, 0.5058823823928833f, 0.6745098233222961f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3686274588108063f, 0.5058823823928833f, 0.6745098233222961f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.501960813999176f, 0.6117647290229797f, 0.7450980544090271f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.2627451121807098f, 0.2980392277240753f, 0.3686274588108063f, 0.4000000059604645f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.3058823645114899f, 0.3490196168422699f, 0.4313725531101227f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.4156862795352936f, 0.4705882370471954f, 0.5764706134796143f, 0.3098039329051971f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 0.800000011920929f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.3058823645114899f, 0.3490196168422699f, 0.4313725531101227f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.156f, 0.180f, 0.219, 1.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4196078479290009f, 0.47843137383461f, 0.5882353186607361f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.3058823645114899f, 0.3490196168422699f, 0.4313725531101227f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.156f, 0.180f, 0.219, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
    }
};
#include "TextureNode.h"
#include "Utils.h"
#include "Data/ProjectData.h"
#include "Texture2D.h"
#include "Base/ImGuiShapes.h"
#include "Generators/CPUNodeEditor/CPUNodeEditor.h"
#include <iostream>
#include <implot.h>
#include <mutex>
#include "Base/ImGuiCurveEditor.h"

#define CLAMP01(x) x > 1 ? 1 : ( x < 0 ? 0 : x )

NodeOutput TextureNode::Evaluate(NodeInputParam input, NodeEditorPin* pin)
{
    if (isDefault)
        return NodeOutput({0.0f});
    float res, sc, x, y;
    res = sc = x = y = 0.0f;
    int channel = 0;

    if (pin->id == outputPins[0]->id)
        channel = 0;
    else if (pin->id == outputPins[1]->id)
        channel = 1;
    else if (pin->id == outputPins[2]->id)
        channel = 2;

    if (inputPins[0]->IsLinked())
        x = inputPins[0]->other->Evaluate(input).value;
    else
        x = input.texX;

    if (inputPins[1]->IsLinked())
        y = inputPins[1]->other->Evaluate(input).value;
    else
        y = input.texY;


    if (inputPins[2]->IsLinked())
        sc = inputPins[2]->other->Evaluate(input).value;
    else
        sc = scale;
    mutex.lock();
    int xC = (int)(x * (texture->GetWidth()-1));
    int yC = (int)(y * (texture->GetHeight()-1));
    unsigned char elevC = texture->GetData()[yC * texture->GetWidth() * 3 + xC * 3 + channel];
    res = (float)elevC / 256;
    res = res * 2.0 - 1.0f;
    mutex.unlock();
    return NodeOutput({ res });
}

void TextureNode::Load(nlohmann::json data)
{
    scale = data["scale"];
    if (isDefault && data["isDefault"])
        return;
    isDefault = data["isDefault"];
    if (isDefault)
    {
        delete texture;
        texture = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png", false, false);
    }
    else
    {
        std::string hash = data["texture"];
        if (!ProjectAssetExists(hash))
        {
            ShowMessageBox("Failed to Load Texture : " + hash, "Error");
            isDefault = true;
        }
        else
        {
            delete texture;
            texture = new Texture2D(GetProjectResourcePath() + "\\" + GetProjectAsset(hash));
            Log("Loaded Cached Texture : " + hash);
        }
    }
    // TODO : Load Image
}

nlohmann::json TextureNode::Save()
{
    nlohmann::json data;
    data["type"] = MeshNodeEditor::MeshNodeType::Texture;
    data["scale"] = scale;
    data["isDefault"] = isDefault;
    
    if (!isDefault)
    {
        std::string hash = MD5File(texture->GetPath()).ToString();
        data["texture"] = hash;
        if (!ProjectAssetExists(hash))
        {            
            MkDir(GetProjectResourcePath() + "\\textures");
            CopyFileData(texture->GetPath(), GetProjectResourcePath() + "\\textures\\" + hash);
            RegisterProjectAsset(hash, "textures\\" + hash);
            Log("Cached " + texture->GetPath());
        }
        else
        {
            Log("Texture already Cached : " + hash);
        }
    }

    // TODO : Save Image

    return data;
}

void TextureNode::OnRender()
{
    DrawHeader("Texture");

    inputPins[0]->Render();
    ImGui::Text("X");
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(150, 10));
    ImGui::SameLine();
    ImGui::Text("R");
    outputPins[0]->Render();

    inputPins[1]->Render();
    ImGui::Text("Y");
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(150, 10));
    ImGui::SameLine();
    ImGui::Text("G");
    outputPins[1]->Render();

    inputPins[2]->Render();
    if (inputPins[2]->IsLinked())
        ImGui::Text("Scale");
    else
    {
        ImGui::PushItemWidth(100);
        ImGui::DragFloat(("##" + std::to_string(inputPins[1]->id)).c_str(), &scale, 0.01f);
        ImGui::PopItemWidth();
    }
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(60, 10));
    ImGui::SameLine();
    ImGui::Text("B");
    outputPins[2]->Render();

    ImGui::NewLine();

    if (ImGui::ImageButton((ImTextureID)texture->GetRendererID(), ImVec2(200, 200)))
        ChangeTexture();

    if (ImGui::Button(MAKE_IMGUI_LABEL(id, "Change Texture")))
        ChangeTexture();

}

void TextureNode::ChangeTexture()
{
    std::string path = ShowOpenFileDialog(".png");
    if (path.size() < 3)
        return;
    isDefault = false;
    delete texture;
    texture = new Texture2D(path, true, false);
    texture->Resize(256, 256);
    Log("Loaded Texture : " + texture->GetPath());
}


TextureNode::TextureNode()
{
    inputPins.push_back(new NodeEditorPin());
    inputPins.push_back(new NodeEditorPin());
    inputPins.push_back(new NodeEditorPin());
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    outputPins.push_back(new NodeEditorPin(NodeEditorPinType::Output));
    headerColor = ImColor(IMAGE_NODE_COLOR);
    texture = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png", false, false);
    isDefault = true;
    scale = 1.0f;
}

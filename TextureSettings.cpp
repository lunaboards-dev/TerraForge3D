#include "TextureSettings.h"
#include <Utils.h>
#include <imgui/imgui.h>
#include <Texture2D.h>
#include <TextureStore.h>
#include <json.hpp>


bool* reqrfrsh;
float* textureScale;
int co = 0;
int cTexID = 0;
uint32_t diffuseUBOLocCache = -1;
uint32_t diffuseUBO;
Texture2D* diffuse;

struct TextureLayer {
	Texture2D* texture;
	std::string name = "Texture Layer";
	float heightMax = 1;
	float heightMin = -1;
	float textureScale = 1;
};


std::vector<TextureLayer> textureLayers;

std::vector<Texture2D*>* textureThumbs;
nlohmann::json* texture_database;



static void AddNewLayer() {
	TextureLayer layer;
	layer.name.reserve(256);
	layer.texture = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
	textureLayers.push_back(layer);
}

void TextureSettingsTick() {
	if (IsTextureThumnsLoaded()) {
		texture_database = GetTextureDatabase();
		textureThumbs = GetTextures();
	}
}

void SetupTextureSettings(bool* reqRefresh, float* textScale)
{
	for(int i=0;i<NUM_TEXTURE_LAYERS;i++)
		AddNewLayer();
	
	glGenBuffers(1, &diffuseUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, diffuseUBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	reqrfrsh = reqRefresh;
	textureScale = textScale;
	if (diffuse)
		delete diffuse;
	diffuse = new Texture2D(GetExecutableDir() + "\\Data\\textures\\white.png");
}

static void LoadUpTexture(std::string path) {
	if (path.size() <= 3)
		return;
	textureLayers[cTexID].texture = new Texture2D(path);
	cTexID++;
	ImGui::CloseCurrentPopup();
}

void ShowOpenTextureModal() {
	if (ImGui::BeginPopupModal("Open Image"))
	{
		if (ImGui::Button("Close"))
			ImGui::CloseCurrentPopup();

		if (ImGui::Button("Open From File")) {
			std::string textureFilePath = ShowOpenFileDialog((wchar_t*)L".png\0");
			if (textureFilePath.size() > 1) {
				LoadUpTexture(textureFilePath);
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::NewLine();


		if (!IsTextureThumnsLoaded()) {
			if (ImGui::Button("Load Texture Data")) {
				LoadTextureThumbs();
				texture_database = GetTextureDatabase();
				textureThumbs = GetTextures();
			}
		}
		else {
			int tp = 0;
			int cop = 0;
			for (auto it = (*texture_database).begin(); it != (*texture_database).end();) {
				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++; 
				if (it == (*texture_database).end())
					break;
				ImGui::SameLine();

				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++;
				if (it == (*texture_database).end())
					break;
				ImGui::SameLine();

				if (it.value()["downloaded"]) {
					it.value()["key"] = it.key();
					ImGui::BeginChild(it.key().c_str(), ImVec2(200, 200));
					if (ImGui::ImageButton((ImTextureID)(*textureThumbs)[tp]->GetRendererID(), ImVec2(150, 150))) {
						LoadUpTexture(it.value()["downloaddata"]["dpath"]);
					}
					ImGui::Text(std::string(it.value()["name"]).c_str());
					ImGui::EndChild();
					cop++;
				}
				it++;
				tp++;
				if (cop == 3) {
					ImGui::NewLine();
					cop = 0;
				}
			}
		}


		ImGui::EndPopup();
	}
}

void ShowTextureSettings(bool* pOpen)
{
	ImGui::Begin("Texture Settings", pOpen);
	ShowOpenTextureModal();
	uint32_t id = diffuse ? diffuse->GetRendererID() : 0;
	//ImGui::Image((ImTextureID)(id), ImVec2(200, 200));

	int i = 0;
	for (auto& it = textureLayers.begin(); it != textureLayers.end();it++) {
		ImGui::Separator();
		if (ImGui::ImageButton((ImTextureID)it->texture->GetRendererID(), ImVec2(30, 30))) {
			cTexID = i;
			ImGui::OpenPopup("Open Image");
		}
		ImGui::SameLine();
		ImGui::InputText(("##TLayerName" + std::to_string(i)).c_str(), it->name.data(), 256);
		ImGui::DragFloat(("Min##TLayerHeightMin" + std::to_string(i)).c_str(), &it->heightMin, 0.01f);
		ImGui::DragFloat(("Max##TLayerHeightMax" + std::to_string(i)).c_str(), &it->heightMax, 0.01f);
		ImGui::DragFloat(("Scale##TLayerTScale" + std::to_string(i)).c_str(), &it->textureScale, 0.01f);
		/*
		if (ImGui::Button("Delete")) {
			textureLayers.erase(it);
			break;
		}
		*/
		ImGui::Separator();
		i++;
	}

	/*
	if (textureLayers.size() <= 8) {
		if (ImGui::Button("Add Layer")) {
			AddNewLayer();
		}
	}
	*/

	ImGui::DragFloat("Texture Scale", textureScale, 0.1f);


	ImGui::End();
} 

uint32_t UpdateDiffuseTexturesUBO(uint32_t shaderID, std::string diffuseUBOName) {
	for (int i = 0; i < NUM_TEXTURE_LAYERS; i++) {
		textureLayers[i].texture->Bind(i);
		GLuint uUniform = glGetUniformLocation(shaderID, (diffuseUBOName + "[" + std::to_string(i) + "]").c_str());
		glUniform1i(uUniform, i);
		uUniform = glGetUniformLocation(shaderID, (diffuseUBOName + "Heights[" + std::to_string(i) + "]").c_str());
		glUniform3f(uUniform, textureLayers[i].heightMin, textureLayers[i].heightMax, textureLayers[i].textureScale);
	}
	return diffuseUBO;
}

Texture2D* GetCurrentDiffuseTexture()
{ 
	return diffuse;
}
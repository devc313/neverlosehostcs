#pragma once
#include "..\..\includes.hpp"

class chams : public singleton<chams>
{
public:
	void on_dme();
	void on_sceneend();
private:
	IMaterial* create_material(bool lit, const std::string& material_data);

};


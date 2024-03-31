#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"
struct m_indicator
{
	std::string m_text;
	Color m_color;

	m_indicator(const char* text, Color color) :
		m_text(text), m_color(color)
	{

	}
	m_indicator(std::string text, Color color) :
		m_text(text), m_color(color)
	{

	}

};
class otheresp : public singleton< otheresp >
{
public:
	struct C_HitMatrixEntry {
		int ent_index;
		ModelRenderInfo_t info;
		DrawModelState_t state;
		matrix3x4_t pBoneToWorld[128] = {};
		float time;
		matrix3x4_t model_to_world;
	} m_hit_matrix;

	std::vector<C_HitMatrixEntry> m_Hitmatrix;

	void penetration_reticle();
	void add_matrix(player_t* player, matrix3x4_t* bones);
    void spread_crosshair(LPDIRECT3DDEVICE9 device);
	void automatic_peek_indicator();

	struct Hitmarker
	{
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::White;
		Vector point = ZERO;
	} hitmarker;

	struct Damage_marker
	{
		Vector position = ZERO;
		float hurt_time = FLT_MIN;
		Color hurt_color = Color::White;
		int damage = -1;
		int hitgroup = -1;

		void reset()
		{
			position.Zero();
			hurt_time = FLT_MIN;
			hurt_color = Color::White;
			damage = -1;
			hitgroup = -1;
		}
	} damage_marker[65];
	std::vector<m_indicator> m_indicators;
};
// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\features\visuals\GrenadePrediction.h"
#include "..\..\features\misc\fakelag.h"
#include "..\..\features\lagcompensation\local_animations.h"

using OverrideView_t = void(__stdcall*)(CViewSetup*);

void thirdperson(bool fakeducking);

void __stdcall hooks::hooked_overrideview(CViewSetup* viewsetup)
{
	static auto original_fn = clientmode_hook->get_func_address <OverrideView_t> (18);
	globals.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!viewsetup)
		return original_fn(viewsetup);

	if (globals.local())
	{
		static auto fakeducking = false;

		if (!fakeducking && globals.g.fakeducking)
			fakeducking = true;
		else if (fakeducking && !globals.g.fakeducking && (!globals.local()->get_animation_state()->m_fDuckAmount || globals.local()->get_animation_state()->m_fDuckAmount == 1.0f)) //-V550
			fakeducking = false;

		if (!globals.local()->is_alive()) //-V807
			fakeducking = false;

		auto weapon = globals.local()->m_hActiveWeapon().Get();

		if (weapon)
		{
			if (!globals.local()->m_bIsScoped() && g_cfg.player.enable)
				viewsetup->fov += g_cfg.esp.fov;
			else if (g_cfg.esp.removals[REMOVALS_ZOOM] && g_cfg.player.enable)
			{
				if (weapon->m_zoomLevel() == 1)
					viewsetup->fov = 90.0f + (float)g_cfg.esp.fov;
				else
					viewsetup->fov += (float)g_cfg.esp.fov;
			}
		}
		else if (g_cfg.player.enable)
			viewsetup->fov += g_cfg.esp.fov;

		if (weapon)
		{
			auto viewmodel = (entity_t*)m_entitylist()->GetClientEntityFromHandle(globals.local()->m_hViewModel());

			if (viewmodel)
			{
				auto eyeAng = viewsetup->angles;
				eyeAng.z -= (float)g_cfg.esp.viewmodel_roll;

				viewmodel->set_abs_angles(eyeAng);
			}

			if (weapon->is_grenade() && g_cfg.esp.grenade_prediction && g_cfg.player.enable)
				GrenadePrediction::get().View(viewsetup, weapon);
		}

		if (g_cfg.player.enable && (g_cfg.misc.thirdperson_toggle.key > KEY_NONE && g_cfg.misc.thirdperson_toggle.key < KEY_MAX || g_cfg.misc.thirdperson_when_spectating))
			thirdperson(fakeducking);
		else
		{
			globals.g.in_thirdperson = false;
			m_input()->bCameraInThirdPerson = false;
		}

		original_fn(viewsetup);

		if (fakeducking)
		{
			viewsetup->origin = globals.local()->GetAbsOrigin() + Vector(0.0f, 0.0f, m_gamemovement()->GetPlayerViewOffset(false).z + 0.064f);

			if (m_input()->bCameraInThirdPerson)
			{
				auto camera_angles = Vector(m_input()->vecCameraOffset.x, m_input()->vecCameraOffset.y, 0.0f); //-V807
				auto camera_forward = ZERO;

				math::angle_vectors(camera_angles, camera_forward);
				math::VectorMA(viewsetup->origin, -m_input()->vecCameraOffset.z, camera_forward, viewsetup->origin);
			}
		}
	}
	else
		return original_fn(viewsetup);
}

void thirdperson(bool fakeducking)
{
	static auto current_fraction = 0.0f;
	static auto in_thirdperson = false;

	if (!globals.local()->is_alive())
	{
		if (!g_cfg.misc.thirdperson_when_spectating)
		{
			globals.local()->m_iObserverMode() = OBS_MODE_IN_EYE;
		}
		else
		{
			globals.local()->m_iObserverMode() = OBS_MODE_CHASE;
			globals.g.in_thirdperson = false;
		}
	}


	if (!in_thirdperson && globals.g.in_thirdperson)
	{
		current_fraction = 0.0f;
		in_thirdperson = true;
	}
	else if (in_thirdperson && !globals.g.in_thirdperson)
		in_thirdperson = false;

	if (globals.local()->is_alive() && in_thirdperson) //-V807
	{
		auto distance = (float)g_cfg.misc.thirdperson_distance;

		Vector angles;
		m_engine()->GetViewAngles(angles);

		Vector inverse_angles;
		m_engine()->GetViewAngles(inverse_angles);

		inverse_angles.z = distance;

		Vector forward, right, up;
		math::angle_vectors(inverse_angles, &forward, &right, &up);

		Ray_t ray;
		CTraceFilterWorldAndPropsOnly filter;
		trace_t trace;

		auto eye_pos = fakeducking ? globals.local()->GetAbsOrigin() + m_gamemovement()->GetPlayerViewOffset(false) : globals.local()->GetAbsOrigin() + globals.local()->m_vecViewOffset();
		auto offset = eye_pos + forward * -distance + right + up;

		ray.Init(eye_pos, offset, Vector(-16.0f, -16.0f, -16.0f), Vector(16.0f, 16.0f, 16.0f));
		m_trace()->TraceRay(ray, MASK_SHOT_HULL, &filter, &trace);

		if (current_fraction > trace.fraction)
			current_fraction = trace.fraction;
		else if (current_fraction > 0.9999f)
			current_fraction = 1.0f;

		current_fraction = math::interpolate(current_fraction, trace.fraction, m_globals()->m_frametime * 10.0f);
		angles.z = distance * current_fraction;

		m_input()->bCameraInThirdPerson = current_fraction > 0.1f;
		m_input()->vecCameraOffset = angles;
	}
	else if (m_input()->bCameraInThirdPerson)
	{
		globals.g.in_thirdperson = false;
		m_input()->bCameraInThirdPerson = false;
	}
}
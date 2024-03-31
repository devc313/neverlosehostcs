// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "antiaim.h"
#include "knifebot.h"
#include "zeusbot.h"
#include "..\misc\fakelag.h"
#include "..\misc\prediction_system.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"

void antiaim::create_move(CUserCmd* m_pcmd)
{
	auto velocity = globals.local()->m_vecVelocity().Length(); //-V807

	type = ANTIAIM_STAND;

	if (g_cfg.antiaim.antiaim_type)
		type = ANTIAIM_LEGIT;

	if (!g_cfg.ragebot.enable && type != ANTIAIM_LEGIT)
		return;

	if (condition(m_pcmd))
		return;

	if ((m_pcmd->m_buttons & IN_USE))
		return;

	if ((type == ANTIAIM_LEGIT ? g_cfg.antiaim.desync : g_cfg.antiaim.type[type].desync) && (type == ANTIAIM_LEGIT ? !g_cfg.antiaim.legit_lby_type : !g_cfg.antiaim.lby_type) && !g_cfg.misc.fast_stop && (!globals.g.weapon->is_grenade() || g_cfg.esp.on_click && !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2)) && engineprediction::get().backup_data.velocity.Length2D() <= 20.0f) //-V648
	{
		auto speed = 1.01f;

		

		if (m_pcmd->m_buttons & IN_DUCK || globals.g.fakeducking)
			speed *= 2.94117647f;

		static auto switch_move = false;

		if (switch_move)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;

		switch_move = !switch_move;
	}

	if (type != ANTIAIM_LEGIT)
		m_pcmd->m_viewangles.x = get_pitch(m_pcmd);

	m_pcmd->m_viewangles.y = get_yaw(m_pcmd);
}

float antiaim::get_pitch(CUserCmd* m_pcmd)
{
	static auto invert_jitter = false;
	static auto should_invert = false;

	if (globals.send_packet)
		should_invert = true;
	else if (!globals.send_packet && should_invert) //-V560
	{
		should_invert = false;
		invert_jitter = !invert_jitter;
	}

	auto pitch = m_pcmd->m_viewangles.x;

	switch (g_cfg.antiaim.type[type].pitch)
	{
	case 1:
		pitch = 89.0f;
		break;
	case 2:
		pitch = -89.0f;
		break;
	case 3:
		pitch = 0.0f;
		break;
	}

	return pitch;
}

float antiaim::get_yaw(CUserCmd* m_pcmd)
{
	static auto invert_jitter = false;
	static auto should_invert = false;

	if (globals.send_packet)
		should_invert = true;
	else if (!globals.send_packet && should_invert) //-V560
	{
		should_invert = false;
		invert_jitter = !invert_jitter;
	}

	auto max_desync_delta = globals.local()->get_max_desync_delta(); //-V807

	auto yaw = 0.0f;
	auto lby_type = 0;

	if (type == ANTIAIM_LEGIT)
	{
		yaw = m_pcmd->m_viewangles.y;

		if (!g_cfg.antiaim.desync)
			return yaw;


        if (g_cfg.antiaim.desync == 1)
			flip = key_binds::get().get_key_bind_state(16);
		else if (g_cfg.antiaim.desync == 2)
			flip = invert_jitter;

		desync_angle = max_desync_delta;

		if (g_cfg.antiaim.legit_lby_type && globals.local()->m_vecVelocity().Length() < 5.0f && globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND)
			desync_angle *= 2.0f;

		if (flip)
		{
			desync_angle = -desync_angle;
			max_desync_delta = -max_desync_delta;
		}

		yaw -= desync_angle;
		lby_type = g_cfg.antiaim.legit_lby_type;
	}
	else
	{
		if (manual_side == SIDE_NONE)
		{

		}
		else
			final_manual_side = manual_side;

		auto base_angle = m_pcmd->m_viewangles.y + 180.0f;

		if (final_manual_side == SIDE_LEFT)
			base_angle -= 90.0f;
		if (final_manual_side == SIDE_RIGHT)
			base_angle += 90.0f;

		if (g_cfg.antiaim.type[type].base_angle && manual_side == SIDE_NONE)
			base_angle = at_targets();

		if (g_cfg.antiaim.type[type].desync != 2 && (g_cfg.antiaim.flip_desync.key <= KEY_NONE || g_cfg.antiaim.flip_desync.key >= KEY_MAX))
		{
			if (final_manual_side == SIDE_LEFT)
				flip = true;
			else if (final_manual_side == SIDE_RIGHT)
				flip = false;
		}
		else if (g_cfg.antiaim.type[type].desync == 1)
			flip = key_binds::get().get_key_bind_state(16);

		auto yaw_angle = 0.0f;

		switch (g_cfg.antiaim.type[type].yaw)
		{
		case 1:
			yaw_angle = invert_jitter ? (float)g_cfg.antiaim.type[type].range * -0.5f : (float)g_cfg.antiaim.type[type].range * 0.5f;
			break;
		case 2:
		{
			if (flip)
			{
				auto start_angle = (float)g_cfg.antiaim.type[type].range * 0.5f;
				auto end_angle = (float)g_cfg.antiaim.type[type].range * -0.5f;

				static auto angle = start_angle;

				auto angle_add_amount = (float)g_cfg.antiaim.type[type].speed * 0.5f;

				if (angle - angle_add_amount >= end_angle)
					angle -= angle_add_amount;
				else
					angle = start_angle;

				yaw_angle = angle;
			}
			else
			{
				auto start_angle = (float)g_cfg.antiaim.type[type].range * -0.5f;
				auto end_angle = (float)g_cfg.antiaim.type[type].range * 0.5f;

				static auto angle = start_angle;

				auto angle_add_amount = (float)g_cfg.antiaim.type[type].speed * 0.5f;

				if (angle + angle_add_amount <= end_angle)
					angle += angle_add_amount;
				else
					angle = start_angle;

				yaw_angle = angle;
			}
		}
		break;
		}

		desync_angle = 0.0f;

		if (g_cfg.antiaim.type[type].desync)
		{
			if (g_cfg.antiaim.type[type].desync == 2)
				flip = invert_jitter;

			auto desync_delta = max_desync_delta;

			if (type == ANTIAIM_STAND && g_cfg.antiaim.lby_type)
				desync_delta *= 2.0f;
			else
			{
				if (!flip)
					desync_delta = min(desync_delta, (float)g_cfg.antiaim.type[type].desync_range);
				else
					desync_delta = min(desync_delta, (float)g_cfg.antiaim.type[type].inverted_desync_range);
			}

			if (!flip)
			{
				desync_delta = -desync_delta;
				max_desync_delta = -max_desync_delta;
			}

			base_angle -= desync_delta;

			if (type != ANTIAIM_STAND && type != ANTIAIM_LEGIT || !g_cfg.antiaim.lby_type)
			{
				if (!flip)
					base_angle += desync_delta * (float)g_cfg.antiaim.type[type].body_lean * 0.01f;
				else
					base_angle += desync_delta * (float)g_cfg.antiaim.type[type].inverted_body_lean * 0.01f;
			}

			desync_angle = desync_delta;
		}

		yaw = base_angle + yaw_angle;

		if (!desync_angle) //-V550
			return yaw;

		lby_type = g_cfg.antiaim.lby_type;
	}

	static auto sway_counter = 0;
	static auto force_choke = false;

	if (should_break_lby(m_pcmd, lby_type))
	{
		auto speed = 1.01f;

		if (m_pcmd->m_buttons & IN_DUCK || globals.g.fakeducking)
			speed *= 2.94117647f;

		static auto switch_move = false;

		if (switch_move)
			m_pcmd->m_sidemove += speed;
		else
			m_pcmd->m_sidemove -= speed;

		switch_move = !switch_move;

		if (lby_type != 2 || sway_counter > 3)
		{
			if (desync_angle > 0.0f)
				yaw -= 179.0f;
			else
				yaw += 179.0f;
		}

		if (sway_counter < 8)
			++sway_counter;
		else
			sway_counter = 0;

		breaking_lby = true;
		force_choke = true;
		globals.send_packet = false;

		return yaw;
	}
	else if (force_choke)
	{
		force_choke = false;
		globals.send_packet = false;

		return yaw;
	}
	else if (globals.send_packet)
		yaw += desync_angle;

	return yaw;
}

bool antiaim::condition(CUserCmd* m_pcmd, bool dynamic_check)
{
	if (!m_pcmd)
		return true;

	if (!globals.available())
		return true;

	if (!g_cfg.antiaim.enable)
		return true;

	if (!globals.local()->is_alive()) //-V807
		return true;

	if (globals.local()->m_bGunGameImmunity() || globals.local()->m_fFlags() & FL_FROZEN)
		return true;

	if (globals.local()->get_move_type() == MOVETYPE_NOCLIP || globals.local()->get_move_type() == MOVETYPE_LADDER)
		return true;

	if (globals.g.aimbot_working)
		return true;

	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return true;

	if (m_pcmd->m_buttons & IN_ATTACK && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !weapon->is_non_aim())
		return true;

	auto revolver_shoot = weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !globals.g.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);

	if (revolver_shoot)
		return true;

	if ((m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2) && weapon->is_knife())
		return true;

	if (dynamic_check && freeze_check)
		return true;

	if (dynamic_check && m_pcmd->m_buttons & IN_USE && !g_cfg.antiaim.antiaim_type)
		type = ANTIAIM_LEGIT;

	if (dynamic_check && weapon->is_grenade() && weapon->m_fThrowTime())
		return true;

	return false;
}

bool antiaim::should_break_lby(CUserCmd* m_pcmd, int lby_type)
{
	if (!lby_type)
		return false;

	if (globals.g.fakeducking && m_clientstate()->iChokedCommands > 12)
		return false;

	if (!globals.g.fakeducking && m_clientstate()->iChokedCommands > 14)
	{
		globals.send_packet = true;
		fakelag::get().started_peeking = false;
	}

	auto animstate = globals.local()->get_animation_state(); //-V807

	if (!animstate)
		return false;

	if (animstate->m_velocity > 0.1f || fabs(animstate->flUpVelocity) > 100.0f)
		globals.g.next_lby_update = TICKS_TO_TIME(globals.g.fixed_tickbase + 14);
	else
	{
		if (TICKS_TO_TIME(globals.g.fixed_tickbase) > globals.g.next_lby_update)
		{
			globals.g.next_lby_update = 0.0f;
			return true;
		}
	}

	return false;
}

float antiaim::at_targets()
{
	player_t* target = nullptr;
	auto best_fov = FLT_MAX;

	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e->valid(true))
			continue;

		auto weapon = e->m_hActiveWeapon().Get();

		if (!weapon)
			continue;

		if (weapon->is_non_aim())
			continue;

		Vector angles;
		m_engine()->GetViewAngles(angles);

		auto fov = math::get_fov(angles, math::calculate_angle(globals.g.eye_pos, e->GetAbsOrigin()));

		if (fov < best_fov)
		{
			best_fov = fov;
			target = e;
		}
	}

	auto angle = 180.0f;

	if (manual_side == SIDE_LEFT)
		angle = 90.0f;
	else if (manual_side == SIDE_RIGHT)
		angle = -90.0f;

	if (!target)
		return globals.get_command()->m_viewangles.y + angle;

	return math::calculate_angle(globals.g.eye_pos, target->GetAbsOrigin()).y + angle;
}

void antiaim::freestanding(CUserCmd* m_pcmd)
{
	float Right, Left;
	Vector src3D, dst3D, forward, right, up;
	trace_t tr;
	Ray_t ray_right, ray_left;
	CTraceFilter filter;

	Vector engineViewAngles;
	m_engine()->GetViewAngles(engineViewAngles);
	engineViewAngles.x = 0.0f;

	math::angle_vectors(engineViewAngles, &forward, &right, &up);

	filter.pSkip = globals.local();
	src3D = globals.g.eye_pos;
	dst3D = src3D + forward * 100.0f;

	ray_right.Init(src3D + right * 35.0f, dst3D + right * 35.0f);

	globals.g.autowalling = true;
	m_trace()->TraceRay(ray_right, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	globals.g.autowalling = false;

	Right = (tr.endpos - tr.startpos).Length();

	ray_left.Init(src3D - right * 35.0f, dst3D - right * 35.0f);

	globals.g.autowalling = true;
	m_trace()->TraceRay(ray_left, MASK_SOLID & ~CONTENTS_MONSTER, &filter, &tr);
	globals.g.autowalling = false;

	Left = (tr.endpos - tr.startpos).Length();

	static auto left_ticks = 0;
	static auto right_ticks = 0;
	static auto back_ticks = 0;

	if (Right - Left > 20.0f)
		left_ticks++;
	else
		left_ticks = 0;

	if (Left - Right > 20.0f)
		right_ticks++;
	else
		right_ticks = 0;

	if (fabs(Right - Left) <= 20.0f)
		back_ticks++;
	else
		back_ticks = 0;

	if (right_ticks > 10)
		final_manual_side = SIDE_RIGHT;
	else if (left_ticks > 10)
		final_manual_side = SIDE_LEFT;
	else if (back_ticks > 10)
		final_manual_side = SIDE_BACK;
}
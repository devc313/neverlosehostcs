// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "misc.h"
#include "fakelag.h"
#include "..\ragebot\aim.h"
#include "..\visuals\world_esp.h"
#include "prediction_system.h"
#include "logs.h"
#include "..\..\menu\menu.h"
#include "..\..\utils\render\NewRender.h"

void misc::watermark()
{	
	std::string szWatermark = crypt_str("Neverlose.host | ") + globals.username + crypt_str(" |");

	if (!g_cfg.menu.watermark)
		return;

	INetChannelInfo* m_NetChannelInfo = m_engine()->GetNetChannelInfo();
	if (m_engine()->IsConnected())
	{
		if (m_NetChannelInfo)
		{
			if (m_NetChannelInfo->IsLoopback())
				szWatermark += crypt_str(" local server |");
			else
				szWatermark += " " + (std::string)(m_NetChannelInfo->GetAddress()) + " | ";

			szWatermark += (std::string)(" ") + std::to_string(globals.g.ping) + (std::string)("ms |");
		}
	}
	else
		szWatermark += crypt_str(" unconnected |");

	int nScreenSizeX, nScreenSizeY;
	m_engine()->GetScreenSize(nScreenSizeX, nScreenSizeY);

	int nTextLength = c_menu::get().g_pinfoFont->CalcTextSizeA(16.0f, FLT_MAX, NULL, szWatermark.c_str()).x + 57;
	szWatermark += crypt_str(" fps: ") + std::to_string((int)(1.0f / ImGui::GetIO().DeltaTime));

	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(nScreenSizeX - nTextLength, 11), ImVec2(nScreenSizeX - 10, 30), ImColor(15, 15, 15, 255));

	ImGui::PushFont(c_menu::get().g_pinfoFont);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(nScreenSizeX - nTextLength + 7, 12), ImColor(255, 255, 255, 255), szWatermark.c_str());
	ImGui::PopFont();
}

void misc::forceRelayCluster() noexcept
{
	if (g_cfg.misc.forceRelayCluster == 0)
		return;

	std::string dataCentersList[] = { "", "syd", "vie", "gru", "scl", "dxb", "par", "fra", "hkg",
	"maa", "bom", "tyo", "lux", "ams", "limc", "man", "waw", "sgp", "jnb",
	"mad", "sto", "lhr", "atl", "eat", "ord", "lax", "mwh", "okc", "sea", "iad" };

	*memory.relayCluster = dataCentersList[g_cfg.misc.forceRelayCluster];
}

void misc::NoDuck(CUserCmd* cmd)
{
	if (!g_cfg.misc.noduck)
		return;

	cmd->m_buttons |= IN_BULLRUSH;
}

void misc::ChatSpamer()
{
	if (!g_cfg.misc.chat)
		return;

	static std::string chatspam[] = 
	{ 
		crypt_str("Neverlose.Host - t.me/neverlosehost"),
		crypt_str("Why you pasting LW, we do it )"),
		crypt_str("Neverlose.Host - nitro & UwuTarCh1k"),
		crypt_str("Best cheat ever - neverlose.host > all"),
	};

	static auto lastspammed = 0;

	if (GetTickCount64() - lastspammed > 800)
	{
		lastspammed = GetTickCount64();

		srand(m_globals()->m_tickcount);
		std::string msg = crypt_str("say ") + chatspam[rand() % 4];

		m_engine()->ExecuteClientCmd(msg.c_str());
	}
}

void misc::AutoCrouch(CUserCmd* cmd)
{
	if (fakelag::get().condition)
	{
		globals.g.fakeducking = false;
		return;
	}

	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
	{
		globals.g.fakeducking = false;
		return;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		globals.g.fakeducking = false;
		return;
	}

	if (!key_binds::get().get_key_bind_state(20))
	{
		globals.g.fakeducking = false;
		return;
	}

	if (!globals.g.fakeducking && m_clientstate()->iChokedCommands != 7)
		return;

	if (m_clientstate()->iChokedCommands >= 7)
		cmd->m_buttons |= IN_DUCK;
	else
		cmd->m_buttons &= ~IN_DUCK;

	globals.g.fakeducking = true;
}

void misc::SlideWalk(CUserCmd* cmd)
{
	if (!globals.local()->is_alive()) //-V807
		return;

	if (globals.local()->get_move_type() == MOVETYPE_LADDER)
		return;

	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	if (antiaim::get().condition(cmd, true) && g_cfg.misc.slidewalk)
	{
		if (cmd->m_forwardmove > 0.0f)
		{
			cmd->m_buttons |= IN_BACK;
			cmd->m_buttons &= ~IN_FORWARD;
		}
		else if (cmd->m_forwardmove < 0.0f)
		{
			cmd->m_buttons |= IN_FORWARD;
			cmd->m_buttons &= ~IN_BACK;
		}

		if (cmd->m_sidemove > 0.0f)
		{
			cmd->m_buttons |= IN_MOVELEFT;
			cmd->m_buttons &= ~IN_MOVERIGHT;
		}
		else if (cmd->m_sidemove < 0.0f)
		{
			cmd->m_buttons |= IN_MOVERIGHT;
			cmd->m_buttons &= ~IN_MOVELEFT;
		}
	}
	else
	{
		auto buttons = cmd->m_buttons & ~(IN_MOVERIGHT | IN_MOVELEFT | IN_BACK | IN_FORWARD);

		if (g_cfg.misc.slidewalk)
		{
			if (cmd->m_forwardmove <= 0.0f)
				buttons |= IN_BACK;
			else
				buttons |= IN_FORWARD;

			if (cmd->m_sidemove > 0.0f)
				goto LABEL_15;
			else if (cmd->m_sidemove >= 0.0f)
				goto LABEL_18;

			goto LABEL_17;
		}
		else
			goto LABEL_18;

		if (cmd->m_forwardmove <= 0.0f) //-V779
			buttons |= IN_FORWARD;
		else
			buttons |= IN_BACK;

		if (cmd->m_sidemove > 0.0f)
		{
		LABEL_17:
			buttons |= IN_MOVELEFT;
			goto LABEL_18;
		}

		if (cmd->m_sidemove < 0.0f)
			LABEL_15:

		buttons |= IN_MOVERIGHT;

	LABEL_18:
		cmd->m_buttons = buttons;
	}
}

void misc::automatic_peek(CUserCmd* cmd, float wish_yaw)
{
	if (!globals.g.weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
	{
		if (globals.g.start_position.IsZero())
		{
			globals.g.start_position = globals.local()->GetAbsOrigin();

			if (!(engineprediction::get().backup_data.flags & FL_ONGROUND))
			{
				Ray_t ray;
				CTraceFilterWorldAndPropsOnly filter;
				CGameTrace trace;

				ray.Init(globals.g.start_position, globals.g.start_position - Vector(0.0f, 0.0f, 1000.0f));
				m_trace()->TraceRay(ray, MASK_SOLID, &filter, &trace);
				
				if (trace.fraction < 1.0f)
					globals.g.start_position = trace.endpos + Vector(0.0f, 0.0f, 2.0f);
			}
		}
		else
		{
			auto revolver_shoot = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !globals.g.revolver_working && (cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2);

			if (cmd->m_buttons & IN_ATTACK && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || revolver_shoot)
				globals.g.fired_shot = true;

			if (globals.g.fired_shot)
			{
				auto current_position = globals.local()->GetAbsOrigin();
				auto difference = current_position - globals.g.start_position;

				if (difference.Length2D() > 5.0f)
				{
					auto velocity = Vector(difference.x * cos(wish_yaw / 180.0f * M_PI) + difference.y * sin(wish_yaw / 180.0f * M_PI), difference.y * cos(wish_yaw / 180.0f * M_PI) - difference.x * sin(wish_yaw / 180.0f * M_PI), difference.z);

					cmd->m_forwardmove = -velocity.x * 20.0f;
					cmd->m_sidemove = velocity.y * 20.0f;
				}
				else
				{
					globals.g.fired_shot = false;
					globals.g.start_position.Zero();
				}
			}
		}
	}
	else
	{
		globals.g.fired_shot = false;
		globals.g.start_position.Zero();
	}
}

void misc::ViewModel()
{
	if (g_cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)g_cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV) //-V550
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}
	
	if (g_cfg.esp.viewmodel_x)
	{
		auto viewX = (float)g_cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x")); //-V807

		if (viewXcvar->GetFloat() != viewX) //-V550
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (g_cfg.esp.viewmodel_y)
	{
		auto viewY = (float)g_cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY) //-V550
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (g_cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)g_cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ) //-V550
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}

void misc::FullBright()
{		
	if (!g_cfg.player.enable)
		return;

	static auto mat_fullbright = m_cvar()->FindVar(crypt_str("mat_fullbright"));

	if (mat_fullbright->GetBool() != g_cfg.esp.bright)
		mat_fullbright->SetValue(g_cfg.esp.bright);
}

void misc::PovArrows(player_t* e, Color color)
{
	auto isOnScreen = [](Vector origin, Vector& screen) -> bool
	{
		if (!math::world_to_screen(origin, screen))
			return false;

		static int iScreenWidth, iScreenHeight;
		m_engine()->GetScreenSize(iScreenWidth, iScreenHeight);

		auto xOk = iScreenWidth > screen.x;
		auto yOk = iScreenHeight > screen.y;

		return xOk && yOk;
	};

	Vector screenPos;

	if (isOnScreen(e->GetAbsOrigin(), screenPos))
		return;

	Vector viewAngles;
	m_engine()->GetViewAngles(viewAngles);

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto screenCenter = Vector2D(width * 0.5f, height * 0.5f);
	auto angleYawRad = DEG2RAD(viewAngles.y - math::calculate_angle(globals.g.eye_pos, e->GetAbsOrigin()).y - 90.0f);

	auto radius = g_cfg.player.distance;
	auto size = g_cfg.player.size;

	auto newPointX = screenCenter.x + ((((width - (size * 3)) * 0.5f) * (radius / 100.0f)) * cos(angleYawRad)) + (int)(6.0f * (((float)size - 4.0f) / 16.0f));
	auto newPointY = screenCenter.y + ((((height - (size * 3)) * 0.5f) * (radius / 100.0f)) * sin(angleYawRad));

	std::array <Vector2D, 3> points
	{
		Vector2D(newPointX - size, newPointY - size),
		Vector2D(newPointX + size, newPointY),
		Vector2D(newPointX - size, newPointY + size)
	};

	math::rotate_triangle(points, viewAngles.y - math::calculate_angle(globals.g.eye_pos, e->GetAbsOrigin()).y - 90.0f);
	g_Render->TriangleFilled(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), math::clamp(color.a(), 0, 125)));
	g_Render->Triangle(points.at(0).x, points.at(0).y, points.at(1).x, points.at(1).y, points.at(2).x, points.at(2).y, Color(color.r(), color.g(), color.b(), color.a()));
}

void misc::zeus_range()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.taser_range)
		return;

	if (!m_input()->bCameraInThirdPerson)
		return;

	if (!globals.local()->is_alive())  //-V807
		return;

	auto weapon = globals.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	if (weapon->m_iItemDefinitionIndex() != WEAPON_TASER)
		return;

	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	render::get().Draw3DCircle(globals.local()->get_shoot_position(), weapon_info->flRange, Color(255, 255, 255));
}

void misc::NightmodeFix()
{
	static auto in_game = false;

	if (m_engine()->IsInGame() && !in_game)
	{
		in_game = true;

		globals.g.change_materials = true;
		worldesp::get().changed = true;

		static auto skybox = m_cvar()->FindVar(crypt_str("sv_skyname"));
		worldesp::get().backup_skybox = skybox->GetString();
		return;
	}
	else if (!m_engine()->IsInGame() && in_game)
		in_game = false;

	static auto player_enable = g_cfg.player.enable;

	if (player_enable != g_cfg.player.enable)
	{
		player_enable = g_cfg.player.enable;
		globals.g.change_materials = true;
		return;
	}

	static auto setting = g_cfg.esp.nightmode;

	if (setting != g_cfg.esp.nightmode)
	{
		setting = g_cfg.esp.nightmode;
		globals.g.change_materials = true;
		return;
	}

	static auto setting_world = g_cfg.esp.world_color;

	if (setting_world != g_cfg.esp.world_color)
	{
		setting_world = g_cfg.esp.world_color;
		globals.g.change_materials = true;
		return;
	}

	static auto setting_props = g_cfg.esp.props_color;

	if (setting_props != g_cfg.esp.props_color)
	{
		setting_props = g_cfg.esp.props_color;
		globals.g.change_materials = true;
	}
}

void misc::desync_arrows()
{
	if (!globals.local()->is_alive())
		return;

	if (!g_cfg.ragebot.enable)
		return;

	if (!g_cfg.antiaim.enable)
		return;

	if ((g_cfg.antiaim.manual_back.key <= KEY_NONE || g_cfg.antiaim.manual_back.key >= KEY_MAX) && (g_cfg.antiaim.manual_left.key <= KEY_NONE || g_cfg.antiaim.manual_left.key >= KEY_MAX) && (g_cfg.antiaim.manual_right.key <= KEY_NONE || g_cfg.antiaim.manual_right.key >= KEY_MAX))
		antiaim::get().manual_side = SIDE_NONE;

	if (!g_cfg.antiaim.flip_indicator)
		return;

	if (!globals.g.scoped == false)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto color = g_cfg.antiaim.flip_indicator_color;

	if (antiaim::get().manual_side == SIDE_LEFT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), color);
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}
	else if (antiaim::get().manual_side == SIDE_RIGHT)
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), color);
	}
	else
	{
		render::get().triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), Color(55, 55, 55, 189));
		render::get().triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), Color(55, 55, 55, 189));
	}

	if (g_cfg.antiaim.desync == 2)
	{
		if (antiaim::get().flip == false)
		{
			render::get().rect(width / 2 - 10, height / 2 + 50, 20, 2, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 10, height / 2 + 50, 20, 2, Color(color));
		}
	}
	else
	{
		if (antiaim::get().flip == false)
		{
			render::get().rect(width / 2 - 50, height / 2 - 10, 2, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 - 50, height / 2 - 10, 2, 20, Color(color));
		}
		else if (antiaim::get().flip == true)
		{
			render::get().rect(width / 2 + 48, height / 2 - 10, 2, 20, Color(color.r(), color.g(), color.b(), 255));
			render::get().rect_filled(width / 2 + 48, height / 2 - 10, 2, 20, Color(color));
		}
	}
}

void misc::aimbot_hitboxes()
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.player.lag_hitbox)
		return;

	auto player = (player_t*)m_entitylist()->GetClientEntity(aim::get().last_target_index);

	if (!player)
		return;

	auto model = player->GetModel(); //-V807

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;
	
	auto hitbox_set = studio_model->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return;

	for (auto i = 0; i < hitbox_set->numhitboxes; i++)
	{
		auto hitbox = hitbox_set->pHitbox(i);

		if (!hitbox)
			continue;

		if (hitbox->radius == -1.0f) //-V550
			continue;

		auto min = ZERO;
		auto max = ZERO;

		math::vector_transform(hitbox->bbmin, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main[hitbox->bone], min);
		math::vector_transform(hitbox->bbmax, aim::get().last_target[aim::get().last_target_index].record.matrixes_data.main[hitbox->bone], max);

		m_debugoverlay()->AddCapsuleOverlay(min, max, hitbox->radius, g_cfg.player.lag_hitbox_color.r(), g_cfg.player.lag_hitbox_color.g(), g_cfg.player.lag_hitbox_color.b(), g_cfg.player.lag_hitbox_color.a(), 4.0f, 0, 1);
	}
}

void misc::ragdolls()
{
	if (!g_cfg.misc.ragdolls)
		return;

	for (auto i = 1; i <= m_entitylist()->GetHighestEntityIndex(); ++i)
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		if (client_class->m_ClassID != CCSRagdoll)
			continue;

		auto ragdoll = (ragdoll_t*)e;
		ragdoll->m_vecForce().z = 800000.0f;
	}
}

void misc::rank_reveal()
{
	if (!g_cfg.misc.rank_reveal)
		return;

	using RankReveal_t = bool(__cdecl*)(int*);
	static auto Fn = (RankReveal_t)(util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 A1 ? ? ? ? 85 C0 75 37")));

	int array[3] = 
	{
		0,
		0,
		0
	};

	Fn(array);
}

void misc::fast_stop(CUserCmd* m_pcmd)
{
	if (!g_cfg.misc.fast_stop)
		return;

	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	auto pressed_move_key = m_pcmd->m_buttons & IN_FORWARD || m_pcmd->m_buttons & IN_MOVELEFT || m_pcmd->m_buttons & IN_BACK || m_pcmd->m_buttons & IN_MOVERIGHT || m_pcmd->m_buttons & IN_JUMP;

	if (pressed_move_key)
		return;

	if (!((antiaim::get().type == ANTIAIM_LEGIT ? g_cfg.antiaim.desync : g_cfg.antiaim.type[antiaim::get().type].desync) && (antiaim::get().type == ANTIAIM_LEGIT ? !g_cfg.antiaim.legit_lby_type : !g_cfg.antiaim.lby_type) && (!globals.g.weapon->is_grenade() || g_cfg.esp.on_click & !(m_pcmd->m_buttons & IN_ATTACK) && !(m_pcmd->m_buttons & IN_ATTACK2))) || antiaim::get().condition(m_pcmd)) //-V648
	{
		auto velocity = globals.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
	}
	else
	{
		auto velocity = globals.local()->m_vecVelocity();

		if (velocity.Length2D() > 20.0f)
		{
			Vector direction;
			Vector real_view;

			math::vector_angles(velocity, direction);
			m_engine()->GetViewAngles(real_view);

			direction.y = real_view.y - direction.y;

			Vector forward;
			math::angle_vectors(direction, forward);

			static auto cl_forwardspeed = m_cvar()->FindVar(crypt_str("cl_forwardspeed"));
			static auto cl_sidespeed = m_cvar()->FindVar(crypt_str("cl_sidespeed"));

			auto negative_forward_speed = -cl_forwardspeed->GetFloat();
			auto negative_side_speed = -cl_sidespeed->GetFloat();

			auto negative_forward_direction = forward * negative_forward_speed;
			auto negative_side_direction = forward * negative_side_speed;

			m_pcmd->m_forwardmove = negative_forward_direction.x;
			m_pcmd->m_sidemove = negative_side_direction.y;
		}
		else
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
	}
}



void misc::spectators_list()
{
	if (!g_cfg.misc.spectators_list)
		return;

	const auto freq = 1.f;

	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	std::vector <std::string> spectators;

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";
	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	static float windowsize_x = 0;
	static float windowsize_y = 0;
	static float oldwindowsize_x = 300;
	static float oldwindowsize_y = 300;
	static float speed = 10;
	static bool finish = false;
	static int sub_tabs = false;
	static float alpha_menu = 0.1f;


	if (windowsize_x != oldwindowsize_x)
	{
		if (windowsize_x > oldwindowsize_x)
			windowsize_x -= speed;

		if (windowsize_x < oldwindowsize_x)
			windowsize_x += speed;
	}
	if (windowsize_y != oldwindowsize_y)
	{
		if (windowsize_y > oldwindowsize_y)
			windowsize_y -= speed;

		if (windowsize_y < oldwindowsize_y)
			windowsize_y += speed;
	}

	if (windowsize_x == oldwindowsize_x && windowsize_y == oldwindowsize_y)
		finish = true;
	else
		finish = false;

	for (int i = 1; i < m_globals()->m_maxclients; i++)
	{
		auto e = static_cast<player_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_alive())
			continue;

		if (e->IsDormant())
			continue;

		if (e->m_hObserverTarget().Get() != globals.local())
			continue;

		player_info_s player_info;
		m_engine()->GetPlayerInfo(i, &player_info);
		spect += player_info.szName;
		spect += "\n";
		specs++;
	}

	bool theme = true;

	if ((spect.size() > 0) || hooks::menu_open)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
		else
			if (alpha_menu > 0.1f)
				alpha_menu -= 0.05f;

	if ((g_cfg.misc.spectators_list))
	{
		if ((spect.size() > 0 && alpha_menu > 0.1f) || hooks::menu_open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Spectators", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();

				std::vector<std::string> keybind = { "Spectators" };
				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 25 }, ImColor(15, 15, 15));
				ImGui::PushFont(c_menu::get().g_pinfoFont);
				draw->AddText(ImVec2(p.x + 10, p.y + 7), ImColor(200, 200, 200), keybind.at(0).c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

void misc::hotkey_list()
{
	if (!g_cfg.misc.keybinds)
		return;

	const auto freq = 1.f;
	const auto real_time = m_globals()->m_realtime * freq;
	const int r = std::floor(std::sin(real_time + 0.f) * 127.f + 128.f);
	const int g = std::floor(std::sin(real_time + 2.f) * 127.f + 128.f);
	const int b = std::floor(std::sin(real_time + 4.f) * 127.f + 128.f);

	int specs = 0;
	int modes = 0;
	std::string spect = "";
	std::string mode = "";

	if (key_binds::get().get_key_bind_state(0)) {
		spect += crypt_str("Auto fire");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.legitbot.autofire_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(1)) {
		spect += crypt_str("Legit bot");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.legitbot.key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (misc::get().double_tap_key || misc::get().double_tap_enabled || misc::get().get().recharging_double_tap) {
		spect += crypt_str("DT");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.double_tap_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(3)) {
		spect += crypt_str("SP");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (globals.g.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + globals.g.current_weapon))
	{
		spect += crypt_str("Damage override");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.safe_point_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key) {
		spect += crypt_str("HS");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.antiaim.hide_shots_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(16)) {
		spect += crypt_str("Flip desync");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.antiaim.flip_desync.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(17)) {
		spect += crypt_str("Thirdperson");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.thirdperson_toggle.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(18)) {
		spect += crypt_str("Auto peek");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.automatic_peek.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(19)) {
		spect += crypt_str("Edge jump");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.edge_jump.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(20)) {
		spect += crypt_str("Fake duck");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.fakeduck_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(21)) {
		spect += crypt_str("Slow walk");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.misc.slowwalk_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}

	if (key_binds::get().get_key_bind_state(22)) {
		spect += crypt_str("Body aim");
		spect += crypt_str("\n");
		specs++;

		switch (g_cfg.ragebot.body_aim_key.mode) {
		case 0: {
			mode += crypt_str("[holding]  ");
		}break;
		case 1: {
			mode += crypt_str("[toggled]  ");
		}break;
		}
		mode += crypt_str("\n");
		modes++;
	}



	ImVec2 p;
	ImVec2 size_menu;

	static bool window_set = false;
	float speed = 10;
	static bool finish = false;
	static bool other_bind_pressed = false;
	static int sub_tabs = false;

	if (key_binds::get().get_key_bind_state(4 + hooks::rage_weapon) || key_binds::get().get_key_bind_state(16) || key_binds::get().get_key_bind_state(18) || key_binds::get().get_key_bind_state(20)
		|| key_binds::get().get_key_bind_state(21) || key_binds::get().get_key_bind_state(17) || key_binds::get().get_key_bind_state(22) || key_binds::get().get_key_bind_state(13) || key_binds::get().get_key_bind_state(14) || key_binds::get().get_key_bind_state(15)
		|| misc::get().double_tap_key || misc::get().hide_shots_key || key_binds::get().get_key_bind_state(0) || key_binds::get().get_key_bind_state(3) || key_binds::get().get_key_bind_state(23))
		other_bind_pressed = true;
	else
		other_bind_pressed = false;

	if (g_cfg.menu.windowsize_x_saved != g_cfg.menu.oldwindowsize_x_saved)
	{
		if (g_cfg.menu.windowsize_x_saved > g_cfg.menu.oldwindowsize_x_saved)
			g_cfg.menu.windowsize_x_saved -= g_cfg.menu.speed;

		if (g_cfg.menu.windowsize_x_saved < g_cfg.menu.oldwindowsize_x_saved)
			g_cfg.menu.windowsize_x_saved += g_cfg.menu.speed;
	}

	if (g_cfg.menu.windowsize_y_saved != g_cfg.menu.oldwindowsize_y_saved)
	{
		if (g_cfg.menu.windowsize_y_saved > g_cfg.menu.oldwindowsize_y_saved)
			g_cfg.menu.windowsize_y_saved -= g_cfg.menu.speed;

		if (g_cfg.menu.windowsize_y_saved < g_cfg.menu.oldwindowsize_y_saved)
			g_cfg.menu.windowsize_y_saved += g_cfg.menu.speed;
	}

	if (g_cfg.menu.windowsize_x_saved == g_cfg.menu.oldwindowsize_x_saved && g_cfg.menu.windowsize_y_saved == g_cfg.menu.oldwindowsize_y_saved)
		finish = true;
	else
		finish = false;

	if (!g_cfg.antiaim.flip_desync.key || !g_cfg.misc.automatic_peek.key || !g_cfg.misc.fakeduck_key.key || !g_cfg.misc.slowwalk_key.key || !g_cfg.ragebot.double_tap_key.key || !g_cfg.misc.fakeduck_key.key || !g_cfg.misc.thirdperson_toggle.key || !hooks::menu_open)
	{
		g_cfg.menu.windowsize_x_saved = 0;
		g_cfg.menu.windowsize_y_saved = 0;
	}

	static float alpha_menu = 0.1f;

	if (other_bind_pressed)
		if (alpha_menu < 1.f)
			alpha_menu += 0.05f;
		else
			if (alpha_menu > 0.1f)
				alpha_menu -= 0.05f;

	bool theme = true;

	if ((g_cfg.misc.keybinds))
	{
		if ((other_bind_pressed && alpha_menu > 0.1f) || hooks::menu_open)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_menu);
			if (ImGui::Begin("Binds", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar))
			{
				auto b_alpha = alpha_menu;
				size_menu = ImGui::GetWindowSize();
				p = ImGui::GetWindowPos();
				auto draw = ImGui::GetWindowDrawList();
				std::vector<std::string> keybind = { "Keybinds" };

				draw->AddRectFilled({ p.x + 0, p.y + 0 }, { p.x + 220, p.y + 25 }, ImColor(15, 15, 15));


				ImGui::PushFont(c_menu::get().g_pinfoFont);
				draw->AddText(ImVec2(p.x + 10, p.y + 7), ImColor(200, 200, 200), keybind.at(0).c_str());

				if (specs > 0) spect += "\n";
				if (modes > 0) mode += "\n";
				ImVec2 size = ImGui::CalcTextSize(spect.c_str());
				ImVec2 size2 = ImGui::CalcTextSize(mode.c_str());
				ImGui::SetWindowSize(ImVec2(220, 25 + size.y));
				ImGui::SetCursorPosY(25);
				ImGui::Columns(2, "fart1", false);

				ImGui::SetColumnWidth(0, 220 - (size2.x + 8));
				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), spect.c_str());
				ImGui::NextColumn();

				ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, alpha_menu), mode.c_str());
				ImGui::Columns(1);
				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleVar();
		}
	}
}

bool misc::createmove(CUserCmd* m_pcmd)
{
	//this->double_tap_deffensive(m_pcmd);

	return true;
}

void misc::double_tap_deffensive(CUserCmd* m_pcmd)
{
	return;
}

bool misc::double_tap(CUserCmd* m_pcmd)
{
	double_tap_enabled = true;

	static auto recharge_double_tap = false;
	static auto last_double_tap = 0;

	if (recharge_double_tap)
	{
		recharge_double_tap = false;
		recharging_double_tap = true;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}
	
	if (recharging_double_tap)
	{
		auto recharge_time = globals.g.weapon->can_double_tap() ? TIME_TO_TICKS(0.45f) : TIME_TO_TICKS(1.25f);

		if (!aim::get().should_stop && fabs(globals.g.fixed_tickbase - last_double_tap) > recharge_time)
		{
			last_double_tap = 0;

			recharging_double_tap = false;
			double_tap_key = true;
		}
		else if (m_pcmd->m_buttons & IN_ATTACK)
			last_double_tap = globals.g.fixed_tickbase;
	}

	if (!g_cfg.ragebot.enable)
	{
		double_tap_enabled = false;
		double_tap_key = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (!g_cfg.ragebot.double_tap)
	{
		double_tap_enabled = false;
		double_tap_key = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (g_cfg.ragebot.double_tap_key.key <= KEY_NONE || g_cfg.ragebot.double_tap_key.key >= KEY_MAX)
	{
		double_tap_enabled = false;
		double_tap_key = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (double_tap_key && g_cfg.ragebot.double_tap_key.key != g_cfg.antiaim.hide_shots_key.key)
		hide_shots_key = false;

	if (!double_tap_key)
	{
		double_tap_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (globals.local()->m_bGunGameImmunity() || globals.local()->m_fFlags() & FL_FROZEN) //-V807
	{
		double_tap_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (m_gamerules()->m_bIsValveDS())
	{
		double_tap_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (globals.g.fakeducking)
	{
		double_tap_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return false;
	}

	if (antiaim::get().freeze_check)
		return true;

	auto max_tickbase_shift = globals.g.weapon->get_max_tickbase_shift();

	if (!globals.g.weapon->is_grenade() && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && globals.send_packet && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2 && globals.g.weapon->is_knife())) //-V648
	{
		auto next_command_number = m_pcmd->m_command_number + 1;
		auto user_cmd = m_input()->GetUserCmd(next_command_number);

		memcpy(user_cmd, m_pcmd, sizeof(CUserCmd)); //-V598
		user_cmd->m_command_number = next_command_number;

		util::copy_command(user_cmd, max_tickbase_shift);

		if (globals.g.aimbot_working)
		{
			globals.g.double_tap_aim = true;
			globals.g.double_tap_aim_check = true;
		}

		recharge_double_tap = true;
		double_tap_enabled = false;
		double_tap_key = false;

		last_double_tap = globals.g.fixed_tickbase;
	}
	else if (!globals.g.weapon->is_grenade() && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_TASER && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && !globals.g.weapon->is_knife())
		globals.g.tickbase_shift = max_tickbase_shift;

	return true;
}

void misc::hide_shots(CUserCmd* m_pcmd, bool should_work)
{
	hide_shots_enabled = true;

	if (!g_cfg.ragebot.enable)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			globals.g.ticks_allowed = 0;
			globals.g.next_tickbase_shift = 0;
		}

		return;
	}

	if (!g_cfg.antiaim.hide_shots)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			globals.g.ticks_allowed = 0;
			globals.g.next_tickbase_shift = 0;
		}

		return;
	}

	if (g_cfg.antiaim.hide_shots_key.key <= KEY_NONE || g_cfg.antiaim.hide_shots_key.key >= KEY_MAX)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;

		if (should_work)
		{
			globals.g.ticks_allowed = 0;
			globals.g.next_tickbase_shift = 0;
		}

		return;
	}

	if (!should_work && double_tap_key)
	{
		hide_shots_enabled = false;
		hide_shots_key = false;
		return;
	}

	if (!hide_shots_key)
	{
		hide_shots_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return;
	}

	double_tap_key = false;

	if (globals.local()->m_bGunGameImmunity() || globals.local()->m_fFlags() & FL_FROZEN)
	{
		hide_shots_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return;
	}

	if (globals.g.fakeducking)
	{
		hide_shots_enabled = false;
		globals.g.ticks_allowed = 0;
		globals.g.next_tickbase_shift = 0;
		return;
	}

	if (antiaim::get().freeze_check)
		return;

	globals.g.next_tickbase_shift = m_gamerules()->m_bIsValveDS() ? 6 : 15;

	auto revolver_shoot = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER && !globals.g.revolver_working && (m_pcmd->m_buttons & IN_ATTACK || m_pcmd->m_buttons & IN_ATTACK2);
	auto weapon_shoot = m_pcmd->m_buttons & IN_ATTACK && globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER || m_pcmd->m_buttons & IN_ATTACK2 && globals.g.weapon->is_knife() || revolver_shoot;

	if (globals.send_packet && !globals.g.weapon->is_grenade() && weapon_shoot)
		globals.g.tickbase_shift = globals.g.next_tickbase_shift;
}
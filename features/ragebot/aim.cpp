// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "aim.h"
#include "..\misc\misc.h"
#include "..\misc\logs.h"
#include "..\autowall\autowall.h"
#include "..\misc\prediction_system.h"
#include "..\fakewalk\slowwalk.h"
#include "..\lagcompensation\local_animations.h"
#include "..\visuals\other_esp.h"

void aim::run(CUserCmd* cmd)
{
	backup.clear();
	targets.clear();
	scanned_targets.clear();
	final_target.reset();
	should_stop = false;

	if (!g_cfg.ragebot.enable)
		return;

	automatic_revolver(cmd);
	prepare_targets();

	if (globals.g.weapon->is_non_aim())
		return;

	if (globals.g.current_weapon == -1)
		return;

	scan_targets();

	if (!should_stop && g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_PREDICTIVE])
	{
		for (auto& target : targets)
		{
			if (!target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data, globals.g.eye_pos);

			if (!last_data.valid())
				continue;

			should_stop = true;
			break;
		}
	}

	if (!automatic_stop(cmd))
		return;

	if (scanned_targets.empty())
		return;

	find_best_target();

	if (!final_target.data.valid())
		return;

	fire(cmd);
}

void aim::automatic_revolver(CUserCmd* cmd)
{
	if (!m_engine()->IsActiveApp())
		return;

	if (globals.g.weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER)
		return;

	if (cmd->m_buttons & IN_ATTACK)
		return;

	cmd->m_buttons &= ~IN_ATTACK2;

	static auto r8cock_time = 0.0f;
	auto server_time = TICKS_TO_TIME(globals.g.backup_tickbase);

	if (globals.g.weapon->can_fire(false))
	{
		if (r8cock_time <= server_time) //-V807
		{
			if (globals.g.weapon->m_flNextSecondaryAttack() <= server_time)
				r8cock_time = server_time + TICKS_TO_TIME(13); // fix for 128 tickrate
			else
				cmd->m_buttons |= IN_ATTACK2;
		}
		else
			cmd->m_buttons |= IN_ATTACK;
	}
	else
	{
		r8cock_time = server_time + TICKS_TO_TIME(13);
		cmd->m_buttons &= ~IN_ATTACK;
	}

	globals.g.revolver_working = true;
}

void aim::prepare_targets()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		if (g_cfg.player_list.white_list[i])
			continue;

		auto e = (player_t*)m_entitylist()->GetClientEntity(i);

		if (!e->valid(true, false))
			continue;

		auto records = &player_records[i]; //-V826

		if (records->empty())
			continue;

		auto original_record = get_record(records, false);
		auto history_record = get_record(records, true);

		targets.emplace_back(target(e, original_record, history_record));
	}


	if (targets.size() >= 4)
	{
		Vector engine_angles;
		m_engine()->GetViewAngles(engine_angles);

		for (auto i = 0; i < (targets.size() - 4); ++i)
		{
			auto worst_dist = -1.f;
			int worst_player = -1;
			for (auto i = 0; i < targets.size(); ++i)
			{
				auto fov = math::get_fov(engine_angles, math::calculate_angle(globals.g.eye_pos, targets.at(i).e->hitbox_position(HITBOX_CHEST)));
				auto dist = targets.at(i).e->GetAbsOrigin().DistTo(globals.local()->GetAbsOrigin());
				if (dist > worst_dist && fov > 25.f)
				{
					worst_dist = dist;
					worst_player = i;
				}
			}

			if (worst_player != -1)
				targets.erase(targets.begin() + i);
		}
	}

	for (auto& target : targets)
		backup.emplace_back(adjust_data(target.e));
}

static bool compare_records(const optimized_adjust_data& first, const optimized_adjust_data& second)
{
	auto first_pitch = math::normalize_pitch(first.angles.x);
	auto second_pitch = math::normalize_pitch(second.angles.x);

	if (fabs(first_pitch - second_pitch) > 15.0f)
		return fabs(first_pitch) < fabs(second_pitch);
	else if (first.duck_amount != second.duck_amount) //-V550
		return first.duck_amount < second.duck_amount;
	else if (first.origin != second.origin)
		return first.origin.DistTo(globals.local()->GetAbsOrigin()) < second.origin.DistTo(globals.local()->GetAbsOrigin());

	return first.simulation_time > second.simulation_time;
}

adjust_data* aim::get_record(std::deque <adjust_data>* records, bool history)
{
	if (history)
	{
		std::deque <optimized_adjust_data> optimized_records; //-V826

		for (auto i = 0; i < records->size(); ++i) // red
		{
			auto record = &records->at(i);
			optimized_adjust_data optimized_record;

			optimized_record.i = i;
			optimized_record.player = record->player;
			optimized_record.simulation_time = record->simulation_time;
			optimized_record.duck_amount = record->duck_amount;
			optimized_record.angles = record->angles;
			optimized_record.origin = record->origin;

			optimized_records.emplace_back(optimized_record);
		}

		if (optimized_records.size() < 2)
			return nullptr;

		std::sort(optimized_records.begin(), optimized_records.end(), compare_records);

		for (auto& optimized_record : optimized_records)
		{
			auto record = &records->at(optimized_record.i);

			if (!record->valid())
				continue;

			return record;
		}
	}
	else
	{
		for (auto i = 0; i < records->size(); ++i)
		{
			auto record = &records->at(i);

			if (!record->valid())
				continue;

			return record;
		}
	}

	return nullptr;
}

int aim::get_minimum_damage(bool visible, int health)
{
	auto minimum_damage = 1;

	if (visible)
	{
		if (g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_visible_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_visible_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_visible_damage, 1, health);
	}
	else
	{
		if (g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_damage, 1, health);
	}

	if (key_binds::get().get_key_bind_state(4 + globals.g.current_weapon))
	{
		if (g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage > 100)
			minimum_damage = health + g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage - 100;
		else
			minimum_damage = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].minimum_override_damage, 1, health);
	}

	return minimum_damage;
}

void aim::scan_targets()
{
	if (targets.empty())
		return;

	for (auto& target : targets)
	{
		if (target.history_record->valid())
		{
			scan_data last_data;

			if (target.last_record->valid())
			{
				target.last_record->adjust_player();
				scan(target.last_record, last_data);
			}

			scan_data history_data;

			target.history_record->adjust_player();
			scan(target.history_record, history_data);

			if (last_data.valid() && last_data.damage > history_data.damage)
				scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
			else if (history_data.valid())
				scanned_targets.emplace_back(scanned_target(target.history_record, history_data));
		}
		else
		{
			if (!target.last_record->valid())
				continue;

			scan_data last_data;

			target.last_record->adjust_player();
			scan(target.last_record, last_data);

			if (!last_data.valid())
				continue;

			scanned_targets.emplace_back(scanned_target(target.last_record, last_data));
		}
	}
}

bool aim::automatic_stop(CUserCmd* cmd)
{
	if (!should_stop)
		return true;

	if (!g_cfg.ragebot.weapon[globals.g.current_weapon].autostop)
		return true;

	if (globals.g.slowwalking)
		return true;

	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND)) //-V807
		return true;

	if (globals.g.weapon->is_empty())
		return true;

	if (!g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_BETWEEN_SHOTS] && !globals.g.weapon->can_fire(false))
		return true;

	auto animlayer = globals.local()->get_animlayers()[1];

	if (animlayer.m_nSequence)
	{
		auto activity = globals.local()->sequence_activity(animlayer.m_nSequence);

		if (activity == ACT_CSGO_RELOAD && animlayer.m_flWeight > 0.0f)
			return true;
	}

	auto weapon_info = globals.g.weapon->get_csweapon_info();

	if (!weapon_info)
		return true;

	auto max_speed = 0.33f * (globals.g.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);

	if (engineprediction::get().backup_data.velocity.Length2D() < max_speed)
		slowwalk::get().create_move(cmd);
	else
	{
		Vector direction;
		Vector real_view;

		math::vector_angles(engineprediction::get().backup_data.velocity, direction);
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

		cmd->m_forwardmove = negative_forward_direction.x;
		cmd->m_sidemove = negative_side_direction.y;

		if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_FORCE_ACCURACY])
			return false;
	}

	return true;
}

static bool compare_points(const scan_point& first, const scan_point& second)
{
	return !first.center && first.hitbox == second.hitbox;
}

void aim::scan(adjust_data* record, scan_data& data, const Vector& shoot_position, bool optimized)
{
	if (!globals.g.weapon)
		return;

	auto weapon_info = globals.g.weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	auto hitboxes = get_hitboxes(record);

	if (hitboxes.empty())
		return;

	auto force_safe_points = key_binds::get().get_key_bind_state(3) || g_cfg.player_list.force_safe_points[record->i] || g_cfg.ragebot.weapon[globals.g.current_weapon].max_misses && globals.g.missed_shots[record->i] >= g_cfg.ragebot.weapon[globals.g.current_weapon].max_misses_amount;
	auto best_damage = 0;

	auto minimum_damage = get_minimum_damage(false, record->player->m_iHealth());
	auto minimum_visible_damage = get_minimum_damage(true, record->player->m_iHealth());

	auto get_hitgroup = [](const int& hitbox)
	{
		if (hitbox == HITBOX_HEAD)
			return 0;
		else if (hitbox == HITBOX_PELVIS)
			return 1;
		else if (hitbox == HITBOX_STOMACH)
			return 2;
		else if (hitbox >= HITBOX_LOWER_CHEST && hitbox <= HITBOX_UPPER_CHEST)
			return 3;
		else if (hitbox >= HITBOX_RIGHT_THIGH && hitbox <= HITBOX_LEFT_FOOT)
			return 4;
		else if (hitbox >= HITBOX_RIGHT_HAND && hitbox <= HITBOX_LEFT_FOREARM)
			return 5;

		return -1;
	};

	std::vector <scan_point> points;

	for (auto& hitbox : hitboxes)
	{
		auto current_points = get_points(record, hitbox);

		for (auto& point : current_points)
		{
			point.safe = record->bot || (hitbox_intersection(record->player, record->matrixes_data.zero, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.first, hitbox, shoot_position, point.point) && hitbox_intersection(record->player, record->matrixes_data.second, hitbox, shoot_position, point.point));

			if (!force_safe_points || point.safe)
				points.emplace_back(point);
		}
	}

	for (auto& point : points)
	{
		if (points.empty())
			return;

		if (point.hitbox == HITBOX_HEAD)
			continue;

		for (auto it = points.begin(); it != points.end(); ++it)
		{
			if (point.point == it->point)
				continue;

			auto first_angle = math::calculate_angle(shoot_position, point.point);
			auto second_angle = math::calculate_angle(shoot_position, it->point);

			auto distance = shoot_position.DistTo(point.point);
			auto fov = math::fast_sin(DEG2RAD(math::get_fov(first_angle, second_angle))) * distance;

			if (fov < 5.0f)
			{
				points.erase(it);
				break;
			}
		}
	}

	if (points.empty())
		return;

	auto body_hitboxes = true;

	for (auto& point : points)
	{
		if (body_hitboxes && (point.hitbox < HITBOX_PELVIS || point.hitbox > HITBOX_UPPER_CHEST))
		{
			body_hitboxes = false;

			if (g_cfg.player_list.force_body_aim[record->i])
				break;

			if (key_binds::get().get_key_bind_state(22))
				break;

			if (g_cfg.ragebot.weapon[globals.g.current_weapon].prefer_body_aim && best_damage >= 1)
				break;
		}

		auto fire_data = autowall::get().wall_penetration(shoot_position, point.point, record->player);

		if (!fire_data.valid)
			continue;

		if (fire_data.damage < 1)
			continue;

		if (!fire_data.visible && !g_cfg.ragebot.autowall)
			continue;

		if (get_hitgroup(fire_data.hitbox) != get_hitgroup(point.hitbox))
			continue;

		auto current_minimum_damage = fire_data.visible ? minimum_visible_damage : minimum_damage;

		if (fire_data.damage >= current_minimum_damage && fire_data.damage >= best_damage)
		{
			if (!should_stop)
			{
				should_stop = true;

				if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_LETHAL] && fire_data.damage < record->player->m_iHealth())
					should_stop = false;
				else if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_VISIBLE] && !fire_data.visible)
					should_stop = false;
				else if (g_cfg.ragebot.weapon[globals.g.current_weapon].autostop_modifiers[AUTOSTOP_CENTER] && !point.center)
					should_stop = false;
			}

			if (force_safe_points && !point.safe)
				continue;

			best_damage = fire_data.damage;

			data.point = point;
			data.visible = fire_data.visible;
			data.damage = fire_data.damage;
			data.hitbox = fire_data.hitbox;
		}
	}
}

std::vector <int> aim::get_hitboxes(adjust_data* record, bool optimized)
{
	std::vector <int> hitboxes; //-V827

	if (optimized)
	{
		hitboxes.emplace_back(HITBOX_HEAD);
		hitboxes.emplace_back(HITBOX_CHEST);
		hitboxes.emplace_back(HITBOX_STOMACH);
		hitboxes.emplace_back(HITBOX_PELVIS);

		return hitboxes;
	}

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(1))
		hitboxes.emplace_back(HITBOX_UPPER_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(2))
		hitboxes.emplace_back(HITBOX_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(3))
		hitboxes.emplace_back(HITBOX_LOWER_CHEST);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(4))
		hitboxes.emplace_back(HITBOX_STOMACH);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(5))
		hitboxes.emplace_back(HITBOX_PELVIS);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(0))
		hitboxes.emplace_back(HITBOX_HEAD);

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(6))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_UPPER_ARM);
		hitboxes.emplace_back(HITBOX_LEFT_UPPER_ARM);
	}

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(7))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_THIGH);
		hitboxes.emplace_back(HITBOX_LEFT_THIGH);

		hitboxes.emplace_back(HITBOX_RIGHT_CALF);
		hitboxes.emplace_back(HITBOX_LEFT_CALF);
	}

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitboxes.at(8))
	{
		hitboxes.emplace_back(HITBOX_RIGHT_FOOT);
		hitboxes.emplace_back(HITBOX_LEFT_FOOT);
	}

	return hitboxes;
}

std::vector <scan_point> aim::get_points(adjust_data* record, int hitbox, bool from_aim)
{
	
		std::vector <scan_point> points; //-V827
		auto model = record->player->GetModel();

		if (!model)
			return points;

		auto hdr = m_modelinfo()->GetStudioModel(model);

		if (!hdr)
			return points;

		auto set = hdr->pHitboxSet(record->player->m_nHitboxSet());

		if (!set)
			return points;

		auto bbox = set->pHitbox(hitbox);

		if (!bbox)
			return points;

		auto g_VecMax = bbox->bbmax;
		auto g_VecMin = bbox->bbmin;
		auto g_VecCenter = (bbox->bbmin + bbox->bbmax) / 2.f;

		bool g_StaticPointScale = g_cfg.ragebot.weapon[globals.g.current_weapon].static_point_scale;

		if (bbox->radius <= 0.0f)
		{
			auto rotation_matrix = math::angle_matrix(bbox->rotation);

			matrix3x4_t matrix;
			math::concat_transforms(record->matrixes_data.main[bbox->bone], rotation_matrix, matrix);

			auto origin = matrix.GetOrigin();

			if (hitbox == HITBOX_UPPER_CHEST || hitbox == HITBOX_RIGHT_THIGH || hitbox == HITBOX_LOWER_CHEST || hitbox == HITBOX_CHEST || hitbox == HITBOX_LEFT_THIGH)
			{
				const auto v105 = (g_VecMax.x + g_VecMin.x) * .5f;
				const auto v106 = (g_VecMax.y + g_VecMin.y) * .5f;
				const auto v107 = g_VecMax.x - v105;
				const auto v108 = (g_VecMax.z + g_VecMin.z) * .5f;

				points.emplace_back(scan_point(Vector(.0f, v106, .0f), hitbox, false));
				points.emplace_back(scan_point(Vector(.0f, .0f, v108), hitbox, false));
				points.emplace_back(scan_point(Vector(v107 * .45f + v105, .0f, .0f), hitbox, false));
			}
		}
		else
		{
			auto g_HeadScale = .0f;
			auto g_BodyScale = .0f;

			if (g_StaticPointScale)
			{
				if (hitbox == HITBOX_HEAD)
					g_HeadScale = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].head_scale, .0f, .70f);
				else if (hitbox == HITBOX_UPPER_CHEST || hitbox == HITBOX_RIGHT_THIGH || hitbox == HITBOX_LOWER_CHEST || hitbox == HITBOX_CHEST || hitbox == HITBOX_LEFT_THIGH || hitbox == HITBOX_RIGHT_UPPER_ARM || hitbox == HITBOX_LEFT_UPPER_ARM || hitbox == HITBOX_PELVIS)
					g_BodyScale = math::clamp(g_cfg.ragebot.weapon[globals.g.current_weapon].body_scale, .0f, .65f);
			}

			if (!g_StaticPointScale)
			{
				auto transformed_center = g_VecCenter;
				math::vector_transform(transformed_center, record->matrixes_data.main[bbox->bone], transformed_center);

				auto g_Accuracy = globals.g.spread + globals.g.inaccuracy;
				auto g_Distance = transformed_center.DistTo(globals.g.eye_pos);

				g_Distance /= math::fast_sin(DEG2RAD(90.f - RAD2DEG(g_Accuracy)));
				g_Accuracy = math::fast_sin(g_Accuracy);

				auto g_HitboxRadius = max(bbox->radius - g_Distance * g_Accuracy, .0f);

				g_HeadScale = math::clamp(g_HitboxRadius / bbox->radius, .0f, .70f);
				g_BodyScale = math::clamp(g_HitboxRadius / bbox->radius, .0f, .65f);
			}

			if ((g_HeadScale || g_BodyScale) <= .0f) //-V648
			{
				math::vector_transform(g_VecCenter, record->matrixes_data.main[bbox->bone], g_VecCenter);
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));

				return points;
			}

			auto g_HeadRadius = bbox->radius * g_HeadScale;
			auto g_BodyRadius = bbox->radius * g_BodyScale;

			if (hitbox == HITBOX_HEAD)
			{
				auto pitch_down = math::normalize_pitch(record->angles.x) > 85.f;
				auto backward = fabs(math::normalize_yaw(record->angles.y - math::calculate_angle(record->player->get_shoot_position(), globals.local()->GetAbsOrigin()).y)) > 120.f;

				//Center points.
				points.emplace_back(scan_point(g_VecCenter, hitbox, !pitch_down || !backward));

				points.emplace_back(scan_point(Vector(bbox->bbmax.x + 0.70710678f * g_HeadRadius, bbox->bbmax.y - 0.70710678f * g_HeadRadius, bbox->bbmax.z), hitbox, false));

				//Points scale.
				points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z + g_HeadRadius), hitbox, false));
				points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y, bbox->bbmax.z - g_HeadRadius), hitbox, false));
				points.emplace_back(scan_point(Vector(bbox->bbmax.x, bbox->bbmax.y - g_HeadRadius, bbox->bbmax.z), hitbox, false));

				if (pitch_down && backward)
					points.emplace_back(scan_point(Vector(bbox->bbmax.x - g_HeadRadius, bbox->bbmax.y, bbox->bbmax.z), hitbox, false));
			}
			else if (hitbox == HITBOX_STOMACH)
			{
				//Center points.
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));

				//Points scale.
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMin.z + g_BodyRadius), hitbox, false));
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMax.z - g_BodyRadius), hitbox, false));

				//Backward point.
				points.emplace_back(scan_point(Vector{ g_VecCenter.x, g_VecMax.y - g_BodyRadius, g_VecCenter.z }, hitbox, true));
			}
			else if (hitbox == HITBOX_PELVIS || hitbox == HITBOX_UPPER_CHEST)
			{
				//Center points.
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));

				//Points scale.
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMax.z + g_BodyRadius), hitbox, false));
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMin.z - g_BodyRadius), hitbox, false));
			}
			else if (hitbox == HITBOX_LOWER_CHEST || hitbox == HITBOX_CHEST)
			{
				//Center points.
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));

				//Points scale.
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMax.z + g_BodyRadius), hitbox, false));
				points.emplace_back(scan_point(Vector(g_VecCenter.x, g_VecCenter.y, g_VecMin.z - g_BodyRadius), hitbox, false));

				//Backward point.
				points.emplace_back(scan_point(Vector{ g_VecCenter.x, g_VecMax.y - g_BodyRadius, g_VecCenter.z }, hitbox, false));
			}
			else if (hitbox == HITBOX_RIGHT_THIGH || hitbox == HITBOX_LEFT_THIGH)
			{
				//Center point.
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));
			}
			else if (hitbox == HITBOX_RIGHT_UPPER_ARM || hitbox == HITBOX_LEFT_UPPER_ARM)
			{
				//Center point.
				points.emplace_back(scan_point(g_VecCenter, hitbox, true));
			}
		}

		for (auto& point : points)
			math::vector_transform(point.point, record->matrixes_data.main[bbox->bone], point.point);

		return points;

}

static bool compare_targets(const scanned_target& first, const scanned_target& second)
{
	if (g_cfg.player_list.high_priority[first.record->i] != g_cfg.player_list.high_priority[second.record->i])
		return g_cfg.player_list.high_priority[first.record->i];

	switch (g_cfg.ragebot.weapon[globals.g.current_weapon].selection_type)
	{
	case 1:
		return first.fov < second.fov;
	case 2:
		return first.distance < second.distance;
	case 3:
		return first.health < second.health;
	case 4:
		return first.data.damage > second.data.damage;
	}

	return false;
}

void aim::find_best_target()
{
	if (g_cfg.ragebot.weapon[globals.g.current_weapon].selection_type)
		std::sort(scanned_targets.begin(), scanned_targets.end(), compare_targets);

	for (auto& target : scanned_targets)
	{
		if (target.fov > (float)g_cfg.ragebot.field_of_view)
			continue;

		final_target = target;
		final_target.record->adjust_player();
		break;
	}
}

void aim::fire(CUserCmd* cmd)
{
	if (!globals.g.weapon->can_fire(true))
		return;

	auto aim_angle = math::calculate_angle(globals.g.eye_pos, final_target.data.point.point).Clamp();

	if (!g_cfg.ragebot.silent_aim)
		m_engine()->SetViewAngles(aim_angle);

	if (!g_cfg.ragebot.autoshoot && !(cmd->m_buttons & IN_ATTACK))
		return;

	auto final_hitchance = 0;

	if (g_cfg.ragebot.weapon[globals.g.current_weapon].hitchance)
	{
		auto is_valid_hitchance = calculate_hitchance(final_hitchance);

		if (!is_valid_hitchance)
		{
			auto is_zoomable_weapon = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AUG || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SG553;

			if (g_cfg.ragebot.autoscope && is_zoomable_weapon && !globals.g.weapon->m_zoomLevel())
				cmd->m_buttons |= IN_ATTACK2;

			return;
		}
	}

	auto backtrack_ticks = 0;
	auto net_channel_info = m_engine()->GetNetChannelInfo();

	if (net_channel_info)
	{
		auto original_tickbase = globals.g.backup_tickbase;

		if (misc::get().double_tap_enabled && misc::get().double_tap_key)
			original_tickbase = globals.g.backup_tickbase + globals.g.weapon->get_max_tickbase_shift();

		static auto sv_maxunlag = m_cvar()->FindVar(crypt_str("sv_maxunlag"));

		if (globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1)
			backtrack_ticks = 0;

		auto correct = math::clamp(net_channel_info->GetLatency(FLOW_OUTGOING) + net_channel_info->GetLatency(FLOW_INCOMING) + util::get_interpolation(), 0.0f, sv_maxunlag->GetFloat());
		auto delta_time = correct - (TICKS_TO_TIME(original_tickbase) - final_target.record->simulation_time);

		backtrack_ticks = TIME_TO_TICKS(fabs(delta_time));
	}

	static auto get_hitbox_name = [](int hitbox, bool shot_info = false) -> std::string
	{
		switch (hitbox)
		{
		case HITBOX_HEAD:
			return shot_info ? crypt_str("Head") : crypt_str("head");
		case HITBOX_LOWER_CHEST:
			return shot_info ? crypt_str("Lower chest") : crypt_str("lower chest");
		case HITBOX_CHEST:
			return shot_info ? crypt_str("Chest") : crypt_str("chest");
		case HITBOX_UPPER_CHEST:
			return shot_info ? crypt_str("Upper chest") : crypt_str("upper chest");
		case HITBOX_STOMACH:
			return shot_info ? crypt_str("Stomach") : crypt_str("stomach");
		case HITBOX_PELVIS:
			return shot_info ? crypt_str("Pelvis") : crypt_str("pelvis");
		case HITBOX_RIGHT_UPPER_ARM:
		case HITBOX_RIGHT_FOREARM:
		case HITBOX_RIGHT_HAND:
			return shot_info ? crypt_str("Left arm") : crypt_str("left arm");
		case HITBOX_LEFT_UPPER_ARM:
		case HITBOX_LEFT_FOREARM:
		case HITBOX_LEFT_HAND:
			return shot_info ? crypt_str("Right arm") : crypt_str("right arm");
		case HITBOX_RIGHT_THIGH:
		case HITBOX_RIGHT_CALF:
			return shot_info ? crypt_str("Left leg") : crypt_str("left leg");
		case HITBOX_LEFT_THIGH:
		case HITBOX_LEFT_CALF:
			return shot_info ? crypt_str("Right leg") : crypt_str("right leg");
		case HITBOX_RIGHT_FOOT:
			return shot_info ? crypt_str("Left foot") : crypt_str("left foot");
		case HITBOX_LEFT_FOOT:
			return shot_info ? crypt_str("Right foot") : crypt_str("right foot");
		}
	};

	otheresp::get().add_matrix(final_target.record->player, final_target.record->matrixes_data.main);

	static auto get_resolver_type = [](resolver_type type) -> std::string
	{
		switch (type)
		{
		case ORIGINAL:
			return crypt_str("none");
		case BRUTEFORCE:
			return crypt_str("bruteforce");
		case LBY:
			return crypt_str("lowerbody");
		case DELTA:
			return crypt_str("opposite");
		case LAYERS:
			return crypt_str("animation layer");
		case TRACE:
			return crypt_str("trace");
		case DIRECTIONAL:
			return crypt_str("direction");
		}
	};

	static auto get_side_type = [](resolver_side type) -> std::string
	{
		switch (type)
		{
		case RESOLVER_ORIGINAL:
			return crypt_str("none ");
		case RESOLVER_ZERO:
			return crypt_str("zero desync delta ");
		case RESOLVER_FIRST:
			return crypt_str("positive max desync delta ");
		case RESOLVER_SECOND:
			return crypt_str("negative max desync delta ");									  
		case RESOLVER_LOW_FIRST:
			return crypt_str("positive half of max desync delta ");
		case RESOLVER_LOW_SECOND:
			return crypt_str("negative half of max desync delta ");
		}
	};
	
	// если надоели логи ресольвера измени на false,или захотелись обратно измени на true //    
    #define RESOLVER_LOGS_ENABLED false	  // хуйня крашит

	player_info_s player_info;
	m_engine()->GetPlayerInfo(final_target.record->i, &player_info);

	// логи куда красивее чем были
	std::stringstream log;	
	log << crypt_str("Fired shot at ") + (std::string)player_info.szName + crypt_str(": ");
	log << crypt_str("hitchance: ") + (final_hitchance > 100 ? crypt_str("100") : std::to_string(final_hitchance)) + crypt_str(", ");
	log << crypt_str("hitbox: ") + get_hitbox_name(final_target.data.hitbox) + crypt_str(", ");
	log << crypt_str("damage: ") + std::to_string(final_target.data.damage) + crypt_str(", ");
	log << crypt_str("safe: ") + std::to_string((bool)final_target.data.point.safe) + crypt_str(", ");
	log << crypt_str("backtrack: ") + std::to_string(backtrack_ticks);
	if (!player_info.fakeplayer && RESOLVER_LOGS_ENABLED == true)
	{
		log << crypt_str("delta: ") + get_side_type(final_target.record->side);
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_HIT])
		eventlogs::get().add(log.str(), Color(0, 130, 200));

	cmd->m_viewangles = aim_angle;
	cmd->m_buttons |= IN_ATTACK;
	cmd->m_tickcount = TIME_TO_TICKS(final_target.record->simulation_time + util::get_interpolation());

	last_target_index = final_target.record->i;
	last_shoot_position = globals.g.eye_pos;
	last_target[last_target_index] = Last_target
	{
		*final_target.record, final_target.data, final_target.distance
	};

	auto shot = &globals.shots.emplace_back();

	shot->last_target = last_target_index;
	shot->side = final_target.record->side;
	shot->fire_tick = m_globals()->m_tickcount;
	shot->shot_info.target_name = player_info.szName;
	shot->shot_info.client_hitbox = get_hitbox_name(final_target.data.hitbox, true);
	shot->shot_info.client_damage = final_target.data.damage;
	shot->shot_info.hitchance = math::clamp(final_hitchance, 0, 100);
	shot->shot_info.backtrack_ticks = backtrack_ticks;
	shot->shot_info.aim_point = final_target.data.point.point;

	globals.g.aimbot_working = true;
	globals.g.revolver_working = false;
	globals.g.last_aimbot_shot = m_globals()->m_tickcount;
}

static std::vector<std::tuple<float, float, float>> pre_computed_seeds = {};
static const int total_seeds = 255;
void aim::build_seed_table()
{
	if (!pre_computed_seeds.empty())
		return;

	for (auto i = 0; i < total_seeds; i++) {
		math::random_seed(i + 1);

		const auto pi_seed = math::random_float(0.f, M_PI * 2);

		// получаем сиды (255), затем число пи(отношение окружности к ее диаметру) умножаем на 2

		pre_computed_seeds.emplace_back(math::random_float(0.f, 1.f),
			sin(pi_seed), cos(pi_seed));

	}
}

bool aim::calculate_hitchance(int& final_hitchance)
{
	// generate look-up-table to enhance performance.
	build_seed_table();	 // строим сиды разброса

	const auto info = globals.g.weapon->get_csweapon_info();

	if (!info)
	{
		final_hitchance = 0;
		return true;
	}

	auto hitchance_cfg = g_cfg.ragebot.weapon[globals.g.current_weapon].hitchance_amount;

	// performance optimization.
	if ((globals.g.eye_pos - final_target.data.point.point).Length() > info->flRange)
	{
		final_hitchance = 0;
		return true;
	}

	static auto nospread = m_cvar()->FindVar(crypt_str("weapon_accuracy_nospread"));

	if (nospread->GetBool())
	{
		final_hitchance = INT_MAX;		 // nospread fix
		return true;
	}

	// setup calculation parameters.
	const auto round_acc = [](const float accuracy) { return roundf(accuracy * 1000.f) / 1000.f; };
	const auto sniper = globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_AWP || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_G3SG1
		|| globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SCAR20 || globals.g.weapon->m_iItemDefinitionIndex() == WEAPON_SSG08;
	const auto crouched = globals.local()->m_fFlags() & FL_DUCKING;

	// calculate inaccuracy.
	auto weapon_inaccuracy = globals.g.weapon->get_inaccuracy();


	if (globals.g.double_tap_aim)
		final_hitchance = 100;		// hc when dt is shifting

	// no need for hitchance, if we can't increase it anyway.
	if (crouched)
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyCrouchAlt : info->flInaccuracyCrouch))
		{
			final_hitchance = 0;  // crouched increased hitchance
			return true;
		}
	}
	else
	{
		if (round_acc(weapon_inaccuracy) == round_acc(sniper ? info->flInaccuracyStandAlt : info->flInaccuracyStand))
		{
			final_hitchance = 100;	 // stand hitchance
			return true;
		}
	}

	// calculate start and angle.
	static auto weapon_recoil_scale = m_cvar()->FindVar(crypt_str("weapon_recoil_scale"));
	const auto aim_angle = math::calculate_angle(globals.g.eye_pos, final_target.data.point.point) - (globals.local()->m_aimPunchAngle() * weapon_recoil_scale->GetFloat());
	auto forward = Vector(0.f, 0.f, 0.f);
	auto right = Vector(0.f, 0.f, 0.f);
	auto up = Vector(0.f, 0.f, 0.f);

	math::angle_vectors(aim_angle, &forward, &right, &up);

	math::fast_vec_normalize(forward);
	math::fast_vec_normalize(right);
	math::fast_vec_normalize(up);

	// keep track of all traces that hit the enemy.
	auto current = 0;

	// setup calculation parameters.
	Vector total_spread, end;
	Vector spread_angle;

	float inaccuracy;
	float spread_x;
	float spread_y;
	std::tuple<float, float, float>* seed;

	
	for (auto i = 0; i < 255; i++)  
	{
		// get seed.
		seed = &pre_computed_seeds[i];	 // get seeds


		auto a = math::random_float(0.0f, 1.0f);
		auto b = math::random_float(0.0f, M_PI * 2);
		auto c = math::random_float(0.0f, 1.0f);
		auto d = math::random_float(0.0f, M_PI * 2);
		static float spread_values[255][6];
		spread_values[i][0] = a;
		spread_values[i][1] = c;

		auto sin_b = 0.0f, cos_b = 0.0f;
		DirectX::XMScalarSinCos(&sin_b, &cos_b, b);

		auto sin_d = 0.0f, cos_d = 0.0f;
		DirectX::XMScalarSinCos(&sin_d, &cos_d, d);

		spread_values[i][2] = sin_b;
		spread_values[i][3] = cos_b;
		spread_values[i][4] = sin_d;
		spread_values[i][5] = cos_d;

		// calculate spread.
		inaccuracy = spread_values[i][0] * weapon_inaccuracy;	  // get weapon inaccuracy
		auto spread = spread_values[i][1] * globals.g.spread;  // get spread and * 0
		auto spread_x = spread_values[i][3] * inaccuracy + spread_values[i][5] * spread; // get spread x
		auto spread_y = spread_values[i][2] * inaccuracy + spread_values[i][4] * spread;  // get spread y
		
		auto direction = Vector(0.f, 0.f, 0.f);

		direction.x = forward.x + spread_x * right.x + spread_y * up.x;		// направление по Pitch
		direction.y = forward.y + spread_x * right.y + spread_y * up.y;	   // направление по Yaw
		direction.z = forward.z + spread_x * right.z + spread_y * up.z;	   // направление по Z

		total_spread.x = (forward.x + right.x * spread_x + up.x * spread_y); // spread x
		total_spread.y = (forward.y + right.y * spread_x + up.y * spread_y); // spread y
		total_spread.z = (forward.z + right.z * spread_x + up.z * spread_y);  // spread z
	
		math::clamp(total_spread.x, total_spread.y, total_spread.z);			// clamp spread
		total_spread.Normalize();				// normalize spread


		// calculate angle with spread applied.
		math::vector_angles(total_spread, spread_angle);

		// calculate end point of trace.
		math::angle_vectors(spread_angle , end);
		end.Normalize();
		end = globals.g.eye_pos + end * info->flRange;

		// did we hit the hitbox?
		if (hitbox_intersection(final_target.record->player, final_target.record->matrixes_data.main, final_target.data.hitbox, globals.g.eye_pos, end))
			current++;

		// abort if hitchance is already sufficent.
		if ((static_cast<float>(current) / 255.f) * 100.f >= hitchance_cfg)		  // делим на количество спридов (255)
		{
			final_hitchance = (static_cast<float>(current) / 255.f) * 100.f;
			return true;
		}

		// abort if we can no longer reach hitchance.
		if ((static_cast<float>(current + 255 - i) / 255.f) * 100.f < hitchance_cfg)
		{
			final_hitchance = (static_cast<float>(current + 255 - i) / 255.f) * 100.f;
			return false;
		}
	}

	final_hitchance = (static_cast<float>(current) / 255.f) * 100.f >= hitchance_cfg;
	return (static_cast<float>(current) / 255.f) * 100.f >= hitchance_cfg;
}

static int clip_ray_to_hitbox(const Ray_t& ray, mstudiobbox_t* hitbox, matrix3x4_t& matrix, trace_t& trace)
{
	static auto fn = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 F3 0F 10 42"));

	trace.fraction = 1.0f;
	trace.startsolid = false;

	return reinterpret_cast <int(__fastcall*)(const Ray_t&, mstudiobbox_t*, matrix3x4_t&, trace_t&)> (fn)(ray, hitbox, matrix, trace);
}

bool aim::hitbox_intersection(player_t* e, matrix3x4_t* matrix, int hitbox, const Vector& start, const Vector& end, float* safe)
{
	auto model = e->GetModel();

	if (!model)
		return false;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return false;

	auto studio_set = studio_model->pHitboxSet(e->m_nHitboxSet());

	if (!studio_set)
		return false;

	auto studio_hitbox = studio_set->pHitbox(hitbox);

	if (!studio_hitbox)
		return false;

	Vector min, max;

	const auto is_capsule = studio_hitbox->radius != -1.f;

	if (is_capsule)
	{
		math::vector_transform(studio_hitbox->bbmin, matrix[studio_hitbox->bone], min);
		math::vector_transform(studio_hitbox->bbmax, matrix[studio_hitbox->bone], max);
		const auto dist = math::segment_to_segment(start, end, min, max);

		if (dist < studio_hitbox->radius)
			return true;
	}
	else
	{
		math::vector_transform(math::vector_rotate(studio_hitbox->bbmin, studio_hitbox->rotation), matrix[studio_hitbox->bone], min);
		math::vector_transform(math::vector_rotate(studio_hitbox->bbmax, studio_hitbox->rotation), matrix[studio_hitbox->bone], max);

		math::vector_i_transform(start, matrix[studio_hitbox->bone], min);
		math::vector_i_rotate(end, matrix[studio_hitbox->bone], max);

		if (math::intersect_line_with_bb(min, max, studio_hitbox->bbmin, studio_hitbox->bbmax))
			return true;
	}

	return false;
}
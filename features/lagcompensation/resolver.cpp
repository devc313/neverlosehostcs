#include "animation_system.h"
#include "..\ragebot\aim.h"

void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
	player = e;
	player_record = record;

	original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
	original_pitch = math::normalize_pitch(pitch);
}


void resolver::lagcomp_initialize(player_t* player, Vector& origin, Vector& velocity, int& flags, bool on_ground)
{
	lagcompensation::get().extrapolate(player, origin, velocity, flags, on_ground);	   // �������� �������������
}

void resolver::reset()
{
	player = nullptr;
	player_record = nullptr;		   // �������� ���

}

float_t get_backward_side(player_t* player)
{
	return math::calculate_angle(globals.local()->m_vecOrigin(), player->m_vecOrigin()).y;	// �������� backward ������� (yaw �����)
}

static auto resolve_update_animations(player_t* e)
{
	e->update_clientside_animation();
};

Vector GetHitboxPos(player_t* player, matrix3x4_t* mat, int hitbox_id)
{
	if (!player)
		return Vector();

	auto hdr = m_modelinfo()->GetStudioModel(player->GetModel());

	if (!hdr)
		return Vector();

	auto hitbox_set = hdr->pHitboxSet(player->m_nHitboxSet());

	if (!hitbox_set)
		return Vector();

	auto hitbox = hitbox_set->pHitbox(hitbox_id);

	if (!hitbox)
		return Vector();

	Vector min, max;

	math::vector_transform(hitbox->bbmin, mat[hitbox->bone], min);
	math::vector_transform(hitbox->bbmax, mat[hitbox->bone], max);

	return (min + max) * 0.5f;
}

float_t MaxYawModificator(player_t* enemy)
{
	auto animstate = enemy->get_animation_state();

	if (!animstate)
		return 0.0f;

	auto speedfactor = math::clamp(animstate->m_flFeetSpeedForwardsOrSideWays, 0.0f, 1.0f);
	auto avg_speedfactor = (animstate->m_flStopToFullRunningFraction * -0.3f - 0.2f) * speedfactor + 1.0f;

	auto duck_amount = animstate->m_fDuckAmount;

	if (duck_amount)
	{
		auto max_velocity = math::clamp(animstate->m_flFeetSpeedUnknownForwardOrSideways, 0.0f, 1.0f);
		auto duck_speed = duck_amount * max_velocity;

		avg_speedfactor += duck_speed * (0.5f - avg_speedfactor);
	}

	return animstate->yaw_desync_adjustment() * avg_speedfactor;
}

float_t GetBackwardYaw(player_t* player) {

	return math::calculate_angle(player->m_vecOrigin(), player->m_vecOrigin()).y;
}

void resolver::resolve_yaw()
{
	player_info_t player_info; 
	auto choked = abs(TIME_TO_TICKS(player->m_flSimulationTime() - player->m_flOldSimulationTime()) - 1);  // choked ����������

	if (!m_engine()->GetPlayerInfo(player->EntIndex(), &player_info))
		return;

	if (!globals.local()->is_alive() || player->m_iTeamNum() == globals.local()->m_iTeamNum())
		return;

	int playerint = player->EntIndex();

	auto animstate = player->get_animation_state();

	float new_body_yaw_pose = 0.0f;
	auto m_flCurrentFeetYaw = player->get_animation_state()->m_flCurrentFeetYaw;
	auto m_flGoalFeetYaw = player->get_animation_state()->m_flGoalFeetYaw;
	auto m_flEyeYaw = player->get_animation_state()->m_flEyeYaw;
	float flMaxYawModifier = MaxYawModificator(player);
	float flMinYawModifier = player->get_animation_state()->pad10[512];
	auto anglesy = math::normalize_yaw(player->m_angEyeAngles().y - original_goal_feet_yaw);

	auto valid_lby = true;

	auto speed = player->m_vecVelocity().Length2D();

	float m_lby = player->m_flLowerBodyYawTarget() * 0.574f;

	auto player_stand = player->m_vecVelocity().Length2D();
	player_stand = 0.f;

	float m_flLastClientSideAnimationUpdateTimeDelta = 0.0f;
	auto trace = false;

	auto v54 = animstate->m_fDuckAmount;
	auto v55 = ((((*(float*)((uintptr_t)animstate + 0x11C)) * -0.30000001) - 0.19999999) * animstate->m_flFeetSpeedForwardsOrSideWays) + 1.0f;

	bool bWasMovingLastUpdate = false;
	bool bJustStartedMovingLastUpdate = false;

	auto unknown_velocity = *(float*)(uintptr_t(animstate) + 0x2A4);

	auto first_matrix = player_record->matrixes_data.first;
	auto second_matrix = player_record->matrixes_data.second;
	auto central_matrix = player_record->matrixes_data.zero;
	auto leftPose = GetHitboxPos(player, first_matrix, HITBOX_HEAD);
	auto rightPose = GetHitboxPos(player, second_matrix, HITBOX_HEAD);
	auto centralPose = GetHitboxPos(player, central_matrix, HITBOX_HEAD);

	auto fire_first = autowall::get().wall_penetration(globals.g.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first), player);
	auto fire_second = autowall::get().wall_penetration(globals.g.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second), player);
	auto fire_third = autowall::get().wall_penetration(globals.g.eye_pos, player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.zero), player);

	auto delta = math::AngleDiff(animstate->m_flEyeYaw, animstate->m_flGoalFeetYaw);

	///////////////////// [ ANIMLAYERS ] /////////////////////
	auto i = player->EntIndex();	// ������� ���������� i, ������� ����� �������� �� ������ ������, ����� �� ���������� ����
	AnimationLayer layers[15];
	memcpy(layers, player->get_animlayers(), player->animlayer_count() * sizeof(AnimationLayer));  // ���������� ���������� layers ������� ������, ����������� �� ��������� ���������� � �������� ������ AnimatioLayer * 13(����� ������) 
	
	// p.s ���� ���������� � ���, ����� ������ ������� ����� �������� (layers) � ������� (player_record), � ���������� - ������� ����� ����.
	
	if (player->m_fFlags() & FL_ONGROUND || player_record->flags & FL_ONGROUND)	// �������� ���������� ������ �� �����
	{
		if (speed <= 0.1f || ( layers[3].m_flWeight == 0.f && layers[3].m_flCycle == 0.f ) )	// ���� �������� ������ ������ 0.1f (������) ��� ������������ ���� 3(adjust) ����� ��� 0.f
		{
			delta = -delta;	 // ������������� ����� � ��������������� ������� (max_desync_delta * -1) 
			if (globals.g.missed_shots[player->EntIndex()] % 2)
			{
				static const float resolve_list[3] = {player->m_angEyeAngles().y - player->get_max_desync_delta(), player->m_angEyeAngles().y - player->get_max_desync_delta() };	  // � ������, ���� ��������� ����� 2� ��� 3 ������.
				animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + resolve_list[globals.g.missed_shots[player->EntIndex()] % 2]);	
			}
		}
		else 
		{
			if (!(player_record->layers[12].m_flWeight))	 // slowwalk check
			{
				if (speed > 1.f && (player_record->layers[6].m_flWeight) == (layers[6].m_flWeight)) // ���������� ������� � ���������� �������, ���� ��� ����� - ���� ���, ���� ���, �� ���� � ������ �������
				{
					lagcompensation::get().setupvelocity(player, player_record); // �������� ������������� � ����� ����������� (player, player_record)
					float delta_rotate_first = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[0][6].m_flPlaybackRate);		 // ����������� ������
					float delta_rotate_second = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[2][6].m_flPlaybackRate);	// ����� ������
					float delta_rotate_third = abs(player_record->layers[6].m_flPlaybackRate - resolver_layers[1][6].m_flPlaybackRate);		// ������ ������

					// ������ �������� ������, ��� ������ �� ������� � ��� �������, �� ��� �� ���, ������� ���� - ������, ���������� �� �������� ������������ -
					// - �������� �� �������, ���� ����������, �� ������ ���� � ��������������� �������, ���� �� ������� - ���� ����������. 

					// ������ resolver_layers? ������ ��� �� ������ ������������ � ���� ��, ��� �����. � ������ ������ ��������� ����� ��� ��������� �� �������
					// �� ������� �� �� +60, -60, � 0, ����� ������ �� ��� ��������� ����, ����� ��������� �������� ������������ ��������(playbackrate)
					// ������� �����, ���� �� ������ �������� movelayers - �� � ���� ��� ����� ����� ��������� � angles.y = 0.f (��������)
					// ���� ����� ������ ���, ����� � ���� ���� ���������, ������ ������ ����� ���� ���� (60, -60, 0)
					// ���� �� ������ �������� ������� ����� ��� ������ �� �������, �� � ���� ���� ����� ������������ ��� � 0.f
					// animation_system.cpp  744 ������, �� ������������� ������ ������� ����, � ����� ���������� resolver_layers ��� ���������
					// ������ ������ ������������ ����� �����, �� ����� ������� ������� ������������� �������� - ��� � ����.
					// ���� ������� ������ (layers) �� ��������� � ����������(player_record), �� � ���� ��������� � ������ �������. 

					if (delta_rotate_first < delta_rotate_second || delta_rotate_second <= delta_rotate_third)
					{
						if (delta_rotate_first >= delta_rotate_third && delta_rotate_second > delta_rotate_third)
						{
							animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + player->get_max_desync_delta());
							globals.g.updating_animation = true;
							switch (globals.g.missed_shots[i] % 2) // ������ ������ ������ ����
							{
							case 0:	animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - player->get_max_desync_delta());
								globals.g.updating_animation = true;
								break;
							case 1:
								animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + player->get_max_desync_delta());
								globals.g.updating_animation = true;
								break;
							case 2:
								animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - player->get_max_desync_delta());
								globals.g.updating_animation = true;
								break;
							case 3:
								globals.g.missed_shots[i] = 0;
								break;
							default: globals.g.missed_shots[i] = 0; break;
							}
						}
					}
					else
					{
						animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - player->get_max_desync_delta());
						globals.g.updating_animation = true;
						switch (globals.g.missed_shots[i] % 2)	// ������ ������ ������ ����
						{
						case 0:	animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + player->get_max_desync_delta());
							globals.g.updating_animation = true;
							break;
						case 1:
							animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - player->get_max_desync_delta());
							globals.g.updating_animation = true;
							break;
						case 2:
							animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + player->get_max_desync_delta());
							globals.g.updating_animation = true;
							break;
						case 3:
							globals.g.missed_shots[i] = 0;
							break;

						default: globals.g.missed_shots[i] = 0; break;
						}
					}
				}
			}
		}
	}   ///////////////////// [ ANIMLAYERS ] /////////////////////
}

float resolver::resolve_pitch()
{
		return original_pitch;
}


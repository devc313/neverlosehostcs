// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "local_animations.h"

void local_animations::run(ClientFrameStage_t stage)
{
	if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
	{
		if (stage == FRAME_NET_UPDATE_END)
		{
			fake_server_update = false;

			auto animstate = globals.local()->get_animation_state();

			if (!animstate)
				return;

			if (globals.local()->m_flSimulationTime() != fake_simulation_time)
			{
				fake_server_update = true;
				fake_simulation_time = globals.local()->m_flSimulationTime();
			}

			globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
			globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
			globals.local()->get_animlayers()[12].m_flWeight = 0.0f;

			update_fake_animations();
		}
		else if (stage == FRAME_RENDER_START)
		{
			auto animstate = globals.local()->get_animation_state();

			if (!animstate)
				return;

			real_server_update = false;

			if (globals.local()->m_flSimulationTime() != real_simulation_time)
			{
				real_server_update = true;
				real_simulation_time = globals.local()->m_flSimulationTime();
			}

			globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
			globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
			globals.local()->get_animlayers()[12].m_flWeight = 0.0f;

			update_local_animations(animstate);
		}
	}
	else if (stage == FRAME_RENDER_START)
	{
		auto animstate = globals.local()->get_animation_state();

		if (!animstate)
			return;

		real_server_update = false;
		fake_server_update = false;

		if (globals.local()->m_flSimulationTime() != real_simulation_time || globals.local()->m_flSimulationTime() != fake_simulation_time)
		{
			real_server_update = fake_server_update = true;
			real_simulation_time = fake_simulation_time = globals.local()->m_flSimulationTime();
		}

		globals.local()->get_animlayers()[3].m_flWeight = 0.0f;
		globals.local()->get_animlayers()[3].m_flCycle = 0.0f;
		globals.local()->get_animlayers()[12].m_flWeight = 0.0f;

		update_fake_animations();
		update_local_animations(animstate);
	}
}

void local_animations::update_prediction_animations()
{
	auto alloc = !local_data.prediction_animstate;
	auto change = !alloc && handle != &globals.local()->GetRefEHandle();
	auto reset = !alloc && !change && globals.local()->m_flSpawnTime() != spawntime;

	if (change)
		m_memalloc()->Free(local_data.prediction_animstate);

	if (reset)
	{
		util::reset_state(local_data.prediction_animstate);
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (alloc || change)
	{
		local_data.prediction_animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

		if (local_data.prediction_animstate)
			util::create_state(local_data.prediction_animstate, globals.local());

		handle = (CBaseHandle*)&globals.local()->GetRefEHandle();
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (!alloc && !change && !reset)
	{
		float pose_parameter[24]; //-V688
		memcpy(pose_parameter, &globals.local()->m_flPoseParameter(), 24 * sizeof(float));

		AnimationLayer layers[15]; //-V688
		memcpy(layers, globals.local()->get_animlayers(), globals.local()->animlayer_count() * sizeof(AnimationLayer));

		local_data.prediction_animstate->m_pBaseEntity = globals.local();
		util::update_state(local_data.prediction_animstate, ZERO);

		globals.local()->setup_bones_rebuilt(globals.g.prediction_matrix, BONE_USED_BY_HITBOX);

		memcpy(&globals.local()->m_flPoseParameter(), pose_parameter, 24 * sizeof(float));
		memcpy(globals.local()->get_animlayers(), layers, globals.local()->animlayer_count() * sizeof(AnimationLayer));
	}
}

void local_animations::update_fake_animations()
{
	
	auto alloc = !local_data.animstate;
	auto change = !alloc && handle != &globals.local()->GetRefEHandle();
	auto reset = !alloc && !change && globals.local()->m_flSpawnTime() != spawntime;

	if (change)
		m_memalloc()->Free(local_data.animstate);

	if (reset)
	{
		util::reset_state(local_data.animstate);
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (alloc || change)
	{
		local_data.animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

		if (local_data.animstate)
			util::create_state(local_data.animstate, globals.local());

		handle = (CBaseHandle*)&globals.local()->GetRefEHandle();
		spawntime = globals.local()->m_flSpawnTime();
	}

	if (!alloc && !change && !reset && fake_server_update)
	{
		float pose_parameter[24]; //-V688
		memcpy(pose_parameter, &globals.local()->m_flPoseParameter(), 24 * sizeof(float));

		AnimationLayer layers[15]; //-V688
		memcpy(layers, globals.local()->get_animlayers(), globals.local()->animlayer_count() * sizeof(AnimationLayer));

		auto backup_frametime = m_globals()->m_frametime;
		auto backup_curtime = m_globals()->m_curtime;

		m_globals()->m_frametime = m_globals()->m_intervalpertick;
		m_globals()->m_curtime = globals.local()->m_flSimulationTime();

		local_data.animstate->m_pBaseEntity = globals.local();
		util::update_state(local_data.animstate, local_animations::get().local_data.fake_angles);

		local_data.animstate->m_bInHitGroundAnimation = false;
		local_data.animstate->m_fLandingDuckAdditiveSomething = 0.0f;
		local_data.animstate->m_flHeadHeightOrOffsetFromHittingGroundAnimation = 1.0f;

		globals.local()->setup_bones_rebuilt(globals.g.fake_matrix, BONE_USED_BY_ANYTHING);
		local_data.visualize_lag = g_cfg.player.visualize_lag;

		if (!local_data.visualize_lag)
		{
			for (auto& i : globals.g.fake_matrix)
			{
				i[0][3] -= globals.local()->GetRenderOrigin().x;
				i[1][3] -= globals.local()->GetRenderOrigin().y;
				i[2][3] -= globals.local()->GetRenderOrigin().z;
			}
		}

		m_globals()->m_frametime = backup_frametime;
		m_globals()->m_curtime = backup_curtime;

		memcpy(&globals.local()->m_flPoseParameter(), pose_parameter, 24 * sizeof(float));
		memcpy(globals.local()->get_animlayers(), layers, globals.local()->animlayer_count() * sizeof(AnimationLayer));
	}
}

void local_animations::update_local_animations(c_baseplayeranimationstate* animstate)
{
	if (!animstate)
	{
		globals.g.updating_animation = false;
		return;
	}

	if (tickcount != m_globals()->m_tickcount)
	{
		tickcount = m_globals()->m_tickcount;
		memcpy(layers, globals.local()->get_animlayers(), globals.local()->animlayer_count() * sizeof(AnimationLayer));

		if (local_data.animstate)
			animstate->m_fDuckAmount = local_data.animstate->m_fDuckAmount;

		animstate->m_iLastClientSideAnimationUpdateFramecount = 0;
		util::update_state(animstate, local_animations::get().local_data.fake_angles);

		if (real_server_update)
		{
			abs_angles = animstate->m_flGoalFeetYaw;
			memcpy(pose_parameter, &globals.local()->m_flPoseParameter(), 24 * sizeof(float));
		}
	}
	else
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount;


	bool old_onground = animstate->m_bOnGround;
	bool new_onladder = !animstate->m_bOnGround && globals.local()->get_move_type() == MOVETYPE_LADDER;
	bool new_onground = globals.local()->m_fFlags() & FL_ONGROUND;

	bool just_landed;
	if (old_onground != new_onground && new_onground)
		just_landed = true;
	else
		just_landed = false;

	if (animstate->m_bOnGround)
	{
		if (!animstate->m_bInHitGroundAnimation)
		{
			if (just_landed || new_onladder)
			{
				int landing_activity = animstate->time_since_in_air() > 1.0f ? ACT_CSGO_LAND_HEAVY : ACT_CSGO_LAND_LIGHT;
				layers[5].m_nSequence = landing_activity;
				layers[5].m_flCycle = 0.0f;
				animstate->m_bInHitGroundAnimation = true;
			}
		}
	}

	if (!globals.local()->get_move_type() == MOVETYPE_LADDER)
	{
		if (layers[5].m_flWeight > 0.0f)
		{
			float v175 = (animstate->time_since_in_air() - 0.2f) * -5.0f;
			v175 = std::clamp(v175, 0.0f, 1.0f);
			float newlayer5_weight = ((3.0f - (v175 + v175)) * (v175 * v175)) * layers[5].m_flWeight;
			// set new weight.
			layers[5].m_flWeight = newlayer5_weight;
		}
	}

	globals.local()->get_animation_state()->m_flFeetCycle = layers[6].m_flCycle;
	globals.local()->get_animation_state()->m_flFeetYawRate = layers[6].m_flWeight;

	globals.local()->get_animation_state()->m_primary_cycle() = layers[6].m_flCycle;
	globals.local()->get_animation_state()->m_move_weight() = layers[6].m_flWeight;



	animstate->m_flGoalFeetYaw = antiaim::get().condition(globals.get_command()) ? abs_angles : local_animations::get().local_data.real_angles.y;
	globals.local()->set_abs_angles(Vector(0.0f, abs_angles, 0.0f));

	memcpy(globals.local()->get_animlayers(), layers, globals.local()->animlayer_count() * sizeof(AnimationLayer));
	memcpy(&globals.local()->m_flPoseParameter(), pose_parameter, 24 * sizeof(float));
}
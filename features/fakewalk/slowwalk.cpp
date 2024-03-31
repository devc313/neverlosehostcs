// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "slowwalk.h"

void slowwalk::create_move(CUserCmd* m_pcmd, float custom_speed)
{
	if (!(globals.local()->m_fFlags() & FL_ONGROUND && engineprediction::get().backup_data.flags & FL_ONGROUND))
		return;

	auto weapon_info = globals.g.weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	if (custom_speed == -1.0f) //-V550
		globals.g.slowwalking = true;

	auto modifier = custom_speed == -1.0f ? 0.3f : custom_speed; //-V550
	auto max_speed = modifier * (globals.g.scoped ? weapon_info->flMaxPlayerSpeedAlt : weapon_info->flMaxPlayerSpeed);



	if (!globals.g.weapon->is_non_aim())
	{
		auto move_length = sqrt(m_pcmd->m_sidemove * m_pcmd->m_sidemove + m_pcmd->m_forwardmove * m_pcmd->m_forwardmove);

		auto forwardmove = m_pcmd->m_forwardmove / move_length;
		auto sidemove = m_pcmd->m_sidemove / move_length;

		if (move_length > max_speed)
		{
			if (max_speed + 1.0f < globals.local()->m_vecVelocity().Length2D())
			{
				m_pcmd->m_forwardmove = 0.0f;
				m_pcmd->m_sidemove = 0.0f;
				
			}
			else
			{
				m_pcmd->m_sidemove = max_speed * sidemove;
				m_pcmd->m_forwardmove = max_speed * forwardmove;
			}
		}
	}
	else
	{
		auto forwardmove = m_pcmd->m_forwardmove;
		auto sidemove = m_pcmd->m_sidemove;

		auto move_length = sqrt(sidemove * sidemove + forwardmove * forwardmove);
		auto move_length_backup = move_length;

		if (move_length > 110.0f)
		{
			m_pcmd->m_forwardmove = forwardmove / move_length_backup * 110.0f;
			move_length = sidemove / move_length_backup * 110.0f;
			m_pcmd->m_sidemove = sidemove / move_length_backup * 110.0f;
		}
	}
}
void slowwalk::stop(float speed, CUserCmd* m_cmd) {
	auto sv_friction = m_cvar()->FindVar(crypt_str("sv_friction"));
	auto sv_stopspeed = m_cvar()->FindVar(crypt_str("sv_stopspeed"));

	auto get_max_friction_acceleration = [&]() {
		Vector v6 = engineprediction::get().backup_data.velocity;
		float v8 = sv_friction->GetFloat() * globals.local()->m_surfaceFriction();
		float v11 = v6.Length();

		if (v11 <= 0.1f)
			goto LABEL_12;

		float v43 = fmax(v11, sv_stopspeed->GetFloat());
		float v23 = v43 * v8 * m_globals()->m_intervalpertick;
		float v19 = fmax(0.f, v11 - v23);

		if (v19 != v11) {
			v19 /= v11;

			v6 *= v19;
		}
	LABEL_12:
		return v6;
	};

	float m_side_move{ }, m_forward_move{ }, v54{ };


LABEL_246:
	if (m_forward_move <= 450.0)
	{
		if (m_forward_move < -450.0)
			m_forward_move = -450.0;
	}
	else
	{
		m_forward_move = 450.0;
	}
	if (v54 <= 450.0)
	{
		if (v54 >= -450.0)
			m_side_move = v54;
		else
			m_side_move = -450.0;
	}

	m_cmd->m_forwardmove = m_forward_move;
	m_cmd->m_sidemove = m_side_move;
}
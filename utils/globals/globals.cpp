// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "globals.hpp"
Globals globals;

bool Globals::available()
{
	if (!globals.local())
		return false;
		
	if (!m_engine()->IsInGame())
		return false;

	return true;
}

player_t* Globals::local(player_t* e, bool initialization)
{
	static player_t* local = nullptr;

	if (initialization)
		local = e;

	return local;
}

CUserCmd* Globals::get_command()
{
	return m_pcmd;
}
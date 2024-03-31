#pragma once

#define NDEBUG
#define DIRECTINPUT_VERSION 0x0800

#include "version.h"
#include <thread>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <deque>
#include <array>
#include <ctime>

#include <d3d9.h>
#include <d3dx9.h>

#include <dinput.h>
#include <tchar.h>

#include "features\misc\key_binds.h"
#include "utils\util.hpp"
#include "utils\render\render.h"
#include "utils\globals\globals.hpp"
#include "utils\math\math.hpp"
#include "utils\singleton.h"
#include "utils\crypts\crypt_str.h"
#include "utils\crypts\lazy_importer.hpp"

#include "utils\lua\Clua.h"
#include "utils\detours\detours.h"
#include "hooks\hooks.hpp"
#include "menu\configs\configs.h"

#include "utils\resources\custom_sounds.hpp"
#include "utils\resources\sounds.hpp"

#include "sdk\math\Vector.hpp"
#include "sdk\math\VMatrix.hpp"
#include "sdk\interfaces.hpp"

#include "sdk\misc\UtlVector.hpp"
#include "sdk\misc\EHandle.hpp"
#include "sdk\misc\CUserCmd.hpp"
#include "sdk\misc\Color.hpp"
#include "sdk\misc\KeyValues.hpp"
#include "sdk\misc\datamap.h"

#include "sdk\interfaces\IClientEntity.hpp"
#include "sdk\structs.hpp"

#define m_client interfaces.m_client
#define m_clientmode interfaces.m_clientmode
#define m_clientstate interfaces.m_clientstate
#define m_cvar interfaces.m_cvar
#define m_debugoverlay interfaces.m_debugoverlay
#define m_device interfaces.m_device
#define m_engine interfaces.m_engine
#define m_enginesound interfaces.m_enginesound
#define m_entitylist interfaces.m_entitylist
#define m_eventmanager interfaces.m_eventmanager
#define m_gamemovement interfaces.m_gamemovement
#define m_gamerules interfaces.m_gamerules
#define m_globals interfaces.m_globals
#define m_glow interfaces.m_glow
#define m_input interfaces.m_input
#define m_inputinternal interfaces.m_inputinternal
#define m_inputsys interfaces.m_inputsys
#define m_localize interfaces.m_localize
#define m_materialsystem interfaces.m_materialsystem
#define m_memalloc interfaces.m_memalloc
#define m_modelcache interfaces.m_modelcache
#define m_modelinfo interfaces.m_modelinfo
#define m_modelrender interfaces.m_modelrender
#define m_movehelper interfaces.m_movehelper
#define m_panel interfaces.m_panel
#define m_physsurface interfaces.m_physsurface
#define m_playerresource interfaces.m_playerresource
#define m_postprocessing interfaces.m_postprocessing
#define m_prediction interfaces.m_prediction
#define m_renderview interfaces.m_renderview
#define m_surface interfaces.m_surface
#define m_trace interfaces.m_trace
#define m_viewrenderbeams interfaces.m_viewrenderbeams
#define m_soundservices interfaces.m_soundservices
#define m_basefilesys interfaces.m_basefilesys
#define m_weaponsys interfaces.m_weaponsys
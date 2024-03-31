#pragma once

#include "interfaces\IVEngineClient.hpp"
#include "interfaces\IInputSystem.hpp"
#include "interfaces\IBaseClientDll.hpp"
#include "interfaces\IClientEntityList.hpp"
#include "interfaces\IClientMode.hpp"
#include "interfaces\ICvar.hpp"
#include "interfaces\IEngineTrace.hpp"
#include "interfaces\IEngineSound.hpp"
#include "interfaces\IRenderView.hpp"
#include "interfaces\IVModelRender.hpp"
#include "interfaces\IMaterialSystem.hpp"
#include "interfaces\IPanel.hpp"
#include "interfaces\IVModelInfoClient.hpp"
#include "interfaces\IMDLCache.hpp"
#include "interfaces\memalloc.h"
#include "interfaces\CClientState.hpp"
#include "interfaces\IPrediction.hpp"
#include "interfaces\IMoveHelper.hpp"
#include "interfaces\CInput.hpp"
#include "interfaces\ISurface.hpp"
#include "interfaces\IVDebugOverlay.hpp"
#include "interfaces\IViewRenderBeams.hpp"
#include "interfaces\IPhysics.hpp"
#include "interfaces\ILocalize.hpp"
#include "interfaces\ISoundServices.hpp"
#include "misc\GlobalVars.hpp"
#include "misc\glow_outline_effect.hpp"
#include "interfaces\IGameEventManager.hpp"
#include "misc\C_CSPlayerResource.h"
#include "interfaces\IWeaponSystem.hpp"

#include <d3dx9.h>
#include <d3d9.h>

class InterfaceReg
{
private:
	using InstantiateInterfaceFn = void*(*)();
public:
	InstantiateInterfaceFn m_CreateFn;
	const char *m_pName;
	InterfaceReg *m_pNext;
};

class Interfaces
{
public:
	IDirect3DDevice9 * m_device();
	IVEngineClient * m_engine();
	IInputSystem * m_inputsys();
	IBaseClientDLL * m_client();
	IClientEntityList * m_entitylist();
	ICvar * m_cvar();
	IEngineTrace * m_trace();
	IEngineSound * m_enginesound();
	DWORD * m_inputinternal();
	IVRenderView * m_renderview();
	IMDLCache * m_modelcache();
	IVModelRender * m_modelrender();
	IMaterialSystem * m_materialsystem();
	IPanel * m_panel();
	IVModelInfoClient * m_modelinfo();
	IPrediction * m_prediction();
	IGameMovement * m_gamemovement();
	ISurface * m_surface();
	IVDebugOverlay * m_debugoverlay();
	IPhysicsSurfaceProps * m_physsurface();
	IGameEventManager2 * m_eventmanager();
	IViewRenderBeams * m_viewrenderbeams();
	IMemAlloc * m_memalloc();
	IClientMode * m_clientmode();
	CGlobalVarsBase * m_globals();
	CGlowObjectManager * m_glow();
	CClientState * m_clientstate();
	IMoveHelper * m_movehelper();
	CInput * m_input();
	C_CSPlayerResource * m_playerresource();
	CSGameRulesProxy * m_gamerules();
	ILocalize * m_localize();
	IBaseFileSystem* m_basefilesys();
	IWeaponSystem* m_weaponsys();

	DWORD m_postprocessing();
	DWORD m_ccsplayerrenderablevftable();
private:
	IDirect3DDevice9 * p_device = nullptr;
	IVEngineClient * p_engine = nullptr;
	IInputSystem * p_inputsys = nullptr;
	IBaseClientDLL * p_client = nullptr;
	IClientEntityList * p_entitylist = nullptr;
	IEngineTrace * p_trace = nullptr;
	IEngineSound * p_enginesound = nullptr;
	ICvar * p_cvar = nullptr;
	DWORD * p_inputinternal = nullptr;
	IVRenderView * p_renderview = nullptr;
	IMDLCache* p_modelcache = nullptr;
	IVModelRender * p_modelrender = nullptr;
	IMaterialSystem * p_materialsystem = nullptr;
	IPanel * p_panel = nullptr;
	IVModelInfoClient * p_modelinfo = nullptr;
	IPrediction * p_prediciton = nullptr;
	IGameMovement * p_gamemovement = nullptr;
	ISurface * p_surface = nullptr;
	IVDebugOverlay * p_debugoverlay = nullptr;
	IPhysicsSurfaceProps * p_physsurface = nullptr;
	IGameEventManager2 * p_eventmanager = nullptr;
	IViewRenderBeams * p_viewrenderbeams = nullptr;
	IMemAlloc * p_memalloc = nullptr;
	IClientMode * p_clientmode = nullptr;
	CGlobalVarsBase * p_globals = nullptr;
	CGlowObjectManager * p_glow = nullptr;
	CClientState * p_clientstate = nullptr;
	IMoveHelper * p_movehelper = nullptr;
	CInput * p_input = nullptr;
	C_CSPlayerResource * p_playerresource = nullptr;
	CSGameRulesProxy * p_gamerules = nullptr;
	ILocalize * p_localize = nullptr;
	IBaseFileSystem* p_basefilesys = nullptr;
	IWeaponSystem* p_weaponsys = nullptr;

	DWORD p_postprocessing = 0;
	DWORD p_ccsplayerrenderablevftable = 0;
};

extern Interfaces interfaces;
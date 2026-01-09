#include "stdafx.h"

//#include "resourcemanager.h"
#include "../Include/xrRender/DrawUtils.h"
//#include "xr_effgamma.h"
#include "render.h"
#include "dedicated_server_only.h"
#include "../xrcdb/xrxrc.h"
#include "xr_input.h"

#include <fstream>

extern XRCDB_API BOOL* cdb_bDebug;

void SetupGPU(IRenderDeviceRender* pRender)
{
	// Command line
	char* lpCmdLine = Core.Params;

	BOOL bForceGPU_SW;
	BOOL bForceGPU_NonPure;
	BOOL bForceGPU_REF;

	if (strstr(lpCmdLine, "-gpu_sw") != NULL) bForceGPU_SW = TRUE;
	else bForceGPU_SW = FALSE;
	if (strstr(lpCmdLine, "-gpu_nopure") != NULL) bForceGPU_NonPure = TRUE;
	else bForceGPU_NonPure = FALSE;
	if (strstr(lpCmdLine, "-gpu_ref") != NULL) bForceGPU_REF = TRUE;
	else bForceGPU_REF = FALSE;

	pRender->SetupGPU(bForceGPU_SW, bForceGPU_NonPure, bForceGPU_REF);
}

void CRenderDevice::_SetupStates()
{
	// General Render States
	m_pRender->SetupStates();
}

void CRenderDevice::_Create(LPCSTR shName)
{
	if (Core.ParamFlags.test(Core.dbg))
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		std::string exePath{ buffer };

		auto lastSlash{ exePath.find_last_of("\\/") };
		if (lastSlash != std::string::npos)
		{
			// Директория исполняемого файла
			auto exeDir{ exePath.substr(0, lastSlash) };

			// Находим разделитель для родительской директории
			auto parent_slash{ exeDir.find_last_of("\\/") };
			if (parent_slash != std::string::npos)
			{
				// Родительская директория
				auto parent_dir{ exeDir.substr(0, parent_slash) };

				// Формируем путь к файлу
				notFoundTexturesFilePath = parent_dir + "/not_found_textures.txt";
			}
		}

		if (std::ifstream file{ notFoundTexturesFilePath })
		{
			std::string line;
			while (std::getline(file, line))
				notFoundTextures.insert(line);
		}
	}

	Memory.mem_compact();

	// after creation
	b_is_Ready = TRUE;
	_SetupStates();

	m_pRender->OnDeviceCreate(shName);
	processResolutionChanged();

	dwFrame = 0;
}

void CRenderDevice::ConnectToRender()
{
	if (!m_pRender)
		m_pRender = RenderFactory->CreateRenderDeviceRender();
}

PROTECT_API void CRenderDevice::Create()
{
	//SECUROM_MARKER_SECURITY_ON(4)

	if (b_is_Ready) return; // prevent double call
	Statistic = xr_new<CStats>();

#ifdef DEBUG
	cdb_clRAY = &Statistic->clRAY; // total: ray-testing
	cdb_clBOX = &Statistic->clBOX; // total: box query
	cdb_clFRUSTUM = &Statistic->clFRUSTUM; // total: frustum query
	cdb_bDebug = &bDebug;
#endif

	if (!m_pRender)
		m_pRender = RenderFactory->CreateRenderDeviceRender();
	SetupGPU(m_pRender);
	Log("Starting RENDER device...");

#ifdef _EDITOR
	psCurrentVidMode[0] = dwWidth;
	psCurrentVidMode[1] = dwHeight;
#endif // #ifdef _EDITOR

	m_pRender->Create(
		m_hWnd,
		dwWidth,
		dwHeight,
		fWidth_2,
		fHeight_2
	);

	string_path fname;
	FS.update_path(fname, "$game_data$", "shaders.xr");

	//////////////////////////////////////////////////////////////////////////
	_Create(fname);

	PreCache(0, false, false);

	//SECUROM_MARKER_SECURITY_OFF(4)

	xBench::setupDevice(this);
}

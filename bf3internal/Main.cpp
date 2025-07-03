#include "FB SDK\Frostbite.h"
#include "FB SDK/ClientSoldierPrediction.h"
#include "FB SDK/ClientCameraContext.h"
#include "FB SDK/CameraContext.h"
#include "FB SDK/Enumerations.h"

#include "VMTHook.h"
#include "Functions.h"
#include "Drawing.h"
#include "EntityPrinter.h"
#include "ESP.h"
#include "Aimbot.h"
#include "PBBypass.h"
#include "NameSpoofer.h"

#include <Windows.h>
#include "include/MinHook.h"
#include <d3d11.h>
#include <dxgi.h>
#include <psapi.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <cstdio>

#pragma region
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return malloc(size);
}
void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	EASTL_ASSERT(alignment <= 8);
	return malloc(size);
}
#pragma endregion EASTL

#pragma region
CVMTHookManager* PreFrameHook = 0;
typedef int(__stdcall* tPreFrameUpdate)(float dt);
tPreFrameUpdate oPreFrameUpdate = 0;
fb::BorderInputNode* g_pBorderInputNode = fb::BorderInputNode::Singleton();

CVMTHookManager* PresentHook = 0;
typedef signed int(__stdcall* tPresent)(int, int, int);
tPresent oPresent = nullptr;

using ResizeBuffers_t = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeBuffers_t oResizeBuffers = nullptr;

// Globals for ImGui + DX11
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
HWND                    g_hWnd = nullptr;

bool g_ImGuiInitialized = false;

WNDPROC g_OriginalWndProc = nullptr;
bool g_ShowMenu = true;
bool g_InsertPressed = false;
bool g_ESPEnabled = false;


struct ESPConfig {
    bool enabled = false;
    bool showBoxes = true;
    bool showHealthBars = true;
    bool showNames = true;
    bool showDistance = true;
    bool showHealthText = true;
    bool showTeammates = false;
    bool showEnemies = true;
    float maxDistance = 300.0f;
    float boxThickness = 2.0f;
    bool showPlayerNames = false;
    bool showOffscreenPointers = false;
} g_ESPConfig;


struct CombatConfig {
    bool aimbot = false;
    bool triggerbot = false;
    bool noRecoil = false;
    bool noSpread = false;
    float aimbotFOV = 5.0f;
    float aimbotSmooth = 1.0f;
} g_CombatConfig;


extern MiscConfig g_MiscConfig;

bool g_ThirdPerson = false;
bool g_F3Pressed = false;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND GetSwapChainHWND(IDXGISwapChain* pSwapChain);

void PrintAllEntitiesToConsole();

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_INSERT) {
        if (!g_InsertPressed) {
            g_ShowMenu = !g_ShowMenu;
            g_InsertPressed = true;
        }
    }
    if (msg == WM_KEYUP && wParam == VK_INSERT) {
        g_InsertPressed = false;
    }
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return CallWindowProc(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

int WINAPI hkPreFrame(float DeltaTime)
{
	_asm pushad
	int returnval = oPreFrameUpdate(DeltaTime);


	static int frameCounter = 0;
	frameCounter++;
	
	//if (g_AimbotConfig.enabled) {
	//	if (frameCounter % 100 == 0) printf("[Main] Calling UpdateAimbot (frame %d)\n", frameCounter);
	//	Aimbot::UpdateAimbot();
	//}
	//if (g_AimbotConfig.triggerbot) {
	//	if (frameCounter % 100 == 0) printf("[Main] Calling UpdateTriggerbot (frame %d)\n", frameCounter);
	//	Triggerbot::UpdateTriggerbot();
	//}
	

	if (g_CombatConfig.noRecoil || g_CombatConfig.noSpread) {
		fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
		if (POINTERCHK(g_pGameContext)) {
			fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
			if (POINTERCHK(pPlayerManager)) {
				fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
				if (POINTERCHK(pLocalPlayer)) {
					fb::ClientSoldierEntity* pMySoldier = pLocalPlayer->getSoldierEnt();
					if (POINTERCHK(pMySoldier) && pMySoldier->IsAlive()) {
						if (g_CombatConfig.noRecoil) {
							ApplyNoRecoil(pMySoldier);
						}
						if (g_CombatConfig.noSpread) {
							ApplyNoSpread(pMySoldier);
						}
					}
				}
			}
			static int debugCounter = 0;
			if (++debugCounter % 600 == 0) {
				printf("[Main] No Recoil: %s, No Spread: %s\n", g_CombatConfig.noRecoil ? "ON" : "OFF", g_CombatConfig.noSpread ? "ON" : "OFF");
			}
		}
	}


	if (GetAsyncKeyState(VK_F3) & 0x8000) {
		if (!g_F3Pressed) {
			g_ThirdPerson = !g_ThirdPerson;
			printf("[Main] Third Person toggled: %s\n", g_ThirdPerson ? "ON" : "OFF");
			g_F3Pressed = true;
		}
	} else {
		g_F3Pressed = false;
	}

	_asm popad

	return returnval;
}


signed int __stdcall hkPresent(int a1, int a2, int a3)
{
	__asm pushad;


	if (!g_ImGuiInitialized) {
		
		IDXGISwapChain* pSwapChain = fb::DxRenderer::Singleton()->pSwapChain;
		if (pSwapChain && SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice))) {
			g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
			g_hWnd = GetSwapChainHWND(pSwapChain);
			ImGui::CreateContext();
			ImGui_ImplWin32_Init(g_hWnd);
			ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
			// Install custom WndProc
			g_OriginalWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
			g_ImGuiInitialized = true;
			OutputDebugStringA("[Main] ImGui initialized!\n");
			PBBypass::Initialize(g_pd3dDevice, g_pd3dDeviceContext, pSwapChain);
		} else {
			OutputDebugStringA("[Main] Failed to initialize ImGui!\n");
		}
	}

	if (g_ImGuiInitialized) {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		if (g_ShowMenu) {
		
			ImGui::SetNextWindowSize(ImVec2(500, 650), ImGuiCond_FirstUseEver);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 6.0f);
	
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.12f, 0.98f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.35f, 0.35f, 0.45f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.8f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.0f, 0.8f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.0f, 0.9f, 0.5f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.15f, 0.15f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.0f, 0.6f, 0.3f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
			
			ImGui::Begin("Battlefield 3 Internal - https://github.com/antipaster", nullptr, ImGuiWindowFlags_NoCollapse);
			

			
		
			static int selectedTab = 0;
			const char* tabs[] = { "Combat", "Visuals", "Misc", "Bypass", "Info" };
			
			ImGui::BeginTabBar("##Tabs");
			for (int i = 0; i < IM_ARRAYSIZE(tabs); i++) {
				if (ImGui::BeginTabItem(tabs[i])) {
					selectedTab = i;
					ImGui::EndTabItem();
				}
			}
			ImGui::EndTabBar();
			
			ImGui::Spacing();
			
	
			switch (selectedTab) {
				case 0: { 
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "Combat Settings");
					ImGui::Separator();
					ImGui::Spacing();
					
				
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Aimbot Configuration");
					ImGui::Spacing();
					
					ImGui::Checkbox("Enable Aimbot", &g_AimbotConfig.enabled);
					if (g_AimbotConfig.enabled) {
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "● ACTIVE");
					}
					
					ImGui::Checkbox("Triggerbot", &g_AimbotConfig.triggerbot);
					ImGui::Checkbox("Visibility Check", &g_AimbotConfig.visibilityCheck);
					ImGui::Checkbox("Magic Bullet", &g_AimbotConfig.magicBullet);
					ImGui::Checkbox("Prediction", &g_AimbotConfig.prediction);
					if (g_AimbotConfig.prediction) {
						ImGui::SliderFloat("Prediction Multiplier", &g_AimbotConfig.predictionMultiplier, 0.5f, 2.0f, "%.2f");
					}
					
					ImGui::Spacing();
					ImGui::Text("Aimbot Activation Mode:");
					const char* activationModes[] = { "Hold Key", "Always On" };
					ImGui::Combo("##AimbotActivationMode", &g_AimbotConfig.activationMode, activationModes, IM_ARRAYSIZE(activationModes));
					if (g_AimbotConfig.activationMode == 0) { // Hold Key
						const char* buttons[] = { "Left Mouse", "Right Mouse", "Middle Mouse" };
						ImGui::Text("Key: %s", buttons[g_AimbotConfig.aimButton]);
						ImGui::SliderInt("##AimButton", &g_AimbotConfig.aimButton, 0, 2, "%d");
					}
					
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Aimbot Parameters");
					ImGui::Spacing();
					
					ImGui::Text("Aimbot FOV (pixels): %.0f", g_AimbotConfig.fov);
					ImGui::SliderFloat("Aimbot FOV (pixels)", &g_AimbotConfig.fov, 10.0f, 500.0f, "%.0f");
					
					ImGui::Text("Smoothing: %.2f (1.0 = instant, lower = smoother)", g_AimbotConfig.smooth);
					ImGui::SliderFloat("##Smooth", &g_AimbotConfig.smooth, 0.01f, 1.0f, "%.2f");
					
					ImGui::Text("Max Distance: %.0fm", g_AimbotConfig.maxDistance);
					ImGui::SliderFloat("##MaxDist", &g_AimbotConfig.maxDistance, 50.0f, 1000.0f, "%.0f m");
					
			
					static const char* boneNames[] = { "Hips (4)", "Spine (5)", "Neck (41)", "Head (45)", "Left Shoulder (6)", "Right Shoulder (13)", "Left Elbow (7)", "Right Elbow (14)", "Left Knee (55)", "Right Knee (56)" };
					static int boneIds[] = { 4, 5, 41, 45, 6, 13, 7, 14, 55, 56 };
					int boneIndex = 0;
					for (int i = 0; i < IM_ARRAYSIZE(boneIds); ++i) {
						if (g_AimbotConfig.targetBone == boneIds[i]) { boneIndex = i; break; }
					}
					if (ImGui::Combo("Target Bone", &boneIndex, boneNames, IM_ARRAYSIZE(boneNames))) {
						g_AimbotConfig.targetBone = boneIds[boneIndex];
					}
					
					ImGui::Spacing();
					ImGui::Text("Target Selection:");
					const char* targetTypes[] = { "Enemies only", "Teammates only", "Both" };
					ImGui::Combo("##AimbotTargetType", &g_AimbotConfig.targetType, targetTypes, IM_ARRAYSIZE(targetTypes));
					
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Weapon Modifications");
					ImGui::Spacing();
					
					ImGui::Checkbox("No Recoil", &g_CombatConfig.noRecoil);
					ImGui::Checkbox("No Spread", &g_CombatConfig.noSpread);
					ImGui::Checkbox("Instant Kill", &g_AimbotConfig.instantKill);
					ImGui::Checkbox("Rapid Fire", &g_AimbotConfig.rapidFire);
					ImGui::Checkbox("Fast Reload", &g_AimbotConfig.fastReload);
					

					break;
				}
				case 1: { 
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "ESP Settings");
					ImGui::Separator();
					ImGui::Spacing();
					
					ImGui::Checkbox("Enable ESP", &g_ESPConfig.enabled);
					if (g_ESPConfig.enabled) {
						ImGui::SameLine();
						ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "[ON]");
					}
					
					ImGui::Spacing();
					ImGui::Text("ESP Elements:");
					ImGui::Checkbox("Show Boxes", &g_ESPConfig.showBoxes);
					ImGui::Checkbox("Show Health Bars", &g_ESPConfig.showHealthBars);
					ImGui::Checkbox("Show Names", &g_ESPConfig.showNames);
					ImGui::Checkbox("Show Distance", &g_ESPConfig.showDistance);
					ImGui::Checkbox("Show Health Text", &g_ESPConfig.showHealthText);
					ImGui::Checkbox("Show Player Names", &g_ESPConfig.showPlayerNames);
					ImGui::Checkbox("Show Offscreen Pointers", &g_ESPConfig.showOffscreenPointers);
					
					ImGui::Spacing();
					ImGui::Text("Targets:");
					ImGui::Checkbox("Show Teammates", &g_ESPConfig.showTeammates);
					ImGui::Checkbox("Show Enemies", &g_ESPConfig.showEnemies);
					
					ImGui::Spacing();
					ImGui::Text("Max Distance: %.0fm", g_ESPConfig.maxDistance);
					ImGui::SliderFloat("##MaxDist", &g_ESPConfig.maxDistance, 50.0f, 500.0f, "%.0f");
					
					ImGui::Text("Box Thickness: %.1f", g_ESPConfig.boxThickness);
					ImGui::SliderFloat("##Thickness", &g_ESPConfig.boxThickness, 1.0f, 5.0f, "%.1f");
					

					break;
				}
				case 2: {
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "Misc Settings");
					ImGui::Separator();
					ImGui::Spacing();
					
					if (ImGui::Button("Print All Entities", ImVec2(150, 25))) {
						printf("[Main] Print Entities button clicked!\n");
						PrintAllEntitiesToConsole();
					}
					
				
					if (ImGui::CollapsingHeader("Misc")) {
						ImGui::Checkbox("Auto Crouch", &g_MiscConfig.autoCrouch);
					}

					break;
				}
				case 3: { 
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "Bypass Features");
					ImGui::Separator();
					ImGui::Spacing();
					static bool pbBypassEnabled = false;
					if (ImGui::Checkbox("PunkBuster Screenshot Cleaner", &pbBypassEnabled)) {
						PBBypass::SetEnabled(pbBypassEnabled);
					}
					ImGui::Text("When enabled, ESP/cheats are hidden from PunkBuster screenshots and the screenshot is saved to disk.");
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();
					static bool spoofEnabled = false;
					static char spoofName[64] = "";
					if (ImGui::Checkbox("Enable Name Spoofing", &spoofEnabled)) {
						NameSpoofer::SetEnabled(spoofEnabled);
					}
					ImGui::InputText("Spoofed Name", spoofName, sizeof(spoofName));
					if (ImGui::Button("Apply Spoofed Name")) {
						NameSpoofer::SpoofName(spoofName);
					}
					if (ImGui::Button("Restore Original Name")) {
						NameSpoofer::RestoreName();
						spoofEnabled = false;
					}
					ImGui::Text("Original Name: %s", NameSpoofer::GetOriginalName());
					break;
				}
				case 4: { 
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "Information");
					ImGui::Separator();
					ImGui::Spacing();
					
					ImGui::TextColored(ImVec4(0.0f, 0.9f, 0.5f, 1.0f), "Status: Connected");
					ImGui::Text("Version: 1.0.0");
					ImGui::Text("Build: Release");
					ImGui::Text("Architecture: x64");
					ImGui::Text("Author: intexpression");
					ImGui::Text("GitHub: https://github.com/antipaster");
					ImGui::Spacing();
					
					ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Debug Information:");
					ImGui::Spacing();
					
			
					SYSTEMTIME st;
					GetLocalTime(&st);
					ImGui::Text("Time: %02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
					ImGui::Text("Date: %02d/%02d/%04d", st.wDay, st.wMonth, st.wYear);
					
				
					MEMORYSTATUSEX memInfo;
					memInfo.dwLength = sizeof(MEMORYSTATUSEX);
					GlobalMemoryStatusEx(&memInfo);
					float memUsage = (float)memInfo.dwMemoryLoad;
					ImGui::Text("Memory Usage: %.1f%%", memUsage);
					
					
					HANDLE hProcess = GetCurrentProcess();
					PROCESS_MEMORY_COUNTERS pmc;
					if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
						ImGui::Text("Process Memory: %.1f MB", (float)pmc.WorkingSetSize / (1024 * 1024));
					}
					
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Hook Status:");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• Present Hook: %s", PresentHook ? "Active" : "Inactive");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• PreFrame Hook: %s", PreFrameHook ? "Active" : "Inactive");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• ImGui: %s", g_ImGuiInitialized ? "Initialized" : "Not Initialized");
					
					ImGui::Spacing();
					ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Game Status:");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• Game Context: %s", fb::ClientGameContext::Singleton() ? "Available" : "Not Available");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• Player Manager: %s", (fb::ClientGameContext::Singleton() && fb::ClientGameContext::Singleton()->m_clientPlayerManager) ? "Available" : "Not Available");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• Local Player: %s", (fb::ClientGameContext::Singleton() && fb::ClientGameContext::Singleton()->m_clientPlayerManager && fb::ClientGameContext::Singleton()->m_clientPlayerManager->m_localPlayer) ? "Available" : "Not Available");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "• D3D Renderer: %s", fb::DxRenderer::Singleton() ? "Available" : "Not Available");
					break;
				}
			}
			
		
			ImGui::PopStyleColor(14);
			ImGui::PopStyleVar(7);
			ImGui::End();
		} else {
			OutputDebugStringA("[Main] Menu is hidden (g_ShowMenu = false)\n");
		}
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	} else {
		OutputDebugStringA("[Main] ImGui not initialized yet\n");
	}


	if (g_ESPConfig.enabled) {

		ESP::RenderESP();

	}
	

	if (g_AimbotConfig.enabled) {
		float screenWidth = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nWidth;
		float screenHeight = (float)fb::DxRenderer::Singleton()->m_screenInfo.m_nHeight;
		fb::Vec3 center;
		center.x = screenWidth / 2.0f;
		center.y = screenHeight / 2.0f;
		center.z = 0.0f;
	
		float fovRadius = g_AimbotConfig.fov;
		fb::Color32 circleColor(0, 255, 0, 255);
		DrawCircle(center, fovRadius, circleColor);
	}

	PBBypass::OnFrame();
	NameSpoofer::OnFrame();

	__asm popad;

	return oPresent(a1, a2, a3);
}

HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
    if (g_ImGuiInitialized) ImGui_ImplDX11_InvalidateDeviceObjects();
    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (g_ImGuiInitialized) ImGui_ImplDX11_CreateDeviceObjects();
    return hr;
}

HWND GetSwapChainHWND(IDXGISwapChain* pSwapChain) {
    DXGI_SWAP_CHAIN_DESC desc;
    if (SUCCEEDED(pSwapChain->GetDesc(&desc)))
        return desc.OutputWindow;
    return nullptr;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        printf("[Main] Console enabled!\n");
		printf("[Main] Thanks for downloading BF3 Internal!\n");
		printf("[Main] https://github.com/antipaster/bf3-internal\n");
        DisableThreadLibraryCalls(hModule);

     
        PresentHook = new CVMTHookManager((DWORD**)fb::DxRenderer::Singleton()->pSwapChain);
        oPresent = (tPresent)PresentHook->dwGetMethodAddress(8);
        PresentHook->dwHookMethod((DWORD)hkPresent, 8);

        PreFrameHook = new CVMTHookManager((DWORD**)g_pBorderInputNode);
        oPreFrameUpdate = (tPreFrameUpdate)PreFrameHook->dwGetMethodAddress(27);
        PreFrameHook->dwHookMethod((DWORD)hkPreFrame, 27);
        printf("[Main] PreFrame hook installed at method 27\n");

    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        PBBypass::Shutdown();
    }
    return TRUE;
}

void PrintAllEntitiesToConsole() {
    printf("[ESP] Starting entity scan...\n");
    
    fb::ClientGameContext* g_pGameContext = fb::ClientGameContext::Singleton();
    if (!g_pGameContext) {
        printf("[ESP] No game context!\n");
        return;
    }
    
    fb::ClientPlayerManager* pPlayerManager = g_pGameContext->m_clientPlayerManager;
    if (!pPlayerManager || pPlayerManager->m_players.empty()) {
        printf("[ESP] No player manager or players!\n");
        return;
    }
    
    fb::ClientPlayer* pLocalPlayer = pPlayerManager->m_localPlayer;
    if (!pLocalPlayer) {
        printf("[ESP] No local player!\n");
        return;
    }
    
    printf("[ESP] Local player: %s (Team: %d)\n", pLocalPlayer->am_name, pLocalPlayer->m_teamId);
    
    eastl::vector<fb::ClientPlayer*> pVecCP = pPlayerManager->m_players;
    printf("[ESP] Total players: %d\n", pVecCP.size());
    
    int entityCount = 0;
    for (int i = 0; i < pVecCP.size(); i++) {
        fb::ClientPlayer* pClientPlayer = pVecCP.at(i);
        if (!pClientPlayer) continue;
        
        fb::ClientSoldierEntity* pEnemySoldier = pClientPlayer->getSoldierEnt();
        if (!pEnemySoldier) continue;
        
        entityCount++;
        printf("[ESP] Player %d: %s (Team: %d, Alive: %s, InVehicle: %s)\n", 
               i, 
               pClientPlayer->am_name, 
               pClientPlayer->m_teamId,
               pEnemySoldier->IsAlive() ? "Yes" : "No",
               pEnemySoldier->isInVehicle() ? "Yes" : "No");
        
        if (pEnemySoldier->m_replicatedController) {
            fb::Vec3 pos = pEnemySoldier->m_replicatedController->m_state.position;
            printf("[ESP]   Position: %.2f, %.2f, %.2f\n", pos.x, pos.y, pos.z);
        }
        
        if (pEnemySoldier->IsAlive()) {
            printf("[ESP]   Health: %.1f/%.1f\n", pEnemySoldier->m_health, pEnemySoldier->maxHealth());
        }
    }
    
    printf("[ESP] Total entities found: %d\n", entityCount);
}
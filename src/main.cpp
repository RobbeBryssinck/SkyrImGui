#include <spdlog/sinks/stdout_color_sinks.h>
#include <Windows.h>
#include <MinHook.h>
#include "hooks/D3D11Hooks.h"
#include "hooks/DInputHook.h"

namespace
{
	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;

		if (AllocConsole()) {
			FILE* file = nullptr;
            freopen_s(&file, "CONOUT$", "w", stdout);
            SetConsoleTitleA("SkyrImGui");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
		}
#else
		const auto level = spdlog::level::info;
#endif
		auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console->set_pattern("%^[%H:%M:%S] [%l]%$ %v");

		auto log = std::make_shared<spdlog::logger>("", spdlog::sinks_init_list{sink, console});
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
	}
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);

	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	while (!IsDebuggerPresent())
		Sleep(100);

	InitializeLog();
	logger::info("SkyrImGui");

	MH_Initialize();

	SKSE::Init(a_skse);

	D3D11Hooks::Install();
	DInputHook::Install();

	return true;
}

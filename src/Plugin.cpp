using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

#include "Plugin.h"
#include "Config.h"

namespace FastTravelTweaks {
    std::optional<std::filesystem::path> getLogDirectory() {
        using namespace std::filesystem;
        PWSTR buf;
        SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, &buf);
        std::unique_ptr<wchar_t, decltype(&CoTaskMemFree)> documentsPath{buf, CoTaskMemFree};
        path directory{documentsPath.get()};
        directory.append("My Games"sv);

        if (exists("steam_api64.dll"sv)) {
            if (exists("openvr_api.dll") || exists("Data/SkyrimVR.esm")) {
                directory.append("Skyrim VR"sv);
            } else {
                directory.append("Skyrim Special Edition"sv);
            }
        } else if (exists("Galaxy64.dll"sv)) {
            directory.append("Skyrim Special Edition GOG"sv);
        } else if (exists("eossdk-win64-shipping.dll"sv)) {
            directory.append("Skyrim Special Edition EPIC"sv);
        } else {
            return current_path().append("skselogs");
        }
        return directory.append("SKSE"sv).make_preferred();
    }

    void initializeLogging() {
        auto path = getLogDirectory();
        if (!path) {
            report_and_fail("Can't find SKSE log directory");
        }
        *path /= std::format("{}.log"sv, Plugin::Name);

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern(PLUGIN_LOGPATTERN_DEFAULT);
    }
}  // namespace FastTravelTweaks

using namespace FastTravelTweaks;

class HorsebackHooks {
        static bool CanFastTravelHorseback(uintptr_t p1) {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (player->IsOnMount()) {
                SetGMSTFloat("fFastTravelSpeedMult", Config::horsebackSpeed);
            } else {
                SetGMSTFloat("fFastTravelSpeedMult", Config::normalSpeed);
            }
            return _CanFastTravel(p1);
        }

        static bool CanFastTravelOwnHorse(uintptr_t p1) {
            auto player = RE::PlayerCharacter::GetSingleton();
            // player->lastRiddenMount isn't working, thanks bunches
            if (player->IsOnMount() || player->GetInfoRuntimeData().lastRiddenMount) {
                SetGMSTFloat("fFastTravelSpeedMult", Config::horsebackSpeed);
            } else {
                SetGMSTFloat("fFastTravelSpeedMult", Config::normalSpeed);
            }
            return _CanFastTravel(p1);
        }        

    public:
        static inline REL::Relocation<std::uintptr_t> Can_Fast_Travel_Hook{REL::ID(RELOCATION_ID(52208, 53095)), 0x183};
        static inline REL::Relocation<decltype(CanFastTravelHorseback)> _CanFastTravel;

        static void hookHorseback() {
            _CanFastTravel = SKSE::GetTrampoline().write_call<5>(Can_Fast_Travel_Hook.address(), CanFastTravelHorseback);
        }
        static void hookOwnHorse() {
            _CanFastTravel = SKSE::GetTrampoline().write_call<5>(Can_Fast_Travel_Hook.address(), CanFastTravelOwnHorse);
        }

        static void SetGMSTFloat(const char *a_setting, float a_set) {
            auto set = RE::GameSettingCollection::GetSingleton();
            if (!set) {
                return;
            }
            auto setting = set->GetSetting(a_setting);
            if (setting) {
                setting->data.f = a_set;
                logger::info("set {} to {}", a_setting, setting->data.f);
            }
        };
};

class CartSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    static CartSink* GetSingleton() {
        static CartSink instance;
        return &instance;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
                                          RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override {
        if (a_event && a_event->opening &&
            std::string_view(a_event->menuName.c_str()) == RE::DialogueMenu::MENU_NAME) {
            auto* topics = RE::MenuTopicManager::GetSingleton();
            auto speaker = topics ? topics->speaker.get() : nullptr;
            auto* actor = speaker ? speaker->As<RE::Actor>() : nullptr;
            if (actor && IsCarriageDriver(actor)) {
                HorsebackHooks::SetGMSTFloat("fFastTravelSpeedMult", Config::cartSpeed);
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }

private:
    static bool IsCarriageDriver(RE::Actor* a_actor) {
        auto* faction = RE::TESForm::LookupByID<RE::TESFaction>(0x00017F04);
        return faction && a_actor->IsInFaction(faction);
    }
};

extern "C" DLLEXPORT bool SKSEPlugin_Load(const LoadInterface *skse) {
    initializeLogging();
    logger::info("'{} {}' is loading, game version '{}'...", Plugin::Name, Plugin::VersionString, REL::Module::get().version().string());
    Init(skse);
    SKSE::AllocTrampoline(100);
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message *message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            Config::LoadConfig();
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(CartSink::GetSingleton());
            if (Config::fasterIfOwnHorse) {
                HorsebackHooks::hookOwnHorse();
            } else {
                HorsebackHooks::hookHorseback();
            }
        }
    });

    logger::info("{} has finished loading.", Plugin::Name);
    return true;
}
#include "Config.h"
#include "SimpleIni.h"

namespace FastTravelTweaks {
    void Config::LoadConfig() noexcept
    {
        logger::info("Loading config");

        CSimpleIniA ini;
        ini.SetUnicode();
        if (ini.LoadFile(R"(.\Data\SKSE\Plugins\FastTravelSpeedTweaks.ini)") != 0) {
            logger::error("Failed to Load config file.");
            return;
        }
        fasterIfOwnHorse = ini.GetBoolValue("General", "fasterIfOwnHorse");
        normalSpeed = (float) ini.GetDoubleValue("General", "normalSpeed");
        horsebackSpeed = (float) ini.GetDoubleValue("General", "horsebackSpeed");
        cartSpeed = (float) ini.GetDoubleValue("General", "cartSpeed");

        logger::info("Loaded config");
        logger::info("");
    }
}
// $Id$

#include "GlobalSettings.hh"
#include "FileContext.hh"
#include "IntegerSetting.hh"
#include "BooleanSetting.hh"
#include "StringSetting.hh"
#include "SettingsConfig.hh"
#include "ThrottleManager.hh"
#include "EnumSetting.hh"

namespace openmsx {

GlobalSettings::GlobalSettings(CommandController& commandController_)
	: commandController(commandController_)
{
	speedSetting.reset(new IntegerSetting(commandController, "speed",
	       "controls the emulation speed: higher is faster, 100 is normal",
	       100, 1, 1000000));
	pauseSetting.reset(new BooleanSetting(commandController, "pause",
	       "pauses the emulation", false, Setting::DONT_SAVE));
	powerSetting.reset(new BooleanSetting(commandController, "power",
	        "turn power on/off", false, Setting::DONT_SAVE));
	autoSaveSetting.reset(new BooleanSetting(commandController,
	        "save_settings_on_exit",
	        "automatically save settings when openMSX exits", true));
	consoleSetting.reset(new BooleanSetting(commandController, "console",
	        "turns console display on/off", false, Setting::DONT_SAVE));
	userDirSetting.reset(new StringSetting(commandController,
	        "user_directories", "list of user directories", ""));
	umrCallBackSetting.reset(new StringSetting(commandController,
	        "umr_callback", "TCL proc to call when an UMR is detected", ""));
	EnumSetting<bool>::Map cmdMap;
	cmdMap["DOS1"] = false;
	cmdMap["DOS2"] = true;
	bootSectorSetting.reset(new EnumSetting<bool>(commandController,
		"bootsector", "boot sector type for dir-as-dsk", true, cmdMap));

	throttleManager.reset(new ThrottleManager(commandController));

	getPowerSetting().attach(*this);
}

GlobalSettings::~GlobalSettings()
{
	getPowerSetting().detach(*this);
	commandController.getSettingsConfig().setSaveSettings(
		autoSaveSetting->getValue());
}

IntegerSetting& GlobalSettings::getSpeedSetting()
{
	return *speedSetting.get();
}

BooleanSetting& GlobalSettings::getPauseSetting()
{
	return *pauseSetting.get();
}

BooleanSetting& GlobalSettings::getPowerSetting()
{
	return *powerSetting.get();
}

BooleanSetting& GlobalSettings::getAutoSaveSetting()
{
	return *autoSaveSetting.get();
}

BooleanSetting& GlobalSettings::getConsoleSetting()
{
	return *consoleSetting.get();
}

ThrottleManager& GlobalSettings::getThrottleManager()
{
	return *throttleManager.get();
}

StringSetting& GlobalSettings::getUMRCallBackSetting()
{
	return *umrCallBackSetting.get();
}

StringSetting& GlobalSettings::getUserDirSetting()
{
	return *userDirSetting.get();
}

EnumSetting<bool>& GlobalSettings::getBootSectorSetting()
{
	return *bootSectorSetting.get();
}

// Observer<Setting>
void GlobalSettings::update(const Setting& setting)
{
	if (&setting == &getPowerSetting()) { // either on or off
		// automatically unpause after a power off/on cycle
		// this solved a bug, but apart from that this behaviour also
		// makes more sense
		getPauseSetting().setValue(false);
	}
}

} // namespace openmsx

// $Id$

#include "SettingsManager.hh"
#include "IntegerSetting.hh"
#include "BooleanSetting.hh"
#include "CommandController.hh"
#include "Interpreter.hh"

namespace openmsx {

// SettingsManager implementation:

SettingsManager::SettingsManager()
	: setCompleter(this),
	  settingCompleter(this),
	  toggleCommand(this),
	  commandController(CommandController::instance()),
	  interpreter(Interpreter::instance())
{
	commandController.registerCompleter(&setCompleter,     "set");
	commandController.registerCompleter(&settingCompleter, "incr");
	commandController.registerCompleter(&settingCompleter, "unset");
	commandController.registerCommand(&toggleCommand,  "toggle");
}

SettingsManager::~SettingsManager()
{
	commandController.unregisterCommand(&toggleCommand,  "toggle");
	commandController.unregisterCompleter(&settingCompleter, "unset");
	commandController.unregisterCompleter(&settingCompleter, "incr");
	commandController.unregisterCompleter(&setCompleter,     "set");
}

SettingsManager& SettingsManager::instance()
{
	static SettingsManager oneInstance;
	return oneInstance;
}

void SettingsManager::registerSetting(SettingNode& setting)
{
	const string& name = setting.getName();
	assert(settingsMap.find(name) == settingsMap.end());
	settingsMap[name] = &setting;

	SettingLeafNode* leafNode = dynamic_cast<SettingLeafNode*>(&setting);
	if (leafNode) {
		interpreter.registerSetting(*leafNode);
	}
}

void SettingsManager::unregisterSetting(SettingNode& setting)
{
	SettingLeafNode* leafNode = dynamic_cast<SettingLeafNode*>(&setting);
	if (leafNode) {
		interpreter.unregisterSetting(*leafNode);
	}
	const string& name = setting.getName();
	assert(settingsMap.find(name) != settingsMap.end());
	settingsMap.erase(name);
}

// Helper functions for setting commands

template <typename T>
void SettingsManager::getSettingNames(string& result) const
{
	for (map<string, SettingNode*>::const_iterator it =
	       settingsMap.begin();
	     it != settingsMap.end(); ++it) {
		if (dynamic_cast<T*>(it->second)) {
			result += it->first + '\n';
		}
	}
}

template <typename T>
void SettingsManager::getSettingNames(set<string>& result) const
{
	for (map<string, SettingNode *>::const_iterator it
		 = settingsMap.begin();
	     it != settingsMap.end(); ++it) {
		if (dynamic_cast<T*>(it->second)) {
			result.insert(it->first);
		}
	}
}

template <typename T>
T* SettingsManager::getByName(const string& cmd, const string& name) const
{
	SettingNode* setting = getByName(name);
	if (!setting) {
		throw CommandException(cmd + ": " + name +
		                       ": no such setting");
	}
	T* typeSetting = dynamic_cast<T*>(setting);
	if (!typeSetting) {
		throw CommandException(cmd + ": " + name +
		                       ": setting has wrong type");
	}
	return typeSetting;
}


// SetCompleter implementation:

SettingsManager::SetCompleter::SetCompleter(SettingsManager *manager_)
	: manager(manager_)
{
}

void SettingsManager::SetCompleter::tabCompletion(vector<string> &tokens) const
	throw()
{
	switch (tokens.size()) {
		case 2: {
			// complete setting name
			set<string> settings;
			manager->getSettingNames<SettingLeafNode>(settings);
			CommandController::completeString(tokens, settings);
			break;
		}
		case 3: {
			// complete setting value
			map<string, SettingNode *>::iterator it =
				manager->settingsMap.find(tokens[1]);
			if (it != manager->settingsMap.end()) {
				it->second->tabCompletion(tokens);
			}
			break;
		}
	}
}


// SettingCompleter implementation

SettingsManager::SettingCompleter::SettingCompleter(SettingsManager *manager_)
	: manager(manager_)
{
}

void SettingsManager::SettingCompleter::tabCompletion(vector<string> &tokens) const
	throw()
{
	switch (tokens.size()) {
		case 2: {
			// complete setting name
			set<string> settings;
			manager->getSettingNames<SettingLeafNode>(settings);
			CommandController::completeString(tokens, settings);
			break;
		}
	}
}


// ToggleCommand implementation:

SettingsManager::ToggleCommand::ToggleCommand(SettingsManager *manager_)
	: manager(manager_)
{
}

string SettingsManager::ToggleCommand::execute(const vector<string> &tokens)
	throw(CommandException)
{
	string result;
	switch (tokens.size()) {
	case 1: 
		// list all boolean settings
		manager->getSettingNames<BooleanSetting>(result);
		break;

	case 2: {
		BooleanSetting *boolSetting =
			manager->getByName<BooleanSetting>("toggle", tokens[1]);
		boolSetting->setValue(!boolSetting->getValue());
		break;
	}
	default:
		throw CommandException("toggle: wrong number of parameters");
	}
	return result;
}

string SettingsManager::ToggleCommand::help(const vector<string> &tokens) const
	throw()
{
	return "toggle      : list all boolean settings\n"
	       "toggle name : toggles a boolean setting\n";
}

void SettingsManager::ToggleCommand::tabCompletion(vector<string> &tokens) const
	throw()
{
	switch (tokens.size()) {
		case 2: {
			// complete setting name
			set<string> settings;
			manager->getSettingNames<BooleanSetting>(settings);
			CommandController::completeString(tokens, settings);
			break;
		}
	}
}

} // namespace openmsx

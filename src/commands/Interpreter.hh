// $Id$

#ifndef __INTERPRETER_HH__
#define __INTERPRETER_HH__

#include <Command.hh>

namespace openmsx {

class SettingLeafNode;

class Interpreter
{
public:
	static Interpreter& instance();

	virtual void init(const char* programName) = 0;
	virtual void registerCommand(const string& name, Command& command) = 0;
	virtual void unregisterCommand(const string& name, Command& command) = 0;
	virtual bool isComplete(const string& command) const = 0;
	virtual string execute(const string& command) throw(CommandException) = 0;
	virtual string executeFile(const string& filename) throw(CommandException) = 0;
	
	virtual void setVariable(const string& name, const string& value) = 0;
	virtual void unsetVariable(const string& name) = 0;
	virtual string getVariable(const string& name) const = 0;
	virtual void registerSetting(SettingLeafNode& variable) = 0;
	virtual void unregisterSetting(SettingLeafNode& variable) = 0;
};

} // namespace openmsx

#endif

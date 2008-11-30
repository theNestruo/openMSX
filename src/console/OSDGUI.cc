// $Id$

#include "OSDGUI.hh"
#include "OSDWidget.hh"
#include "OSDTopWidget.hh"
#include "OSDRectangle.hh"
#include "OSDText.hh"
#include "Display.hh"
#include "Command.hh"
#include "CommandException.hh"
#include "TclObject.hh"
#include "StringOp.hh"
#include "openmsx.hh"
#include <algorithm>
#include <cassert>

using std::string;
using std::set;
using std::vector;
using std::auto_ptr;

namespace openmsx {

class OSDCommand : public Command
{
public:
	OSDCommand(OSDGUI& gui, CommandController& commandController);
	virtual void execute(const vector<TclObject*>& tokens, TclObject& result);
	virtual string help(const vector<string>& tokens) const;
	virtual void tabCompletion(vector<string>& tokens) const;

private:
	void create   (const vector<TclObject*>& tokens, TclObject& result);
	void destroy  (const vector<TclObject*>& tokens, TclObject& result);
	void info     (const vector<TclObject*>& tokens, TclObject& result);
	void configure(const vector<TclObject*>& tokens, TclObject& result);
	auto_ptr<OSDWidget> create(const string& type, const string& name) const;
	void configure(OSDWidget& widget, const vector<TclObject*>& tokens,
	               unsigned skip);

	OSDWidget& getWidget(const string& name) const;

	OSDGUI& gui;
};


// class OSDGUI

OSDGUI::OSDGUI(CommandController& commandController, Display& display_)
	: display(display_)
	, osdCommand(new OSDCommand(*this, commandController))
	, topWidget(new OSDTopWidget())
{
}

OSDGUI::~OSDGUI()
{
	PRT_DEBUG("Destructing OSD GUI...");
}

Display& OSDGUI::getDisplay() const
{
	return display;
}

OSDWidget& OSDGUI::getTopWidget() const
{
	return *topWidget;
}

void OSDGUI::refresh() const
{
	getDisplay().repaintDelayed(40000); // 25 fps
}


// class OSDCommand

OSDCommand::OSDCommand(OSDGUI& gui_, CommandController& commandController)
	: Command(commandController, "osd")
	, gui(gui_)
{
}

void OSDCommand::execute(const vector<TclObject*>& tokens, TclObject& result)
{
	if (tokens.size() < 2) {
		throw SyntaxError();
	}
	string subCommand = tokens[1]->getString();
	if (subCommand == "create") {
		create(tokens, result);
		gui.refresh();
	} else if (subCommand == "destroy") {
		destroy(tokens, result);
		gui.refresh();
	} else if (subCommand == "info") {
		info(tokens, result);
	} else if (subCommand == "configure") {
		configure(tokens, result);
		gui.refresh();
	} else {
		throw CommandException(
			"Invalid subcommand '" + subCommand + "', expected "
			"'create', 'destroy', 'info' or 'configure'.");
	}
}

void OSDCommand::create(const vector<TclObject*>& tokens, TclObject& result)
{
	if (tokens.size() < 4) {
		throw SyntaxError();
	}
	string type = tokens[2]->getString();
	string fullname = tokens[3]->getString();
	string parentname, name;
	StringOp::splitOnLast(fullname, ".", parentname, name);
	if (name.empty()) std::swap(parentname, name);

	OSDWidget* parent = gui.getTopWidget().findSubWidget(parentname);
	if (!parent) {
		throw CommandException(
			"Parent widget doesn't exist yet:" + parentname);
	}
	if (parent->findSubWidget(name)) {
		throw CommandException(
			"There already exists a widget with this name: " +
			fullname);
	}

	auto_ptr<OSDWidget> widget = create(type, name);
	configure(*widget, tokens, 4);
	parent->addWidget(widget);

	result.setString(fullname);
}

auto_ptr<OSDWidget> OSDCommand::create(
		const string& type, const string& name) const
{
	if (type == "rectangle") {
		return auto_ptr<OSDWidget>(new OSDRectangle(gui, name));
	} else if (type == "text") {
		return auto_ptr<OSDWidget>(new OSDText(gui, name));
	} else {
		throw CommandException(
			"Invalid widget type '" + type + "', expected "
			"'rectangle' or 'text'.");
	}
}

void OSDCommand::destroy(const vector<TclObject*>& tokens, TclObject& /*result*/)
{
	PRT_DEBUG("OSDCommand::Destroy!");
	if (tokens.size() != 3) {
		throw SyntaxError();
	}
	PRT_DEBUG("OSDCommand::Destroy: gettingWidget " << tokens[2]->getString());
	OSDWidget& widget = getWidget(tokens[2]->getString());
	PRT_DEBUG("OSDCommand::Destroy: getting parent");
	OSDWidget* parent = widget.getParent();
	if (!parent) {
		throw CommandException("Can't destroy the top widget.");
	}
	PRT_DEBUG("OSDCommand::Destroy: deleting the widget");
	parent->deleteWidget(widget);
}

void OSDCommand::info(const vector<TclObject*>& tokens, TclObject& result)
{
	switch (tokens.size()) {
	case 2: {
		// list widget names
		set<string> names;
		gui.getTopWidget().listWidgetNames("", names);
		result.addListElements(names.begin(), names.end());
		break;
	}
	case 3: {
		// list properties for given widget
		const OSDWidget& widget = getWidget(tokens[2]->getString());
		set<string> properties;
		widget.getProperties(properties);
		result.addListElements(properties.begin(), properties.end());
		break;
	}
	case 4: {
		// get current value for given widget/property
		const OSDWidget& widget = getWidget(tokens[2]->getString());
		result.setString(widget.getProperty(tokens[3]->getString()));
		break;
	}
	default:
		throw SyntaxError();
	}
}

void OSDCommand::configure(const vector<TclObject*>& tokens, TclObject& /*result*/)
{
	if (tokens.size() < 3) {
		throw SyntaxError();
	}
	configure(getWidget(tokens[2]->getString()), tokens, 3);
}

void OSDCommand::configure(OSDWidget& widget, const vector<TclObject*>& tokens,
                           unsigned skip)
{
	assert(tokens.size() >= skip);
	if ((tokens.size() - skip) & 1) {
		// odd number of extra arguments
		throw CommandException(
			"Missing value for '" + tokens.back()->getString() + "'.");
	}

	for (unsigned i = skip; i < tokens.size(); i += 2) {
		string name  = tokens[i + 0]->getString();
		string value = tokens[i + 1]->getString();
		widget.setProperty(name, value);
	}
}

string OSDCommand::help(const vector<string>& tokens) const
{
	if (tokens.size() >= 2) {
		if (tokens[1] == "create") {
			return
			  "osd create <type> <widget-path> [<property-name> <property-value>]...\n"
			  "\n"
			  "Creates a new OSD widget of given type. Path is a "
			  "hierarchical name for the wiget (separated by '.')."
			  "The parent widget for this new widget must already "
			  "exist.\n"
			  "Optionally you can set initial values for one or "
			  "more properties.\n"
			  "This command returns the path of the newly created "
			  "widget. This is path is again needed to configure "
			  "or to remove the widget. It may be useful to assign "
			  "this path to a variable.";
		} else if (tokens[1] == "destroy") {
			return
			  "osd destroy <widget-path>\n"
			  "\n"
			  "Remove the specified OSD widget.";
		} else if (tokens[1] == "info") {
			return
			  "osd info [<widget-path> [<property-name>]]\n"
			  "\n"
			  "Query various information about the OSD status. "
			  "You can call this command with 0, 1 or 2 arguments.\n"
			  "Without any arguments, this command returns a list "
			  "of all existing widget IDs.\n"
			  "When a path is given as argument, this command "
			  "returns a list of available properties for that widget.\n"
			  "When both path and property name arguments are "
			  "given, this command returns the current value of "
			  "that property.";
		} else if (tokens[1] == "configure") {
			return
			  "osd configure <widget-path> [<property-name> <property-value>]...\n"
			  "\n"
			  "Modify one or more properties on the given widget.";
		} else {
			return "No such subcommand, see 'help osd'.";
		}
	} else {
		return
		  "Low level OSD GUI commands\n"
		  "  osd create <type> <widget-path> [<property-name> <property-value>]...\n"
		  "  osd destroy <widget-path>\n"
		  "  osd info [<widget-path> [<property-name>]]\n"
		  "  osd configure <widget-path> [<property-name> <property-value>]...\n"
		  "Use 'help osd <subcommand>' to see more info on a specific subcommand";
	}
}

void OSDCommand::tabCompletion(vector<string>& tokens) const
{
	if (tokens.size() == 2) {
		set<string> cmds;
		cmds.insert("create");
		cmds.insert("destroy");
		cmds.insert("info");
		cmds.insert("configure");
		completeString(tokens, cmds);
	} else if ((tokens.size() == 3) && (tokens[1] == "create")) {
		set<string> types;
		types.insert("rectangle");
		types.insert("text");
		completeString(tokens, types);
	} else if ((tokens.size() == 3) ||
	           ((tokens.size() == 4) && (tokens[1] == "create"))) {
		set<string> names;
		gui.getTopWidget().listWidgetNames("", names);
		completeString(tokens, names);
	} else {
		try {
			set<string> properties;
			if (tokens[1] == "create") {
				auto_ptr<OSDWidget> widget = create(tokens[2], "");
				widget->getProperties(properties);
			} else if (tokens[1] == "configure") {
				const OSDWidget& widget = getWidget(tokens[2]);
				widget.getProperties(properties);
			}
			completeString(tokens, properties);
		} catch (MSXException& e) {
			// ignore
		}
	}
}

OSDWidget& OSDCommand::getWidget(const string& name) const
{
	PRT_DEBUG("OSDCommand:getWidget " << name);
	OSDWidget* widget = gui.getTopWidget().findSubWidget(name);
	PRT_DEBUG("OSDCommand:getWidget " << name << " DONE");
	if (!widget) {
		PRT_DEBUG("OSDCommand:getWidget " << name << " NOT FOUND");
		throw CommandException("No widget with name " + name);
	}
	PRT_DEBUG("OSDCommand:getWidget " << name << " returning dereference");
	return *widget;
}

} // namespace openmsx

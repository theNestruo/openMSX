// $Id$

#ifndef OSDCONSOLERENDERER_HH
#define OSDCONSOLERENDERER_HH

#include "Layer.hh"
#include "FilenameSetting.hh"
#include "SettingListener.hh"
#include "openmsx.hh"
#include <memory>
#include <string>

namespace openmsx {

class IntegerSetting;
class Font;
class InputEventGenerator;
class EventDistributor;
class BooleanSetting;
class Console;
template <typename T> class EnumSetting;

class OSDConsoleRenderer : public Layer, private SettingListener,
                           private SettingChecker<FilenameSetting::Policy>
{
public:
	virtual ~OSDConsoleRenderer();
	virtual void loadBackground(const std::string& filename) = 0;
	virtual void loadFont(const std::string& filename) = 0;
	virtual unsigned getScreenW() const = 0;
	virtual unsigned getScreenH() const = 0;

protected:
	OSDConsoleRenderer(Console& console);
	void initConsole();
	bool updateConsoleRect();
	byte getVisibility() const;

	/** How transparent is the console? (0=invisible, 255=opaque)
	  * Note that when using a background image on the GLConsole,
	  * that image's alpha channel is used instead.
	  */
	static const int CONSOLE_ALPHA = 180;
	static const int BLINK_RATE = 500;
	static const int CHAR_BORDER = 4;

	enum Placement {
		CP_TOPLEFT,    CP_TOP,    CP_TOPRIGHT,
		CP_LEFT,       CP_CENTER, CP_RIGHT,
		CP_BOTTOMLEFT, CP_BOTTOM, CP_BOTTOMRIGHT
	};

	int consoleRows;
	int consoleColumns;
	std::auto_ptr<EnumSetting<Placement> > consolePlacementSetting;
	std::auto_ptr<IntegerSetting> consoleRowsSetting;
	std::auto_ptr<IntegerSetting> consoleColumnsSetting;
	std::auto_ptr<FilenameSetting> backgroundSetting;
	std::auto_ptr<FilenameSetting> fontSetting;
	std::auto_ptr<Font> font;
	bool blink;
	unsigned lastBlinkTime;
	unsigned lastCursorPosition;
	unsigned destX;
	unsigned destY;
	unsigned destW;
	unsigned destH;
	Console& console;

private:
	void adjustColRow();
	void update(const Setting* setting);
	void setActive(bool active);

	// SettingChecker
	virtual void check(SettingImpl<FilenameSetting::Policy>& setting,
	                   std::string& value);

	bool active;
	unsigned long long time;
	EventDistributor& eventDistributor;
	InputEventGenerator& inputEventGenerator;
	BooleanSetting& consoleSetting;
};

} // namespace openmsx

#endif

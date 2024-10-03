#include "JoyHandle.hh"

#include "CommandController.hh"
#include "Event.hh"
#include "IntegerSetting.hh"
#include "JoystickManager.hh"
#include "MSXEventDistributor.hh"
#include "StateChange.hh"
#include "StateChangeDistributor.hh"
#include "serialize.hh"
#include "serialize_meta.hh"
#include "build-info.hh"

#include "join.hh"
#include "ranges.hh"
#include "unreachable.hh"
#include "xrange.hh"

namespace openmsx {

/*
 * As I have no local environment properly set,
 * let's forget about State and serialization for now...
 * (less code, less problems)
 *
class JoyHandleState final : public StateChange
{
public:
	JoyHandleState() = default; // for serialize
	JoyHandleState(EmuTime::param time_, uint8_t id_,
	               uint8_t press_, uint8_t release_,
				   uint8_t analogValue_)
		: StateChange(time_), id(id_)
		, press(press_), release(release_)
		, analogValue(analogValue_) {}

	[[nodiscard]] auto getId()          const { return id; }
	[[nodiscard]] auto getPress()       const { return press; }
	[[nodiscard]] auto getRelease()     const { return release; }
	[[nodiscard]] auto getAnalogValue() const { return analogValue; }

	template<typename Archive> void serialize(Archive& ar, unsigned /*version* /) {
		ar.template serializeBase<StateChange>(*this);
		ar.serialize("id",          id,
		             "press",       press,
		             "release",     release,
					 "analogValue", analogValue);
	}

private:
	uint8_t id, press, release, analogValue;
};
REGISTER_POLYMORPHIC_CLASS(StateChange, JoyHandleState, "JoyHandleState");
*/

// TclObject JoyHandle::getDefaultConfig(JoystickId joyId, const JoystickManager& joystickManager)
// {
// 	auto buttons = joystickManager.getNumButtons(joyId);
// 	if (!buttons) return {};

// 	TclObject listA, listB;
// 	auto joy = joyId.str();
// 	for (auto b : xrange(*buttons)) {
// 		((b & 1) ? listB : listA).addListElement(tmpStrCat(joy, " button", b));
// 	}
// 	return TclObject(TclObject::MakeDictTag{},
// 		"UP",    makeTclList(tmpStrCat(joy, " -axis1"), tmpStrCat(joy, " hat0 up")),
// 		"DOWN",  makeTclList(tmpStrCat(joy, " +axis1"), tmpStrCat(joy, " hat0 down")),
// 		"LEFT",  makeTclList(tmpStrCat(joy, " -axis0"), tmpStrCat(joy, " hat0 left")),
// 		"RIGHT", makeTclList(tmpStrCat(joy, " +axis0"), tmpStrCat(joy, " hat0 right")),
// 		"A",     listA,
// 		"B",     listB);
// }

JoyHandle::JoyHandle(CommandController& commandController_,
                     MSXEventDistributor& eventDistributor_,
                     StateChangeDistributor& stateChangeDistributor_,
                     JoystickManager& joystickManager_,
                     uint8_t id_)
	: commandController(commandController_)
	, eventDistributor(eventDistributor_)
	, stateChangeDistributor(stateChangeDistributor_)
	, joystickManager(joystickManager_)
	// , configSetting(commandController, tmpStrCat("msxjoystick", id_, "_config"),
	// 	"msxjoystick mapping configuration", getDefaultConfig(JoystickId(id_ - 1), joystickManager).getString())
	, description(strCat("Panasonic FS-JH1 ", id_, "."))
	, id(id_)
{
	// configSetting.setChecker([this](const TclObject& newValue) {
	// 	this->checkJoystickConfig(newValue); });
	// // fill in 'bindings'
	// checkJoystickConfig(configSetting.getValue());
}

JoyHandle::~JoyHandle()
{
	if (isPluggedIn()) {
		JoyHandle::unplugHelper(EmuTime::dummy());
	}
}

// void JoyHandle::checkJoystickConfig(const TclObject& newValue)
// {
// 	std::array<std::vector<BooleanInput>, 6> newBindings;

// 	auto& interp = commandController.getInterpreter();
// 	unsigned n = newValue.getListLength(interp);
// 	if (n & 1) {
// 		throw CommandException("Need an even number of elements");
// 	}
// 	for (unsigned i = 0; i < n; i += 2) {
// 		static constexpr std::array<std::string_view, 6> keys = {
// 			// order is important!
// 			"UP", "DOWN", "LEFT", "RIGHT", "A", "B"
// 		};
// 		std::string_view key  = newValue.getListIndex(interp, i + 0).getString();
// 		auto it = ranges::find(keys, key);
// 		if (it == keys.end()) {
// 			throw CommandException(
// 				"Invalid key: must be one of ", join(keys, ", "));
// 		}
// 		auto idx = std::distance(keys.begin(), it);

// 		TclObject value = newValue.getListIndex(interp, i + 1);
// 		for (auto j : xrange(value.getListLength(interp))) {
// 			std::string_view val = value.getListIndex(interp, j).getString();
// 			auto bind = parseBooleanInput(val);
// 			if (!bind) {
// 				throw CommandException("Invalid binding: ", val);
// 			}
// 			newBindings[idx].push_back(*bind);
// 		}
// 	}

// 	// only change current bindings when parsing was fully successful
// 	ranges::copy(newBindings, bindings);
// }

// Pluggable
std::string_view JoyHandle::getName() const
{
	switch (id) {
		case 1: return "joyhandle1";
		case 2: return "joyhandle2";
		default: UNREACHABLE;
	}
}

std::string_view JoyHandle::getDescription() const
{
	return description;
}

void JoyHandle::plugHelper(Connector& /*connector*/, EmuTime::param /*time*/)
{
	eventDistributor.registerEventListener(*this);
	stateChangeDistributor.registerListener(*this);

	lastTime = EmuTime::zero();
	cycle = 0;
	analogValue = 128;
}

void JoyHandle::unplugHelper(EmuTime::param /*time*/)
{
	stateChangeDistributor.unregisterListener(*this);
	eventDistributor.unregisterEventListener(*this);
}


// MSXJoystickDevice
uint8_t JoyHandle::read(EmuTime::param time)
{
	checkTime(time);
	if (analogValue < 48) {
		return 1 << 2; // "LEFT"
	}
	if (analogValue >= 208) {
		return 1 << 3; // "RIGHT"
	}
	if (analogValue < 96) {
		return cycle == 1 ? 1 << 2 : 0x00; // "LEFT" or ""
	}
	if (analogValue >= 160) {
		return cycle == 1 ? 1 << 3 : 0x00; // "RIGHT" or ""
	}
	return 0x00;
}

void JoyHandle::write(uint8_t /*value*/, EmuTime::param /*time*/)
{
	/*
	 * Let's ignore this for now...
	 * (less code, less problems)
	 *
	pin8 = (value & 0x04) != 0;
	 */
}

void JoyHandle::checkTime(EmuTime::param time)
{
	if ((time - lastTime) > EmuDuration::msec(500)) {
		// longer than 500ms since last read -> change cycle
		lastTime = time;
		cycle = 1 - cycle;
	}
}


// MSXEventListener
void JoyHandle::signalMSXEvent(const Event& event,
                                 EmuTime::param time) noexcept
{
	uint8_t press = 0;
	uint8_t release = 0;

	visit(overloaded{
		[&](const JoystickAxisMotionEvent& e) {
			/*
			 * Let's ignore this for now...
			 * (less code, less problems)
			 *
			if (e.getAxis() == ((uint8_t) 0) ) {
			 */
				constexpr int SCALE = 2;
				analogValue = e.getValue() / SCALE);
			/*
			}
			 */
		},
		[](const EventBase&) { /*ignore*/ }
	}, event);

	// auto getJoyDeadZone = [&](JoystickId joyId) {
	// 	const auto* setting = joystickManager.getJoyDeadZoneSetting(joyId);
	// 	return setting ? setting->getInt() : 0;
	// };
	// for (int i : xrange(6)) {
	// 	for (const auto& binding : bindings[i]) {
	// 		if (auto onOff = match(binding, event, getJoyDeadZone)) {
	// 			(*onOff ? press : release) |= 1 << i;
	// 		}
	// 	}
	// }
	//
	// if (((status & ~press) | release) != status) {
	// 	stateChangeDistributor.distributeNew<JoyHandleState>(
	// 		time, id, press, release);
	// }
}

/*
 * Let's ignore this for now...
 * (less code, less problems)
 *
// StateChangeListener
void JoyHandle::signalStateChange(const StateChange& event)
{
	const auto* jhs = dynamic_cast<const JoyHandleState*>(&event);
	if (!jhs) return;
	if (jhs->getId() != id) return;

	analogValue = narrow_cast<uint8_t>(std::clamp(analogValue + jhs->getAnalogValue(), 0, 255));
}

void JoyHandle::stopReplay(EmuTime::param time) noexcept
{
	uint8_t newStatus = JOY_UP | JOY_DOWN | JOY_LEFT | JOY_RIGHT |
	                    JOY_BUTTONA | JOY_BUTTONB;
	if (newStatus != status) {
		uint8_t release = newStatus & ~status;
		stateChangeDistributor.distributeNew<JoyHandleState>(
			time, id, uint8_t(0), release);
	}
}
 */


/*
 * Let's ignore serialization for now...
 * (less code, less problems)
 *
template<typename Archive>
void JoyHandle::serialize(Archive& ar, unsigned /*version* /)
{
	ar.serialize("status",      status,
				 "lastTime",    lastTime,
				 "cycle",       cycle,
				 "analogValue", analogValue);
	if constexpr (Archive::IS_LOADER) {
		if (isPluggedIn()) {
			plugHelper(*getConnector(), EmuTime::dummy());
		}
	}
	// no need to serialize 'pin8'
}
INSTANTIATE_SERIALIZE_METHODS(JoyHandle);
 */
REGISTER_POLYMORPHIC_INITIALIZER(Pluggable, JoyHandle, "JoyHandle");

} // namespace openmsx

#include "CPURegs.hh"
#include "serialize.hh"

namespace openmsx {

// version 1: Initial version.
// version 2: Replaced 'afterEI' boolean with 'after' byte
//            (holds both afterEI and afterLDAI information).
template<typename Archive>
void CPURegs::serialize(Archive& ar, unsigned version)
{
	ar.serialize("af",  AF_.w,
	             "bc",  BC_.w,
	             "de",  DE_.w,
	             "hl",  HL_.w,
	             "af2", AF2_.w,
	             "bc2", BC2_.w,
	             "de2", DE2_.w,
	             "hl2", HL2_.w,
	             "ix",  IX_.w,
	             "iy",  IY_.w,
	             "pc",  PC_.w,
	             "sp",  SP_.w,
	             "i",   I_);
	byte r = getR();
	ar.serialize("r",   r);  // combined R_ and R2_
	if (ar.isLoader()) setR(r);
	ar.serialize("im",   IM_,
	             "iff1", IFF1_,
	             "iff2", IFF2_);

	assert(isSameAfter());
	if (ar.versionBelow(version, 2)) {
		bool afterEI = false; // initialize to avoid warning
		ar.serialize("afterEI", afterEI);
		clearNextAfter();
		if (afterEI) setAfterEI();
	} else {
		ar.serialize("after", afterNext_);
	}
	copyNextAfter();

	ar.serialize("halt", HALT_);
}
INSTANTIATE_SERIALIZE_METHODS(CPURegs);

} // namespace openmsx

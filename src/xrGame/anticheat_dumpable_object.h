#ifndef ANTICHEAT_DUMPABLE_OBJECT
#define ANTICHEAT_DUMPABLE_OBJECT

class IAnticheatDumpable
{
public:
	virtual shared_str const 	GetAnticheatSectionName	() const { return ""; }
};

#endif //#ifndef ANTICHEAT_DUMPABLE_OBJECT

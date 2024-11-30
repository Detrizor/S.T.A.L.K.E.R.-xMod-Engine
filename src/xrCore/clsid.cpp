#include "stdafx.h"
#pragma hdrstop

XRCORE_API void __stdcall CLSID2TEXT(CLASS_ID id, LPSTR text)
{
    text[8] = 0;
    for (int i = 7; i >= 0; i--) { text[i] = char(id & 0xff); id >>= 8; }
}

#ifdef TARGET_PC
/* Native declarations use float; retain the original IEEE-754 bit patterns. */
float MSL_TrigF_80400770[] = { __builtin_nanf("0x7fffff") };
float MSL_TrigF_80400774[] = { __builtin_inff() };
#else
int MSL_TrigF_80400770[] = { 0x7FFFFFFF };
int MSL_TrigF_80400774[] = { 0x7F800000 };
#endif

#include NAVBOT_PCH_FILE
#include "concmd_args.h"

bool SVCommandArgs::HasArg(const char* arg) const
{
    for (auto& string : m_args)
    {
        const char* ptr = string.c_str();

        if (ke::StrCaseCmp(ptr, arg) == 0)
        {
            return true;
        }
    }

    return false;
}

#include "ThemeManager.h"

namespace freedaw {

ThemeManager& ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

} // namespace freedaw

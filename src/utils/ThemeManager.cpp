#include "ThemeManager.h"

namespace OpenDaw {

ThemeManager& ThemeManager::instance()
{
    static ThemeManager mgr;
    return mgr;
}

} // namespace OpenDaw

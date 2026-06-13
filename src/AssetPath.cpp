#include "AssetPath.hpp"
#include <SDL3/SDL.h>
#include <sys/stat.h>

namespace
{
    bool fileExists(const std::string &path)
    {
        struct stat st;
        return ::stat(path.c_str(), &st) == 0 && (st.st_mode & S_IFREG);
    }
}

namespace AssetPath
{

std::string resolve(const std::string &relative)
{
    if (relative.empty())
    {
        return relative;
    }
    // An already-absolute path is returned untouched. SDL paths used at the
    // call sites are all relative, so this is mostly a defensive guard.
    if (relative.front() == '/')
    {
        return relative;
    }

    const char *base = SDL_GetBasePath();
    if (base != nullptr && base[0] != '\0')
    {
        std::string baseDir(base);

        // Tarball / Finder launch: assets sit next to the binary.
        std::string nearBinary = baseDir + relative;
        if (fileExists(nearBinary))
        {
            return nearBinary;
        }

        // Source build via `make`: bin/coremetrics, assets at repo root.
        std::string repoRoot = baseDir + "../" + relative;
        if (fileExists(repoRoot))
        {
            return repoRoot;
        }

        // Homebrew installs the binary into <prefix>/bin/coremetrics and the
        // shared assets into <prefix>/share/coremetrics/{assets,base.xml}.
        std::string prefixShare = baseDir + "../share/coremetrics/" + relative;
        if (fileExists(prefixShare))
        {
            return prefixShare;
        }
    }

    return relative;
}

}

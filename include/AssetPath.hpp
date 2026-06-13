#ifndef __ASSET_PATH_HPP__
#define __ASSET_PATH_HPP__

#include <string>

// Resolves a relative asset filename to an absolute filesystem path. The
// binary used to assume the runtime cwd was the repo root, which broke
// every launch path except `./bin/coremetrics` from a fresh `make`. After
// a Homebrew install or a downloaded-tarball launch from Finder, the cwd
// is wrong and assets show up as a black window because base.xml and
// font.ttf both fail to load.
//
// resolveAsset(relative) probes, in order:
//   1. SDL_GetBasePath() + "/" + relative          (binary directory)
//   2. SDL_GetBasePath() + "/../share/coremetrics/" + relative
//      (Homebrew prefix layout: bin/coremetrics + share/coremetrics/assets)
//   3. relative as-is                              (current working dir)
//
// The first existing path wins. Returns the original string when none
// match so the caller's own error path still fires with the same text it
// used to print.
namespace AssetPath
{
    std::string resolve(const std::string &relative);
}

#endif

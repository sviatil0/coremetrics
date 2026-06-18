# Nix derivation for CoreMetrics.
#
# Usage:
#   nix-build packaging/nix/default.nix
#
# Or as an overlay:
#   final: prev: { coremetrics = final.callPackage ./packaging/nix { }; }
#
# TODO: when submitting upstream to nixpkgs, the maintainer will need to
# add a hash for fetchFromGitHub by running:
#   nix-prefetch-url --unpack \
#     https://github.com/sviatil0/coremetrics/archive/v0.2.18.tar.gz
# and replacing REPLACE_WITH_NIX_SRC_HASH below.

{ lib
, stdenv
, fetchFromGitHub
, pkg-config
, sdl3
, sdl3-ttf
, sdl3-image
, freetype
, harfbuzz
, darwin
}:

stdenv.mkDerivation rec {
  pname = "coremetrics";
  version = "0.2.18";

  src = fetchFromGitHub {
    owner = "sviatil0";
    repo = "coremetrics";
    rev = "v${version}";
    # `nix-prefetch-url --unpack <tag tarball>` to generate.
    sha256 = "REPLACE_WITH_NIX_SRC_HASH";
  };

  nativeBuildInputs = [ pkg-config ];

  buildInputs = [
    sdl3
    sdl3-ttf
    sdl3-image
    freetype
    harfbuzz
  ] ++ lib.optionals stdenv.isDarwin (with darwin.apple_sdk.frameworks; [
    IOKit
    CoreFoundation
  ]);

  enableParallelBuilding = true;

  buildPhase = ''
    runHook preBuild
    mkdir -p obj bin
    make bin/coremetrics \
      CXXFLAGS="-std=c++23 -Wall -O2 -I ./include $(pkg-config --cflags sdl3 sdl3-ttf sdl3-image)" \
      LDFLAGS="$(pkg-config --libs sdl3 sdl3-ttf sdl3-image)"
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    install -D -m 0755 bin/coremetrics $out/bin/coremetrics
    install -D -m 0644 base.xml $out/share/coremetrics/base.xml
    mkdir -p $out/share/coremetrics/assets
    cp -r assets/* $out/share/coremetrics/assets/
    install -D -m 0644 man/coremetrics.1 $out/share/man/man1/coremetrics.1 || true
    runHook postInstall
  '';

  meta = with lib; {
    description = "Cross-platform desktop system metrics monitor (CPU / RAM / GPU / processes)";
    homepage = "https://github.com/sviatil0/coremetrics";
    license = licenses.lgpl21Only;
    platforms = platforms.unix;
    maintainers = [ ];
    mainProgram = "coremetrics";
  };
}

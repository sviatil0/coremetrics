{
  description = "CoreMetrics: cross-platform desktop system metrics monitor on raw SDL3 surfaces.";

  # TODO: pin nixpkgs to a stable channel before public consumption. `nixos-unstable`
  # is the right channel while SDL3 packaging is still landing across distros.
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        coremetrics = pkgs.callPackage ./default.nix { };
      in
      {
        packages = {
          default = coremetrics;
          coremetrics = coremetrics;
        };

        apps.default = {
          type = "app";
          program = "${coremetrics}/bin/coremetrics";
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ coremetrics ];
          packages = with pkgs; [ gnumake gcc13 ];
        };
      });
}

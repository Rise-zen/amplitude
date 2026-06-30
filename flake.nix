{
  description = "amplitude — a tiny C++ system-metrics daemon";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    let
      mkModule = pkgs: { config, lib, ... }:
        let
          cfg = config.services.amplitude;
        in {
          options.services.amplitude = {
            enable = lib.mkEnableOption "amplitude system-metrics daemon";
            package = lib.mkOption {
              type = lib.types.package;
              default = self.packages.${pkgs.system}.default;
              description = "amplitude package to use.";
            };
            output = lib.mkOption {
              type = lib.types.str;
              default = "/tmp/amplitude.json";
              description = "Path the daemon writes the metrics JSON to.";
            };
            interval = lib.mkOption {
              type = lib.types.int;
              default = 1000;
              description = "Poll interval in milliseconds.";
            };
          };

          config = lib.mkIf cfg.enable {
            systemd.user.services.amplitude = {
              Unit = {
                Description = "amplitude — system metrics daemon";
                PartOf = [ "graphical-session.target" ];
                After = [ "graphical-session.target" ];
              };
              Service = {
                ExecStart = "${cfg.package}/bin/amplitude --out ${cfg.output} --interval ${toString cfg.interval}";
                Restart = "on-failure";
                RestartSec = 2;
              };
              Install.WantedBy = [ "graphical-session.target" ];
            };
          };
        };
    in
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        amplitude = pkgs.stdenv.mkDerivation {
          pname = "amplitude";
          version = "0.1.0";
          src = ./.;
          nativeBuildInputs = [ pkgs.meson pkgs.ninja pkgs.pkg-config ];
          meta = with pkgs.lib; {
            description = "Tiny C++ system-metrics daemon (CPU, memory, network, battery)";
            homepage = "https://github.com/Rise-zen/amplitude";
            license = licenses.mit;
            platforms = platforms.linux;
            mainProgram = "amplitude";
          };
        };
      in {
        packages.default = amplitude;
        packages.amplitude = amplitude;
        apps.default = {
          type = "app";
          program = "${amplitude}/bin/amplitude";
        };
        devShells.default = pkgs.mkShell {
          packages = [ pkgs.meson pkgs.ninja pkgs.pkg-config pkgs.gcc pkgs.clang-tools ];
        };
        homeManagerModules.default = mkModule pkgs;
        nixosModules.default = mkModule pkgs;
      });
}

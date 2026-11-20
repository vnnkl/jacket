# NixOS / Nix: `nix-shell` gives everything flash.sh needs (esptool, curl, picocom for the serial log).
{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  packages = with pkgs; [ esptool curl picocom ];
}

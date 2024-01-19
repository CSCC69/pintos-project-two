{
  description = "Pintos Development Environment";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    nixpkgs-bochs.url = "github:nixos/nixpkgs?rev=585bf4039a89cbb47a12c5e00ab820ac56cec6c1";
    nixpkgs-gdb.url = "github:nixos/nixpkgs?rev=d6d07f262b171bb1e415e1a06c52288af056a98d";
  };

  outputs = { self, nixpkgs, nixpkgs-bochs, nixpkgs-gdb }:
  let
      pkgs = import nixpkgs {
        system = "x86_64-linux";
      };
      /*pkgs-bochs = import nixpkgs-bochs {
        system = "x86_64-linux";
      };*/
      pkgs-gdb = import nixpkgs-gdb {
        system = "x86_64-linux";
      };
  in {
    devShells."x86_64-linux".default = pkgs.mkShell {
      packages = with pkgs; [
        bear
        clang-tools
        gcc
        perl
        qemu
        libclang.python
        python3
        ctags
        cscope
      ] ++ [
        #pkgs-bochs.bochs
        pkgs-gdb.gdb
      ] ++ [
        (pkgs.callPackage ./bochs-plain.nix { wxGTK32 = pkgs.wxGTK32; })
        #(pkgs.callPackage ./bochs-with-debugger.nix { wxGTK32 = pkgs.wxGTK32; })
      ];
    };
  };
}

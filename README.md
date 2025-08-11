# Welcome to mcpkg a Minecraft package management tool

### WARNING mcpkg is under heavy development. New modules are being added rapidly and the codebase isn’t fully stable yet. Expect breaking changes as we iterate. 
 
### Current Sprint: *(Aug 10 -- Aug 20th)*
* **CONTAINER:** Done
* **FILE SYSTEMS(FS)<:** Done and tested on unix like system's
* **CRYPTO:** Done
* **MATH:** Done additions and tests might come as needed  
* **MC:** Refactor in progress depends on 
* **PACKAGES:** Refactor in progress depends on mc
* **API:** Refactor in progress depends on Packages AND PROVIDERS
* **API:** Refactor in progress depends on Packages AND API(Client) 
 
[Feel free to checkout last sprints commit](https://github.com/container-craft/mcpkg/commit/459e419dc0d5d9b0f8234a7738f6db09b6632a67)


mcpkg is a cross-platform package manager for Minecraft mods, inspired by Linux package managers like apt and opkg.

It provides a consistent command-line interface for:

* Installing mods from multiple providers (e.g., Modrinth, CurseForge(once they give me a testing key.....), Hangar(soon))
* Updating your mod cache to the latest versions
* Upgrading installed mods while respecting dependencies
* Activating mods directly into your Minecraft installation
* Managing global settings for Minecraft base path, version, and mod loader

On Linux, activation uses symlinks to avoid duplicating files.

On Windows, activation uses file copies to avoid permission issues. (Never tried this)

This makes it easy to manage and swap mod sets without cluttering your .minecraft folder.

mcpkg is designed to be deterministic, reproducible, and script-friendly, so you can integrate it into CI/CD pipelines or automated modpack builders. This is what the project was made for. also checkout [container_craft the other end of this project](https://github.com/container-craft/container_craft).

[Join Discord](https://discord.gg/bWWfbW3dBa)

####  Environment (optional)

You can override defaults via env vars:

```shell
export MC_BASE=/home/someuser/.minecraft
export MC_VERSION=1.21.8
export MC_LOADER=fabric
export MCPKG_CACHE=/var/cache/mcpkg
```

#### update the local cache for the chosen version/loader
```shell
mcpkg update -v "$MC_VERSION" -l "$MC_LOADER"
```

#### search & show from cache
```shell
mcpkg cache search sodium
mcpkg cache show sodium
```

#### install / remove
```shell
mcpkg install sodium
mcpkg remove sodium
```

#### set globals (persisted)
```shell
mcpkg global mcbase ~/.minecraft
mcpkg global loader fabric
mcpkg global version 1.21.8
```

#### activate into Minecraft base (symlink on Unix, copy on Windows)

```shell
mcpkg activate -v "$MC_VERSION" -l "$MC_LOADER"
```


## Build dependencies

I have only tested this on Debian Sid.

mcpkg uses libsodium for hashing/signing. Require libsodium ≥ 1.0.12 (for crypto_generichash_blake2b and crypto_sign_ed25519).

**Debian Based**
 ```shell
sudo apt-get -y install git build-essential cmake pkg-config \
  libzstd-dev libcjson-dev libcurl4-openssl-dev libmsgpack-c-dev \
  libsodium-dev
```

**Fedora**

```
sudo dnf install -y \
  gcc gcc-c++ cmake pkgconfig \
  zstd-devel cjson-devel libcurl-devel msgpack-devel \
  libsodium-devel
```
 

**Arch**
```shell
sudo pacman -S --needed \
  base-devel cmake pkgconf \
  zstd cjson curl msgpack-c \
  libsodium
```

**OSX Homebrew**
```shell
brew install cmake zstd cjson curl msgpack libsodium
```

**Windows**
The best I can do for you is provide links to the software that is needed. Do your research
* [zstd](https://facebook.github.io/zstd/)
* [libcurl](https://curl.se/windows/)
* [libcjson](https://github.com/DaveGamble/cJSON/blob/master/README.md)
* [libmsgpack-c](https://msgpack.org/)
* [libsodium](https://doc.libsodium.org/installation#pre-built-libraries)


## Building
```shell
git clone https://github.com/container-craft/mcpkg.git
mkdir build
cd build 
cmake --build ../ --target all -j$(nproc)
```


## Installing
``` 
cmake --build ../ --target install
```


## Testing 
``` 
cmake --build ../ -DTST_BUILD=ON -DTST_VERBOSE=ON --target tests \
./tests/libmcpkg_tst/tst_libmcpkg
```

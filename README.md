## Welcome to mcpkg a Minecraft package management tool
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
./mcpkg install sodium
./mcpkg remove sodium
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


## Building

## build time dependencies
```shell
sudo apt-get install git build-essential cmake pkg-config libzstd-dev libcjson-dev libcurl4-openssl-dev libmsgpack-c-dev
```

#### build it
```shell
git clone https://github.com/container-craft/mcpkg.git
mkdir build
cd build 
cmake --build ../ --target all -j $(nproc)
``

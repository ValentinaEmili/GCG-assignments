#!/bin/bash
set -e -o pipefail

# List of the packages to be removed
echo "Won't remove: \"wget g++ g++-11 gdb make ninja-build rsync zip software-properties-common lsb-release\", you have to remove them yourself if you don't want them anymore!"
packages=(
  kitware-archive-keyring
  cmake
  libassimp-dev
  vulkan-sdk
  dpkg-dev
  vulkan-tools
  libvulkan1
  libvulkan-dev
  vulkan-utils
  libvulkan1-dbgsym
  mesa-vulkan-drivers
  libgl1-mesa-dev
  amdvlk
  libxi-dev
  libxrandr-dev 
  libxinerama-dev
  libxcursor-dev
  libwayland-dev
  libxkbcommon-dev
  wayland-protocols
  extra-cmake-modules
  xorg-dev
)

# Loop over the packages and attempt to remove them
for pkg in "${packages[@]}"; do
  if dpkg -l | grep -q "^ii  $pkg"; then
    sudo apt remove -y $pkg
    echo "Removed $pkg"
  else
    echo "Package $pkg not found, skipping."
  fi
done

# Remove the graphics drivers and related repositories
cpu=$(lscpu)
gpu=$(lspci | grep -i --color 'vga\|3d\|2d')

if echo "$gpu" | grep -q -i "nvidia\|intel"; then
    sudo add-apt-repository -r -y ppa:oibaf/graphics-drivers
elif echo "$cpu" | grep -q -i "amd"; then
    sudo rm /etc/apt/sources.list.d/amdvlk.list || true
    sudo apt-key del `sudo apt-key list | grep -B 1 "repo.radeon.com" | head -n 1 | cut -d '/' -f 2 | cut -d ' ' -f 1` || true
fi

# Remove the general repositories
sudo add-apt-repository -r -y "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
sudo add-apt-repository -r -y ppa:ubuntu-toolchain-r/test
sudo rm /etc/apt/sources.list.d/lunarg-vulkan-$(lsb_release -cs).list || true

# Remove the keys
sudo rm /etc/apt/trusted.gpg.d/kitware.gpg || true
sudo rm /etc/apt/trusted.gpg.d/lunarg.asc || true

# Update system
sudo apt update

echo "You may now additionally run \"sudo apt autoremove\" to clean up leftover packages which are no longer used if you want."

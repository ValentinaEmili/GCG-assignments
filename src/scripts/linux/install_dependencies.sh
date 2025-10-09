#!/bin/bash
set -e -o pipefail

# Add repositories first
# For Kitware's CMake package
sudo add-apt-repository -y -n "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main"
# For Ubuntu test toolchain for g++-11
sudo add-apt-repository -y -n ppa:ubuntu-toolchain-r/test

# For Vulkan SDK
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-$(lsb_release -cs).list http://packages.lunarg.com/vulkan/lunarg-vulkan-$(lsb_release -cs).list

# Import all necessary keys
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc

# Add repositories based on the CPU and GPU
cpu=$(lscpu)
gpu=$(lspci | grep -i --color 'vga\|3d\|2d')

sudo add-apt-repository -y -n ppa:oibaf/graphics-drivers

# Update and install packages
sudo apt update
sudo apt install -y wget g++ gdb make ninja-build rsync zip software-properties-common libxi-dev libxrandr-dev libxinerama-dev libxcursor-dev lsb-release kitware-archive-keyring cmake libassimp-dev g++-11 dpkg-dev vulkan-sdk vulkan-utils libvulkan-dev libvulkan1 libvulkan1-dbgsym --fix-missing

sudo apt install -y mesa-vulkan-drivers libgl1-mesa-dev mesa-utils
sudo apt install -y libwayland-dev libxkbcommon-dev wayland-protocols extra-cmake-modules xorg-dev xvfb imagemagick

sudo apt upgrade -y
sudo apt clean all

# Set gcc/g++ version
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 60 --slave /usr/bin/g++ g++ /usr/bin/g++-11

# Test Vulkan installation
echo ""
echo "Now running \"vulkaninfo\" to see if vulkan has been installed successfully:"
vulkaninfo

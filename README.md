# Prerequesites

## Visual Studio 2022

**Download and install `Visual Studio Community 2022`** from [https://visualstudio.microsoft.com/vs/](https://visualstudio.microsoft.com/vs/) with the C++ Development Tools selected.

## CMake

**Download and install `CMake 3.24` or higher** from [https://cmake.org/download/](https://cmake.org/download/).

## VULKAN

**Download and install the `Vulkan SDK 1.3.216.0` or newer** from [https://vulkan.lunarg.com/](https://vulkan.lunarg.com/).

## Linux requirements

If you open this framework on Linux, please first run the script `./scripts/linux/install_dependencies.sh` which will install most necessary dependencies except for VS Code and its extensions.


## Open Project

# Download the libraries

Execute the `download_assets_and_libs`script, depending on your operating system. This will download the necessary shared library files.

# In VS 2022

1. Open the Framework as folder in VS 2022 (right-click in the folder containing the `CMakeLists.txt` and select "Open with Visual Studio").
2. The contained `CMakeLists.txt` will automatically be used by Visual Studio to load the project and the CMake configuration process is started immediately and the CMake output should be visible automatically. If not, please right-click on `CMakeLists.txt` and select "Configure project_name".

# Alternative: Using your default generator

We supplied you with a make.bat file for windows and a makefile for MacOS/Linux. You can double click make.bat or execute `make debug` or `make release` to build with your default cmake generator (for example Visual Studio 2022 or Xcode). Project files can be found in `_project` afterwards. You can edit the files to change the generator for example to Xcode by adding a `-G "Xcode"` to the cmake generation command (first line). You can also use CLion or VS Code as an IDE, but on Windows it will require the installation of Visual Studio 2022 nonetheless, as you need the accompanying MSVC compiler.

# Project Structure

Shader Code is located in the `assets/shaders/` folder, and the application will try to find any shaders inside this folder. You should edit and add shaders only inside this folder in the root of the project.
Source Code is located in the `src` folder, please implement your tasks there and in the relevant shaders.

# Errors and FAQ

Please follow the instructions of this readme carefully if something does not work.
If everything was done correctly, please look if a new checkout of the Repo in a different location helps.
Sometimes CMake caches can interfere for example. Sometimes project caches can also lead to problems.
Windows path length is a major problem often, it is restricted to 260 characters and you should place the repository into a short path
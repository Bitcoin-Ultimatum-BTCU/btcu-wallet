# Windows Build Instructions and Notes

The commands in this guide should be executed in a Terminal application.
You can run it by clicking `Win + X` and selecting `Windows Powershell (Admin)` in Windows 10 or via `Win + R` and typing `Powershell` in the run window on earlier version.

We also use the [Windows Terminal app](https://www.microsoft.com/en-us/p/windows-terminal/9n0dx20hk701) from the Microsoft Store. You can check it and probably you'll like it.

## Prerequisites
In order to build BTCU on Mac it is required to have a MacOS computer, with at least 15GB of free disk space (some of the dependencies will require more space, but you may have these installed already) without the database.

## Important notes and caveats

In is recommended to turn off Windows Defender (Real-time protection) or other AntiVirus program while a build process goes on (such as VCPKG dependencies installation, Berkley DB build and build of the BTCU project). 
For the Defender you can find the option here by clicking `WIN + X`, selecting Settings, then select an option Update & Security, Windows Security, Virus & thread protection. In opened window of the Windows Defenrer click on a `Manage settings` in a `Virus & thread protection settings` section and turn off `Real-time protection` option. Please take a note: at first time this option will be automatically restored after 10 minutes. At second time of turning it off it will be disabled untill next reboot.

Also important: We use everywhere windows version 0x0A00 to build the project for Windows 10. If you need use an older one you will have to download an appopriate version of the Windows Developer Kit and change the version code here and everywhere in the project where we use 0x0A00 (or 0x0A000001). We do have a plan to add simpler Windows API selection process in the apcoming releases.

Important too: Do not use spaces and other line breaking simbols in a path to the VCPKG folder and the BTCU folder because it'll cause a compilation problems. Also please make sure you don't have non-UTF8 simbols in the path to the VCPKG folder due to it may cause build problems.

Also we've reported about the errors in MSVC build that caused by apostoroph simbols and analogs in windows profile names of persons who tries to build the project. Sigh. Please be carefull about this misbehavour in Windows environment.

If you encountered crushes of MSVC please check twice did you really turned of the Windows Defender.

## Preparation
The next step is required only if you don't have an already instaled Visual Studio (build on VS Code will be in upcoming releases). The short name for the Microsoft Visual Studio is MSVC. It will be used in the instruction later.

1. You can install lastest Visual Studio from [here](https://visualstudio.microsoft.com/ru/). The free one is the Community Edition. Please read carefully licensing agreement for the CE version if you will use it.

2. Then you will need to install the Git for Windows. One of the possible solutions is to use [Git-SCM](https://git-scm.com/download/win). But you can use another git program. Visual Studio has it's own git extention. The key point is to have git.exe for the command line. Make sure the installed program is added to the environment PATH.

3. Next you will have to install a few Visual Studio extensions. In order to do this run Visual Studio Installer (you can find it via search panel).

Click on Modify button on Visual Studio Build Tools row and select next option:
- Visual C++ build tools (in Workloads tab)

In Installation details for the `Visual C++ build tools` option select next components from the `Optional` section:
- Windows 10 SDK
- Visual C++ tools for CMake
- Testing tools core features - Build Tools

(Optional) In the `Individual components` select:
- Windows Universal C Runtime

Hint: With this will be installed MSBuild. You may want to read more about it [here](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-160). Also will be added CMake tools package with build-in Ninja generator. You can read more about it [here](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-160).

4. The next step is the VCPKG installation. (Read more about VCPKG [here](https://docs.microsoft.com/en-us/cpp/build/vcpkg?view=msvc-160).)
In order to do it please navigate to desired installation folder and run followed commands:

```cmd
    git clone https://github.com/Microsoft/vcpkg.git
    cd vcpkg
    ./bootstrap-vcpkg.bat        # Installation script
    ./vcpkg integrate install    # Integration with MSVC

    setx VCPKG_ROOT "${PWD}" /M  # Set env variable VCPKG_ROOT
    setx VCPKG_DEFAULT_TRIPLET "x64-windows-static" /M
        # The default supported build configuration is
        # x64 build with static dependencies
    refreshenv                   # Update environment variables
```

At this point you should have the system environment variable VCPKG_ROOT but sometimes it is recommended to reboot the PC in order to get the change applied everywere.

Hint: You can also check the complete explanation of the installation process of the VCPKG [here](https://github.com/microsoft/vcpkg).

## Downloading the BTCU project

You can clone the BTCU source code via installed prevously git cmd util, for that open desired folder for the project and run within it:
```cmd
    git clone https://github.com/bitcoin-ultimatum-btcu/btcu-wallet
    cd btcu
```

Or you can download in manually from [the GitHub project page](https://github.com/bitcoin-ultimatum-btcu/btcu-wallet) and unpack it to the project folder.


## Dependencies

To install project dependencies open the project folder in the Powershell terminal and run:
```cmd
    vcpkg install --triplet x86-windows $(Get-Content -Path depends\vcpkg\vcpkg-packages-prerequisites-86.txt).split()

    vcpkg install --triplet x64-windows-static $(Get-Content -Path depends\vcpkg\vcpkg-packages.txt).split()
```

Note: If you didn't add the VCPKG folder to the PATH environment variable you will have to specify the full path to vcpkg.exe.

Important: If the process takes more than 30 minutes probably something goes wrong. In that case you can simply re-run the command.

### Boost installation

In order to build Boost firstly please open the project folder in Powershell terminal and run: 
```cmd
    (new-object System.Net.WebClient).DownloadFile("https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.zip","${PWD}\depends\packages\static\boost_1_71_0.zip")

    cd depends\packages\static\  # The folder where
                                 # the Boost files would be

    # This command is only supported on PowerShell v5.1+
    # If you don't have it you can simply unpack 
    # the package manually
    Expand-Archive -Path boost_1_71_0.zip -DestinationPath $PWD

    cd boost_1_71_0

    ./bootstrap.bat              # Prepare installer
```

The next command would be compilation:
```cmd
    ./b2 --variant=debug,release address-model=64 architecture=x86 --build-type=complete -j %NUMBER_OF_PROCESSORS% --with-regex --with-test --with-filesystem --with-date_time --with-random --with-system --with-thread --with-program_options --with-chrono --with-fiber --with-log --with-context --with-math stage link=static runtime-link=static threading=multi define=BOOST_USE_WINAPI_VERSION=0x0A00 define=_SECURE_SCL=0 define=_CRT_SECURE_NO_DEPRECATE define=_CRT_SECURE_NO_WARNINGS asynch-exceptions=on exception-handling=on extern-c-nothrow=off define=BOOST_THREAD_PLATFORM_WIN32 define=BOOST_LOG_BUILDING_THE_LIB=1
```
, where %NUMBER_OF_PROCESSORS% -- is number of cores you have in youre CPU on the PC (for example, 8).

Important: We use everywhere windows version 0x0A00 to build the project for Windows 10. If you need use an older one you will have to download an appopriate version of the Windows Developer Kit and change the version code here and everywhere in the project where we use 0x0A00 (or 0x0A000001). We do have a plan to add simpler Windows API selection process in the apcoming releases. 

### Berkeley DB

Since BSD tar was added in Windows in 2018 the installation process is pretty easy. Navigate to project folder in the Powershell Terminal and run commands:
```cmd
    tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C depends/packages/static
    cd ./depends/packages/static/db-18.1.32/build_windows
    explorer ./
```
It will open new Explorer windows with the BerkeleyDB content. Next open the file `Berkeley_DB_vs2015.sln` via Visual Studio. It may suggest to update the project to your current Visual Studio version. Accept it.
If MSVC not suggested migration while opening the Berkeley DB project please click on a Project btn on the top MSVC menu (it goes after buttons File / Edit and View). In Project's dropdown menu please select Retarget solution option and accept the suggested default values.

Next you will have to select a build scheme Static Debug (the Debug scheme will be selected as a default, you may find it near Run - the green arrow - button, right behind the build platform selection with x64 default value which is good) and click on F6. It will start the entire solution build. Some projects may throw errors but we need only DB project and his dependencies so it can be ignored.

After the build finished please select a build Scheme Static Release and press F6 to run build. If you don't plan to run an application in Debug mode you may skip Static Debug builing step.

## Configuring BTCU

Open the project folder in MSVC. It should automatically start CMake build process. But if not you can just open roon CMakeLists.txt, wait a few seconds while the project will be inited (it will add build tab on a top menu and few other parameters) then open Project button from the top menu and select `Generate Cache` option.

It will run a console command like:
"%PATH_TO_CMAKE\CMake\bin\cmake.exe" -G "Ninja" -DCMAKE_INSTALL_PREFIX:PATH="%PATH_TO_PROJECT\out\install\x64-Debug (default)" -DCMAKE_C_COMPILER:FILEPATH="%PATH_TO_MSVC/VC/Tools/MSVC/14.26.28801/bin/HostX64/x64/cl.exe" -DCMAKE_CXX_COMPILER:FILEPATH="%PATH_TO_MSVC/VC/Tools/MSVC/14.26.28801/bin/HostX64/x64/cl.exe"  -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_MAKE_PROGRAM="%PATH_TO_CMAKE\Ninja\ninja.exe" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
for debug build. For release mode it will set the value of CMAKE_BUILD_TYPE to "Release".

You can change the build scheme to Release if you need a Release binaries. In order to create a release scheme just click on Plus simbol in Configurations section in CMake Settings (you can get here by clicking on `Manage Settings` in Builds schemes dropdown menu) and selecting `x64-Release` mode. That's it. Now you can select release building mode.

Also you can create and run the similar command by your own. 

The process will create a folder `out` in the project folder with all generated CMake files. After that you can open Project dropdown menu from the top menu and select CMake Settings from there. It will create a special json file with your settings. There you can select any desired build option. For example to turn off `ENABLE_WALLET` option to prevent wallet building. Visual Studio will provide a special inteface to do that. Also you can check there current values for all build parameters. In a case of building cmake cache from the Terminal you can simply add `-DENABLE_WALLET=OFF` or any other option and value you want to the build cmake cache command.

## Build BTCU

After the configuration process of the CMake is completed just click F6 to run a build (please do not forget to select desired Build scheme). That's it!

## Running

After that you will get executable .exe files in your project folder available here: `out/build/x64-Debug (default)/bin` or `out/build/x64-Release (default)/bin` depends on the selected build scheme.

## Debugging

If you want to run it in a debug you can select target for debugging in a Target selection menu (it is placed after Build scheme selection and Run button), for example, `btcu-qt.exe`. MSVC will build the debuggable binaries and run it in a debug mode so you can use all MSVC debugging features.

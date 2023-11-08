# template-downloader
```
This is just an example.
```

## What does it do
```
It downloads a public repo specified and changes some specific files.
```

## Files Changes
```
run.bat : Replaces __GAME_TEMPLATE_NAME__ with the specified project name given on the commandline
build.bat : Replaces __GAME_TEMPLATE_NAME__ with the specified project name given on the commandline
```

## Build
You can use vcpkg to install dependencies.
```
> goto (for example) C:/
git clone https://github.com/microsoft/vcpkg.git
run the bootstrap-vcpkg (a vcpkg executable will be created)
add this to global enviroment paths

> Then run:
vcpkg install zlib:x64-windows-static-md
vcpkg install libzip:x64-windows-static-md
vcpkg install curl:x64-windows-static-md

> Or on linux
vcpkg install zlib:x64-linux
vcpkg install libzip:x64-linux
vcpkg install curl:x64-linux
```
Then you can build with
```
cmake -S . -B build
cmake --build build --config=Release
```
If there are problems with paths you may have to pass them in manually. See `build.bat` for example.

### How to Use
```
Call through commandline with required arguments.
```

## Call on the commandline
```
template-downloader -p <name> -o <dest-folder> -s <source-github>
```

## Commandline Options
```
-p <name>         : Project Name (required)
-o <folder>       : Destination Folder (default .)
-s <source>       : Github Source (required) Written as user/project
-attempts <num>   : Number of attempts to download archive. (default 6)
-v                : Verbose Output
-ra               : Prints received commandline arguments
```
## Examples
```
template-downloader -p ld99 -s Azenris/game-template
template-downloader -p ld99 -o C:/projects -s Azenris/game-template
template-downloader -p test_project -o C:/projects/ -s Azenris/game-template
```

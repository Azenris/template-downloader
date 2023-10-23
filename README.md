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

## How to Use
```
Call through commandline with required arguments.
```

### Call on the commandline
```
template-downloader -p <name> -o <dest-folder> -s <source-github>
```

### Commandline Options
```
-p                : Project Name (required)"
-o                : Destination Folder (default .)"
-s                : Github Source (required)"
-v                : Verbose Output"
-attempts <num>   : Number of attempts to download archive. (default 6)"
```
### Examples
```
template-downloader -p ld99 -s Azenris/game-template
template-downloader -p ld99 -o C:/projects -s Azenris/game-template
template-downloader -p test_project -o C:/projects/ -s Azenris/game-template
```

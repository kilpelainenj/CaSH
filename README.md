# CaSH
You can find a short writeup about the project at www

CaSH is a shell with functionalities like environment variables, variable expansion, output and input redirection, pipelines, background execution
and a dirsum command, that uses an LLM to create a summary of your current directory.

## Features

- Pipelines with `|`
- Redirection: `<`, `>`, `>>`
- Environment variables (`export`, `unset`) and expansion (e.g., `$NAME`)
- Built-ins: `cd`, `export`, `unset`, `exit`, …  
- `dirsum`: walks the current directory, collects the first N bytes of each file,
  and streams a summary via SSE using libcurl + cJSON

---

## Requirements

- The current makefile works for macOS only
- C toolchain (macOS: `xcode-select --install`, Linux: `build-essential`)
- **Flex** and **Bison**
- **libcurl** (dev headers)
- **cJSON**
- (Optional) **Catch2** for tests


Dependencies on macOS 
```
brew install cjson curl, catch2, flex
```
You can build the project by running 
```
make
```
If you want to build the tests as well, you can run 
```
make tests
```
and run it with 
```
./cash
```
# wyag

A modern C++ implementation of parts of Thibault Polge's WYAG project.

Original project: https://wyag.thb.lt/#table-of-contents

## Build

```bash
make
```

Run the program with:

```bash
./build/wyag --help
```

## Common Commands

Initialize a repository:

```bash
./build/wyag init
./build/wyag init my-repo
```

Stage files:

```bash
./build/wyag add README.md
./build/wyag add src/main.cpp include/wyag/cli.hpp
```

Check status:

```bash
./build/wyag status
```

Commit staged files:

```bash
./build/wyag commit -m "initial commit"
```

Remove tracked files:

```bash
./build/wyag rm old-file.txt
./build/wyag rm --cached generated.txt
```

List indexed files:

```bash
./build/wyag ls-files
./build/wyag ls-files -v
```

## Other Commands

Lower-level and less common commands: `cat-file`, `hash-file`, `ls-tree`, `checkout`, `show-ref`, `tag`, `rev-parse`, `check-ignore`.

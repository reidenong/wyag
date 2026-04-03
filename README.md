# wyag

A modern C++ implementation of parts of Thibault Polge's WYAG project.

## Build

```bash
make
```

Run the program with:

```bash
./build/wyag --help
```

## Commands

Initialize a repository:

```bash
./build/wyag init
./build/wyag init my-repo
```

Print an object from the repository:

```bash
./build/wyag cat-file blob <object-id>
```

Hash a file without writing it to the repository:

```bash
./build/wyag hash-file path/to/file
```

Hash a file and write it into the repository object database:

```bash
./build/wyag hash-file -w path/to/file
```

Specify the object type explicitly:

```bash
./build/wyag hash-file -t blob path/to/file
./build/wyag cat-file blob <object_id>
```

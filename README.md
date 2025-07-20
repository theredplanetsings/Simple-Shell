# Simple Shell

Implementation of a simple shell program in C, similar to that found in modern computers. The shell reads user commands, parses them, and executes the corresponding programs. The project consists of three main files: `shell.c`, `parser.c`, and `parser.h`.

## Files

### `shell.c`
This file contains the main function of the shell program. It handles the following tasks:
- Initialising the shell environment.
- Reading user input.
- Using the parser to interpret commands.
- Executing the parsed commands.

### `parser.c`
This file contains the implementation of the command parser. It includes functions to:
- Split the input string into individual tokens.
- Identify and handle different types of commands and arguments.

### `parser.h`
This header file declares the functions and data structures used by `parser.c`. It provides the necessary interface for `shell.c` to use the parser.

## Compilation

To compile the project, use the following command:
```sh
gcc -o shell shell.c parser.c
```

## Usage

Run the shell program with:
```sh
./shell
```

Once the shell is running, you can enter commands as you would in a typical terminal. The shell will parse and execute these commands.

## Example

```sh
$ ./shell
> ls -l
> echo "Hello, World!"
> exit
```

## License

This project is licensed under the MIT License.

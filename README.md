# 4DOS

4DOS is a command interpreter for DOS with aliases, command history, filename completion, and enhanced batch scripting. This project builds on the 4DOS 8.00 source release to fix bugs and improve the shell. Contributions are welcome, including patches, test cases, and documentation.

## Building

The DOS build uses Microsoft Visual C++ 1.52 and Microsoft Macro Assembler (MASM) 6.11D. Make an existing `4DOS.COM` available on `PATH` to run the build scripts. From the repository root in DOS, set the tool paths for your installation and run the build.

```bat
cd SOURCES
set MSVC=C:\MSVC152
set MASM=C:\MASM611
BUILD.BAT
```

The build produces `SOURCES/DOS/4DOS.COM`. `BUILD.BAT` builds incrementally by default. Run `BUILD.BAT clean` to remove build artifacts before rebuilding, or `BUILD.BAT --help` to see the available options.

## Contributing

For a bug report, list the steps to reproduce the problem and describe what you expected and what happened. [Open or browse issues](https://github.com/codespunk/4dos/issues)

For a patch, describe what changed and how you tested it. Original material you submit for inclusion is licensed under MIT. [Contribution terms](CONTRIB.md)

AI assistance is welcome. If you use it for code, documentation, or summaries, briefly disclose what it helped with in your submission. [Contributor code of conduct](CODE_OF_CONDUCT.md)

## Licensing

The inherited 4DOS source and third-party material are covered by their respective license terms and notices. Original project contributions are licensed under MIT. [4DOS license and notices](LICENSE.md) · [MIT license](LICENSE_MIT.md)

## Historical resources

The 4DOS.INFO archive provides historical source and binary releases, build notes, and documentation. [Source archives](https://4dos.info/sources.htm) · [4DOS documentation](https://4dos.info/)

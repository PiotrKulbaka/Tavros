import textwrap

def get_doc():
    doc = """
        Command line interface for Tavros tools.
        See `env.toml` for project configuration parameters.

        Usage:
            tavros.py setup
            tavros.py autoformat
            tavros.py collect_sources
            tavros.py cmake_gen (xcode | vstudio | ninja | makefiles)
            tavros.py (-h | --help)

        Options:
            -h, --help          Show this screen

        Commands:
            setup               Initialize the Tavros project
            autoformat          Autoformat all source code files
            collect_sources     Collect source code files to the `CMakeSources.txt`
            cmake_gen           Generate CMake project for the given generator

        Generators:
            xcode               Generate CMake project for Xcode
            vstudio             Generate CMake project for Visual Studio
            ninja               Generate CMake project for Ninja
            makefiles           Generate CMake project for Unix Makefiles
        """
    return textwrap.dedent(doc).strip()

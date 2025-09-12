import textwrap

def get_doc():
    doc = """
        Command line interface for Tavros tools.

        Usage:
            tavros.py autoformat [--colored]
            tavros.py collect_sources [--colored]
            tavros.py cmake_gen (xcode | visual_studio | ninja | makefiles) [--collect_sources] [--autoformat] [--colored]
            tavros.py (--show_resolved_config) [--colored]
            tavros.py (-h | --help) [--colored]

        Options:
            -h, --help              Show this screen
            --autoformat            Autoformat all source code files, executed before the main command
            --collect_sources       Collect source code files to the `CMakeSources.txt`, executed before the main command
            --show_resolved_config  Show resolved configuration file
            --colored               Use colors in the output

        Commands:
            autoformat              Autoformat all source code files
            collect_sources         Collect source code files to the `CMakeSources.txt`
            cmake_gen               Generate CMake project for the given generator

        Generators:
            xcode                   Generate CMake project for Xcode
            visual_studio           Generate CMake project for Visual Studio
            ninja                   Generate CMake project for Ninja
            makefiles               Generate CMake project for Unix Makefiles
        """
    return textwrap.dedent(doc).strip() 

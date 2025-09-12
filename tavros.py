#!/usr/bin/env python

import subprocess
import venv
import platform
import os
import sys
from pathlib import Path

def main() -> int:
    """
    Check if the current interpreter is in the virtual environment created by this script.
    If not, create the virtual environment and install the required packages.
    Then run the script again with the virtual environment's interpreter.

    :return: Exit code of the script.
    """

    root = Path(os.path.dirname(os.path.realpath(__file__)))
    pyenv = root / 'build' / 'pyenv'

    if sys.argv[1:] in ([], ['-h'], ['--help']):
        import tools.scripts.help
        print(tools.scripts.help.get_doc())
        return 0
    if pyenv in Path(sys.executable).parents:
        import tools.scripts.main
        # In the virtual environment
        return tools.scripts.main.main(root)
    else:
        # Not in virtual invirement. So first create the virtual envirement then
        # install requirements and run script again in the created virtual envirement.
        pyenv_bin = pyenv / ('Scripts' if platform.system().startswith('Win') else 'bin')
        venv.create(pyenv, with_pip=True)

        for package in ['docopt==0.6.2', 'toml==0.10.2', 'colorama==0.4.6']:
            subprocess.check_call([
                str(pyenv_bin / 'pip'),
                'install',
                package,
                '-qq', # Quied mode during installation
                '--only-binary',
                'pynacl',
            ])

        return subprocess.run([str(pyenv_bin / 'python'), __file__, ] + sys.argv[1:])

if __name__ == "__main__":
    REQUIRED_PYTHEN_VERSION = (3, 9)
    if sys.version_info < REQUIRED_PYTHEN_VERSION:
        current = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
        required = ".".join(map(str, REQUIRED_PYTHEN_VERSION))
        print(f"Error: Python version {current} found, but required {required} or higher.")
        sys.exit(1)
    sys.exit(main())

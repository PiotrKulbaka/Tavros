import os
from pathlib import Path
from docopt import docopt
from .config import Config
from .tools import setup
from .tools import autoformat
from .tools import collect_sources
from .cmake import cmake_gen
from .help import get_doc


def main(root_dir) -> int:
    env_file_path = Path(root_dir) / 'env.toml'
    doc = get_doc()
    args = docopt(doc)

    if env_file_path.exists():
        cfg = Config(env_file_path)
    elif not args['setup']:
        print("The `env.toml` file not found. Use `./tavros.py setup` to initialize the project\n")
        print(doc)
        return 1

    try:
        if args['setup']:
            setup(root_dir, Path(root_dir) / 'tools' / 'initial_config.toml')
        elif args['autoformat']:
            autoformat(cfg)
        elif args['collect_sources']:
            collect_sources(cfg)
        elif args['cmake_gen']:
            if args['xcode']:
                cmake_gen(cfg, 'xcode')
            if args['vstudio']:
                cmake_gen(cfg, 'vstudio')
            if args['ninja']:
                cmake_gen(cfg, 'ninja')
            if args['makefiles']:
                cmake_gen(cfg, 'makefiles')
        else:
            print('Unknown command.')
            print(doc)
            return 1
    except Exception as e:
        print('An exception was thrown.')
        raise e

    return 0

import time
from pathlib import Path
from docopt import docopt
from .config import Config
from .tools import autoformat
from .tools import collect_sources
from .cmake import cmake_gen
from .help import get_doc
from .variable_resolver import VariableResolver
from .builtin_variables import get_builtin_variables

def main(root_dir) -> int:
    start = time.perf_counter()

    doc = get_doc()
    args = docopt(doc)

    cfg = Config(Path(root_dir) / 'tools' / 'config.toml')
    builtin_vars = get_builtin_variables(root_dir.as_posix())
    resolver = VariableResolver()
    resolver.resolve_for_config(cfg, builtin_vars)

    try:
        if args['--show_resolved_config']:
            print(cfg.to_str())
        elif args['autoformat']:
            autoformat(cfg)
        elif args['collect_sources']:
            collect_sources(cfg)
        elif args['cmake_gen']:
            if args['--autoformat']:
                autoformat(cfg)
            if args['--collect_sources']:
                collect_sources(cfg)
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

    end = time.perf_counter()
    print(f'Execution time: {end - start:.2f} seconds')

    return 0

import time
from pathlib import Path
from docopt import docopt
from .config import Config
from .tools import autoformat
from .tools import collect_sources
from .cmake import cmake_gen
from .help import get_doc
from .log import log_print, CL_FATAL, CL_INFO
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
            log_print(cfg.to_str())
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
            if args['visual_studio']:
                cmake_gen(cfg, 'visual_studio')
            if args['ninja']:
                cmake_gen(cfg, 'ninja')
            if args['makefiles']:
                cmake_gen(cfg, 'makefiles')
        else:
            log_print(f'{CL_FATAL}Unknown command.')
            log_print(doc)
            return 1
    except Exception as e:
        log_print(f'{CL_FATAL}An exception was thrown.')
        raise e

    end = time.perf_counter()
    log_print(f'{CL_INFO}Execution time: {end - start:.2f} seconds')

    return 0

import time
from pathlib import Path
from docopt import docopt
from .config import Config
from .tools import autoformat
from .tools import collect_sources
from .cmake import cmake_gen, build_target
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
            defines={}
            defines['TRACY_ENABLE'] = 'ON' if args['--tracy'] else 'OFF'
            defines['TRACY_ON_DEMAND'] = 'ON' if args['--tracy'] else 'OFF'

            if args['--autoformat']:
                autoformat(cfg)

            if args['--collect_sources']:
                collect_sources(cfg)

            if args['xcode']:
                cmake_gen(cfg, 'xcode', defines)

            if args['visual_studio']:
                cmake_gen(cfg, 'visual_studio', defines)

            if args['makefiles']:
                cmake_gen(cfg, 'makefiles', defines)
    
        elif args['build']:
            # Build command implementation - supports arbitrary targets
            target_name = None

            # Get the target name from the command line (what comes after build)
            # Based on docopt parsing logic, check for available targets
            if args.get('sandbox'):
                target_name = 'sandbox'

            defines={}
            defines['TRACY_ENABLE'] = 'ON' if args['--tracy'] else 'OFF'
            defines['TRACY_ON_DEMAND'] = 'ON' if args['--tracy'] else 'OFF'

            if target_name:
                # Read configuration for this specific target
                try:
                    build_config = cfg.get(f'command.build.{target_name}', {})
                    default_config = build_config.get('default_config', 'debug')
                except:
                    default_config = 'debug'
                    build_config = {}
                
                # Determine configuration (release/debug)
                config = "debug"
                if args['--release']:
                    config = "release"
                elif args['--debug']:
                    config = "debug"
                else:
                    config = default_config
                
                # Get generator from arguments - support multiple generators
                generator = ''
                if args.get('--xcode'):
                    generator = 'xcode'
                elif args.get('--visual_studio'):
                    generator = 'visual_studio'
                elif args.get('--makefiles'):
                    generator = 'makefiles'
                else:
                    log_print(f'{CL_FATAL}No generator specified for build command.')
                    log_print(doc)
                    return 1

                # Call the build function with all parameters including generator
                build_target(cfg, target_name, generator, defines, config, args['--verbose'])
    
            else:
                log_print(f'{CL_FATAL}Unknown target specified for build command.')
                log_print(doc)
                return 1
            
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
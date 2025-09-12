from pathlib import Path
from .config import Config
from .utils import run_command, tool_list, tool_version, parse_version
from .log import log_print, CL_FATAL, CL_INFO, CL_GREEN, CL_INVERT, CL_RESET

def get_cmake_tool(cmake_cfg):
    """
    Find a suitable CMake executable in PATH that meets the minimum version requirement.

    Args:
        cmake_cfg: Configuration object containing at least 'min_version'.

    Returns:
        Path to the CMake executable.

    Raises:
        Exception: If no CMake is found or no version meets the minimum requirement.
    """

    tools = tool_list("cmake")
    if not tools:
        msg = "CMake not found in PATH"
        log_print(CL_FATAL, msg)
        raise Exception(msg)

    min_ver = parse_version(cmake_cfg.min_version)
    for tool in tools:
        ver = tool_version(tool)
        if ver >= min_ver:
            log_print(f"{CL_INFO}{CL_INVERT}Using CMake {ver[0]}.{ver[1]}.{ver[2]}{CL_RESET} from {CL_GREEN}{tool}")
            return tool

    msg = f"No CMake >= {cmake_cfg.min_version} found in PATH"
    log_print(CL_FATAL, msg)
    raise Exception(msg)


def cmake_gen(cfg: Config, generator: str, defines: dict[str, str] = None):
    """
    Generate a project using CMake.

    This function prepares the build directory, constructs the CMake command
    with generator, architecture, compile options, and user-defined variables,
    and executes it.

    Args:
        cfg (Config): Project configuration containing paths and CMake settings.
        generator (str): Name of the generator to use (e.g., "Ninja", "Visual Studio 17").
        defines (dict[str, str], optional): Additional CMake variables to define.

    Raises:
        RuntimeError: If the CMake command fails.
    """

    log_print(f'{CL_INFO}{CL_INVERT}Configuring project...')

    cmake_cfg = cfg.command.cmake_gen[generator]

    cmake_tool = get_cmake_tool(cmake_cfg)

    build_dir = Path(cfg.path.build) / generator
    build_dir.mkdir(parents=True, exist_ok=True)

    compile_options = cmake_cfg.get('compile_options', [])

    args = [
        cmake_tool,
        *(['-G', cmake_cfg.generator] if cmake_cfg.generator else []),
        *(['-A', cmake_cfg.architecture] if cmake_cfg.architecture else []),
        '-S', cfg.path.root,
        '-B', str(build_dir),
        *([f'-DTAV_COMPILE_OPTIONS={' '.join(compile_options)}'] if compile_options else []),
        *([f'-D{k}={v}' for k, v in defines.items()] if defines else []),
    ]

    result = run_command(args)

    if result.returncode != 0:
        raise RuntimeError("CMake configuration failed")
    
    log_print(f'{CL_INFO}{CL_INVERT}Configuring project...{CL_RESET} {CL_GREEN}Done')

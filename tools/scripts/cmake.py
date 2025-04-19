from pathlib import Path
from .config import Config
from .utils import run_command

def cmake_gen(cfg: Config, generator: str, defines: dict[str, str] = None):
    cmake_cfg = cfg.command.cmake_gen[generator]

    build_dir = Path(cfg.path.build) / generator
    build_dir.mkdir(parents=True, exist_ok=True)

    compile_options = cmake_cfg.get('compile_options', [])

    args = [
        cmake_cfg.cmake_tool_path,
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

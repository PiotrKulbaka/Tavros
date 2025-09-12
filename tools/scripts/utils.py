import subprocess
import platform
from pathlib import Path
from .log import log_print, CL_FATAL, CL_INFO, CL_GREEN, CL_INVERT, CL_ERROR, CL_COMMAND

def get_platform_name():
    system = platform.system()
    match system:
        case 'Windows':
            return 'windows'
        case 'Linux':
            return 'linux'
        case 'Darwin':
            return 'macos'
        case _:
            raise Exception(f'Unknown platform: {system}')

def is_platform(name: str) -> bool:
    return get_platform_name() == name

def parse_version(ver: str) -> tuple[int, int, int]:
    """
    Convert a version string like "3.28.1" into a tuple (3, 28, 1).
    If some parts are missing, fill them with zeros.
    
    Args:
        ver (str): Version string in format "major.minor.patch"
    
    Returns:
        tuple[int, int, int]: Parsed version as (major, minor, patch)
    """
    parts = ver.split(".")
    # Keep only numeric parts
    nums = [int(p) for p in parts if p.isdigit()]
    # Fill missing parts with zeros
    while len(nums) < 3:
        nums.append(0)
    return tuple(nums[:3])

def run_command(command, capture_output=False):
    try:
        log_print(f'{CL_INFO}Run: {CL_COMMAND}`{' '.join(str(c) for c in command)}`')
        return subprocess.run(command, check=True, text=True, capture_output=capture_output)
    except subprocess.CalledProcessError as e:
        log_print(f'{CL_ERROR}The command ended with an error. Return code: {e.returncode}')
        raise e
    except FileNotFoundError:
        log_print(f'{CL_ERROR}Cmmand not found: {CL_COMMAND}`{' '.join((str(c) for c in command))}`')
        raise e
    except Exception as e:
        log_print(f'{CL_FATAL}Unknown error: {type(e).__name__}: {e}')
        raise e

def tool_list(tool_name: str) -> list[str]:
    """
    Find all available paths to the given tool in the system PATH.
    
    Args:
        tool_name (str): Name of the tool (e.g., "cmake").
    
    Returns:
        list[str]: List of absolute paths to the tool. Empty if not found.
    """

    if is_platform("windows"):
        search_cmd = ["where", tool_name]
    else:
        search_cmd = ["which", "-a", tool_name]

    try:
        result = run_command(search_cmd, capture_output=True)
        paths = [Path(line.strip()) for line in result.stdout.splitlines() if line.strip()]
        return paths
    except subprocess.CalledProcessError:
        return []

def tool_version(path: Path) -> tuple[int, int, int] | None:
    """Run `<tool> --version` and return version string or None if failed."""
    try:
        result = run_command([str(path), "--version"], capture_output=True)
        first_line = (result.stdout or "").splitlines()[0]
        # usually looks like: "tool version 3.28.1"
        parts = first_line.split()
        for i, token in enumerate(parts):
            if token.lower() == "version" and i + 1 < len(parts):
                return parse_version(parts[i + 1])
    except Exception:
        return None
    return None

import subprocess
import platform

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

def run_command(command):
    try:
        print(f'Run: `{' '.join(command)}`')
        return subprocess.run(command, check=True, encoding='utf-8')
    except subprocess.CalledProcessError as e:
        print(f'The command ended with an error. Return code: {e.returncode}')
        raise
    except FileNotFoundError:
        print(f'Cmmand not found: `{' '.join(command)}`')
        raise
    except Exception as e:
        print(f'Unknown error: {type(e).__name__}: {e}')
        raise

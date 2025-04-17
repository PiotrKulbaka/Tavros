from .utils import get_platform_name

def get_builtin_variables(root_dir, platform_name: str = None) -> dict:
    PLATFORM_MAP = {
        'windows': {
            'clang_format': 'clang-format.exe',
            'cmake': 'cmake.exe',
            'generator': 'Visual Studio 17',
            'arch': '',
        },
        'linux': {
            'clang_format': 'clang-format',
            'cmake': 'cmake',
            'generator': 'Unix Makefiles',
            'arch': '',
        },
        'macos': {
            'clang_format': 'clang-format',
            'cmake': 'cmake',
            'generator': 'Xcode',
            'arch': '',
        }
    }

    def get_value_for_platform(platform_name: str, key: str) -> str:
        try:
            return PLATFORM_MAP[platform_name][key]
        except KeyError:
            raise Exception(f'Unknown platform `{platform_name}` or missing key `{key}`')

    if platform_name is None:
        platform_name = get_platform_name()

    return {
        'builtin_root': str(root_dir),
        'builtin_clang_format_tool_path': get_value_for_platform(platform_name, 'clang_format'),
        'builtin_cmake_tool_path': get_value_for_platform(platform_name, 'cmake'),
        'builtin_cmake_host_generator': get_value_for_platform(platform_name, 'generator'),
        'builtin_cmake_host_architecture': get_value_for_platform(platform_name, 'arch'),
    }

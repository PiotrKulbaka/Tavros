import os
from concurrent.futures.thread import ThreadPoolExecutor
from concurrent.futures import as_completed
from pathlib import Path
from .config import Config
from .utils import run_command, tool_list, tool_version, parse_version
from .log import log_print, CL_FATAL, CL_INFO, CL_GREEN, CL_INVERT, CL_ERROR, CL_COMMAND, CL_NORMAL, CL_RESET


def get_clang_format_tool(clang_format_cfg):
    """
    Find a suitable clang-format executable in PATH that meets the minimum version requirement.

    Args:
        clang_format_cfg: Configuration object containing at least 'min_version'.

    Returns:
        Path to the clang-format executable.

    Raises:
        Exception: If no clang-format is found or no version meets the minimum requirement.
    """

    tools = tool_list("clang-format")
    if not tools:
        msg = "clang-format not found in PATH"
        log_print(f'{CL_FATAL}{msg}')
        raise Exception(msg)

    min_ver = parse_version(clang_format_cfg.min_version)
    for tool in tools:
        ver = tool_version(tool)
        if ver >= min_ver:
            log_print(f"{CL_INFO}{CL_INVERT}Using clang-format {ver[0]}.{ver[1]}.{ver[2]}{CL_RESET} from {CL_GREEN}{tool}")
            return tool

    msg = f"No clang-format >= {clang_format_cfg.min_version} found in PATH"
    log_print(f'{CL_FATAL}{msg}')
    raise Exception(msg)

def collect_files_by_exts_recursive(include, exclude, exts):
    """
    Collect all files with the specified extensions from the given directory
    and its subdirectories.

    :param include: Directories or files to include to the search.
    :param exclude: Directories or files to exclude from the search.
    :param exts: List of file extensions to be collected.
    :return: A sorted list of Path objects representing the collected files.
    """
    collected_files = []

    def has_suffix(path, suffixes):
        for suffix in suffixes:
            if str(path).endswith(suffix):
                return True
        return False

    def not_excluded(path):
        for excluded in exclude:
            if str(path).startswith(str(excluded)):
                return False
        return True

    def recursive_collect(entry):
        for path in Path(entry).iterdir():
            if path.is_file() and has_suffix(path, exts) and not_excluded(path):
                collected_files.append(path)
            elif path.is_dir():
                recursive_collect(path)

    if not isinstance(include, list):
        include = [include]

    for entry in include:
        if Path(entry).is_file() and has_suffix(entry, exts) and not_excluded(entry):
            collected_files.append(entry)
        else:
            recursive_collect(entry)

    return sorted(collected_files, key=lambda path: (path.parent, path))


def autoformat(cfg: Config):
    """
    Format all source code files using clang-format.

    :param cfg: Config object with the project configuration.
    """
    log_print(f'{CL_INFO}{CL_INVERT}Applying clang-format...')

    exts = cfg.command.autoformat.exts
    include = cfg.command.autoformat.include
    exclude = cfg.command.autoformat.exclude if "exclude" in cfg.command.autoformat else []
    clang_foramt_tool_path = get_clang_format_tool(cfg.command.autoformat)
    style_param = f"--style=file:{str(cfg.command.autoformat.clang_format_style_file)}"

    files = collect_files_by_exts_recursive(include, exclude, exts)

    def apply_clang_format(path):
        result = run_command([clang_foramt_tool_path, style_param, '-i', str(path)])
        return result.returncode == 0

    cores = os.cpu_count()
    workers = cores if cores else 8
    futures = []
    with ThreadPoolExecutor(max_workers=workers) as executor:
        for f in files:
            futures.append(executor.submit(apply_clang_format, f))
    
    all_ok = True
    for future in as_completed(futures):
        try:
            ok = future.result()
            if not ok:
                all_ok = False
        except Exception as e:
            all_ok = False

    if not all_ok:
        log_print(f"{CL_FATAL}Some files were not formatted.")
        raise Exception("Some files were not formatted.")

    log_print(f'{CL_INFO}{CL_INVERT}Applying clang-format...{CL_RESET} {CL_GREEN}Done')


def collect_sources(cfg: Config):
    """
    Collect source code files from the specified directories and generate
    'CMakeSources.txt' files for each of them.

    :param cfg: Config object with the project configuration.
    """
    log_print(f'{CL_INFO}{CL_INVERT}Collecting sources...')

    exts = cfg.command.collect_sources.exts
    test_exts = cfg.command.collect_sources.test_exts
    exclude = cfg.command.collect_sources.exclude if "exclude" in cfg.command.collect_sources else []
    platform_subfolders_section = cfg.command.collect_sources.platform_subfolders

    def collect_lib_sources(lib_dir, lib_name):
        """
        Collect source code files from the specified directory and generate
        'CMakeSources.txt' file for it.

        :param lib_dir: Directory containing source code files.
        :param lib_name: Library name for the generated 'CMakeSources.txt' file.
        """
        all_sources = collect_files_by_exts_recursive(lib_dir, exclude, exts)
        test_sources = collect_files_by_exts_recursive(lib_dir, exclude, test_exts)

        all_platfrom_sources = []
        platfrom_sources = {}
        for name, subfolder in platform_subfolders_section.items():
            platfrom_sources[name] = [s for s in all_sources if subfolder in str(s.relative_to(lib_dir).as_posix())]
            all_platfrom_sources.extend(platfrom_sources[name])

        # Exclude test sources and all platfrom sources 
        crossplatform_sources = [s for s in all_sources if s not in test_sources and s not in all_platfrom_sources]

        if not crossplatform_sources and not test_sources and not all_platfrom_sources:
            return

        cmake_sources_filename = lib_dir / 'CMakeSources.txt'

        log_print(f'{CL_INFO}Sources: {CL_GREEN}{CL_NORMAL}`{cmake_sources_filename}`')

        with open(cmake_sources_filename, 'w') as f:
            def print_sources_list(sources):
                prev = None
                for s in sources:
                    if prev and prev.parent != s.parent:
                        f.write('\n')
                    f.write(f'    ${{CMAKE_CURRENT_LIST_DIR}}/{str(Path(s).relative_to(lib_dir).as_posix())}\n')
                    prev = s

            f.write('#\n# Autogenerated file, do not edit manually\n')
            f.write('# Use `./tavros.py collect_sources` for regenerate it file\n#\n\n')

            if crossplatform_sources:
                f.write(f'set(TAV_{lib_name.upper()}_CROSSPLATFORM_SOURCES\n')
                print_sources_list(crossplatform_sources)
                f.write(')\n\n')

            for platfrom_name, sources in platfrom_sources.items():
                if not sources:
                    continue
                f.write(f'set(TAV_{lib_name.upper()}_{platfrom_name.upper()}_SOURCES\n')
                print_sources_list(sources)
                f.write(')\n\n')

            if test_sources:
                f.write(f'\nset(TAV_{lib_name.upper()}_TEST_SOURCES\n')
                print_sources_list(test_sources)
                f.write(')\n\n')

    paths = cfg.command.collect_sources.collect_paths
    for p in paths:
        dir = Path(p)
        if dir.exists():
            collect_lib_sources(dir, dir.name)
        else:
            print(f'{CL_ERROR}Directory is not exists: {CL_COMMAND}{str(dir.as_posix())}')
    
    log_print(f'{CL_INFO}{CL_INVERT}Collecting sources...{CL_RESET} {CL_GREEN}Done')

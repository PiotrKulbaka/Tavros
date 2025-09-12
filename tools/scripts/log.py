from colorama import init, Fore, Style
import sys

init(autoreset=True)

USE_COLORS = '--colored' in sys.argv

CL_INVERT = '\033[7m' if USE_COLORS else ''
CL_NORMAL = Style.NORMAL if USE_COLORS else ''
CL_RESET = Style.RESET_ALL if USE_COLORS else ''
CL_GREEN = Fore.GREEN if USE_COLORS else ''
CL_RED = Fore.RED if USE_COLORS else ''
CL_YELLOW = Fore.YELLOW if USE_COLORS else ''
CL_COMMAND = Fore.CYAN + CL_NORMAL if USE_COLORS else ''
CL_INFO = Fore.WHITE + Style.BRIGHT if USE_COLORS else ''
CL_FATAL = Fore.RED + Style.BRIGHT + CL_INVERT if USE_COLORS else ''
CL_ERROR = Fore.RED + Style.BRIGHT if USE_COLORS else ''

def log_print(*args, **kwargs):
    print(*args, **kwargs)
    if USE_COLORS:
        print(Style.RESET_ALL, end='')

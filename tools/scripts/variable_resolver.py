import re
from .config import Config

class VariableResolver:
    def __init__(self):
        self._pattern = re.compile(r"\$\{([a-zA-Z0-9_.]+)\}")

    def resolve_for_config(self, cfg: Config, vars: dict[str, str], max_resolves: int = 10):
        """
        Resolves variables within a configuration object using a given set of variables.

        This method iteratively resolves placeholder variables in the format 
        '${variable_name}' within the configuration data, using a provided dictionary 
        of variables and the configuration object itself. The resolution process 
        will iterate up to `max_resolves` times or until no more replacements can 
        be made.

        :param cfg: A Config object containing the configuration data where variables 
                    need to be resolved.
        :param vars: A dictionary where keys are variable names and values are the 
                    values to replace the variables with.
        :param max_resolves: The maximum number of resolution passes to perform. 
                            Defaults to 10.
        """

        resolves = False
        def resolve(text: str, vars: dict[str, str], config: Config) -> str:
            nonlocal resolves
            def replacer(match):
                nonlocal resolves
                key = match.group(1)
                resolves = True
                return vars[key] if key in vars else config[key]
            return self._pattern.sub(replacer, text)

        def resolve_in_place(d):
            def resolve_for_list(l):
                for i, item in enumerate(l):
                    if isinstance(item, dict):
                        resolve_in_place(item)
                    elif isinstance(item, list):
                        resolve_for_list(item)
                    elif isinstance(item, str):
                        l[i] = resolve(item, vars, cfg)

            for k, v in d.items():
                if isinstance(v, dict):
                    resolve_in_place(v)
                elif isinstance(v, list):
                    resolve_for_list(v)
                elif isinstance(v, str):
                    d[k] = resolve(v, vars, cfg)
        
        for i in range(max_resolves):
            resolve_in_place(cfg.data)
            if not resolves:
                break

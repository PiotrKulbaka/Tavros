import re
from .config import Config

class VariableResolver:
    def __init__(self):
        self._pattern = re.compile(r"\$\{([a-zA-Z0-9_.]+)\}")

    def resolve(self, text: str, vars: dict[str, str], config: Config) -> str:
        def replacer(match):
            key = match.group(1)
            if key in vars:
                return vars[key]
            return config[key]
        return self._pattern.sub(replacer, text)

    def resolve_for_config(self, cfg: Config, vars: dict[str, str]):
        def resolve_in_place(d):
            for k, v in d.items():
                if isinstance(v, dict):
                    resolve_in_place(v)
                elif isinstance(v, list):
                    for i, item in enumerate(v):
                        if isinstance(item, dict):
                            resolve_in_place(item)
                        elif isinstance(item, str):
                            v[i] = self.resolve(item, vars, cfg)
                            # item = self.resolve(item, vars, cfg)
                            # d[k] = [self.resolve(item, vars, cfg) if isinstance(item, str) else item for item in v]
                elif isinstance(v, str):
                    d[k] = self.resolve(v, vars, cfg)
        resolve_in_place(cfg.data)

import re
from .config import Config

class VariableResolver:
    def __init__(self):
        self._pattern = re.compile(r"\$\{([a-zA-Z0-9_]+)\}")

    def resolve(self, text: str, vars: dict[str, str]) -> str:
        def replacer(match):
            key = match.group(1)
            return vars.get(key, f'${{{key}}}')
        return self._pattern.sub(replacer, text)

    def resolve_for_config(self, cfg: Config, vars: dict[str, str]):
        def resolve_in_place(d):
            for k, v in d.items():
                if isinstance(v, dict):
                    resolve_in_place(v)
                elif isinstance(v, list):
                    d[k] = [self.resolve(item, vars) if isinstance(item, str) else item for item in v]
                elif isinstance(v, str):
                    d[k] = self.resolve(v, vars)
        resolve_in_place(cfg.data)

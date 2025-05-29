import toml

class DotDict(dict):
    def __init__(self, d):
        for k, v in d.items():
            if isinstance(v, dict):
                v = DotDict(v)
            self[k] = v

    def __getattr__(self, item):
        try:
            return self[item]
        except KeyError as e:
            raise AttributeError(f'No such attribute: {item}') from e

    def __setattr__(self, key, value):
        self[key] = value

    def get(self, path, default=None):
        parts = path.split(".")
        current = self
        for part in parts:
            if isinstance(current, dict) and part in current:
                current = current[part]
            else:
                return default
        return current


class Config:
    def __init__(self, config_path):
        with open(config_path, 'r', encoding='utf-8') as f:
            raw = toml.load(f)
        self.data = DotDict(raw)

    def save(self, config_path):
        def to_plain_dict(d):
            if isinstance(d, dict):
                return {k: to_plain_dict(v) for k, v in d.items()}
            return d

        with open(config_path, 'w', encoding='utf-8') as f:
            toml.dump(to_plain_dict(self.data), f)

    def __getitem__(self, key):
        if isinstance(key, str):
            parts = key.split(".")
            current = self.data
            for part in parts:
                if isinstance(current, dict) and part in current:
                    current = current[part]
                else:
                    raise KeyError(f"No such key: {key}")
            return current
        else:
            return self.data[key]

    def __setitem__(self, key, value):
        self.data[key] = value

    def __getattr__(self, item):
        return getattr(self.data, item)

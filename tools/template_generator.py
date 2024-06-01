from pathlib import Path
from typing import Any, MutableMapping

import sys
from pathlib import Path

def import_path(path):
    strpath = str(path)
    if not strpath.startswith("/"):
        parent_path = Path(sys._getframe().f_globals.get("__file__", ".")).parent
        path = parent_path / path
    else:
        path = Path(path)
    try:
        sys.path.insert(0, str(path.parent))
        module = __import__(path.stem)
    finally:
        sys.path.pop(0)
    return module


class OutputStream:
    out = ''

    def write(self, text, *args, **kwargs):
        self.out += text.format(*args, **kwargs)

    def writeln(self, text, *args, **kwargs):
        self.out += text.format(*args, **kwargs) + '\n'

    def clear(self):
        self.out = ''

class TemplateGenerator:
    def __init__(self, input: Path, output: Path, open_block: str = '{{', close_block: str = '}}') -> None:
        self.input = input
        self.output = output
        self.open_block = open_block
        self.close_block = close_block
        self.global_vars: MutableMapping[str, Any] = {}
        self.local_vars: MutableMapping[str, Any] = {}


    def run(self) -> None:
        transformed_content = ''
        try:
            parent_path = Path(sys._getframe().f_globals.get("__file__", ".")).parent
            sys.path.insert(0, str(parent_path / "generator"))
            with open(self.input, 'r') as f:
                transformed_content = self._transform(f.read())
            
            with open(self.output, 'w') as f:
                f.write(transformed_content)
        finally:
            sys.path.pop(0)

    def _transform(self, content: str) -> str:
        output = '// Auto-generated files.\n// All the changes will be overwritten.\n\n'
        last_end = 0
        while True:
            start = content.find(self.open_block, last_end)
            if start == -1:
                break

            end = content.find(self.close_block, start)
            if end == -1:
                break

            output += content[last_end:start]
            command = content[start + len(self.open_block):end]

            output += self._run_command(command)
            last_end = end + len(self.close_block)

        output += content[last_end:]
        return output

    def fix_command_block(self, command: str) -> str:
        """
            [python block] {
                [children]
            }
            |
            v
            [python block]:
                [children]
        """
        lines = command.split('\n')
        fix_source = ''

        indent_level = 0
        for line in lines:
            new_line = line.strip()
            if new_line.endswith('{'):
                new_line = new_line.rstrip('{').rstrip() + ':'
                fix_source += '    ' * indent_level + new_line + '\n'
                indent_level += 1
            elif new_line.endswith('}'):
                new_line = new_line.rstrip('}').rstrip()
                if len(new_line) == 0:
                    continue
                fix_source += '    ' * indent_level + new_line + '\n'
                indent_level -= 1
            else:
                if len(new_line) == 0:
                    continue
                fix_source += '    ' * indent_level + new_line + '\n'
        
        return fix_source


    def _run_command(self, command: str) -> str:
        key = 'ostream'
        if key not in self.local_vars or type(self.local_vars[key]) != OutputStream:
            self.local_vars[key] = OutputStream()
        else:
            self.local_vars[key].clear()

        command = self.fix_command_block(command)
        exec(command, self.global_vars, self.local_vars)
        if type(self.local_vars[key]) == OutputStream:
            return self.local_vars[key].out.strip()
        raise ValueError('Command must return a string')
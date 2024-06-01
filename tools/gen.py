import argparse
from pathlib import Path
import sys
from typing import Optional
from template_generator import TemplateGenerator

def get_output_path(input_path: Path, output: Optional[str]) -> Path:
    filename  = input_path.stem
    parent = input_path.parent
    if output:
        return parent / output
    
    return parent / f'{filename}.def'

def generate_recursively() -> None:
    parent_path = Path(__file__).parent.parent
    for path in parent_path.glob('**/*.pdef'):
        output_path = path.with_suffix('.def')
        if output_path.exists():
            print(f'Overwriting "{output_path.relative_to(parent_path)}"...', file=sys.stderr)
        else:
            print(f'Generating "{output_path.relative_to(parent_path)}"...', file=sys.stderr)
        TemplateGenerator(path, output_path).run()

def main() -> None:
    parser = argparse.ArgumentParser(description='Generate a file with the given name and content.')
    parser.add_argument('-i', '--input', type=str, help='Path to the file to be generated', required=False)
    parser.add_argument('-o', '--output', type=str, help='Content to be written to the file', required=False)
    args = parser.parse_args()
    
    if not args.input:
        return generate_recursively()

    input_path = Path(args.input)
    if not input_path.exists():
        print(f'Error: {input_path} does not exist.', file=sys.stderr)
        exit(1)
    
    output_path = get_output_path(input_path, args.output)

    if output_path.exists():
        print(f'Error: {output_path} already exists.', file=sys.stderr)
        exit(1)
    
    TemplateGenerator(input_path, output_path).run()

if __name__ == '__main__':
    main()
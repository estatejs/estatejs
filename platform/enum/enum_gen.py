#!/usr/bin/env python3
import pystache
import sys

if len(sys.argv) != 4:
    print('Usage: enum_gen <enum_file> <cpp_output> <ts_output>')
    exit(1)

enum_file = sys.argv[1]
cpp_outfile = sys.argv[2]
ts_outfile = sys.argv[3]

with open(enum_file, 'r') as f:
    keys = [line.rstrip() for line in f]

with open("cpp_enum.mustache",'r') as f:
    cpp_enum_text = f.read()

with open("ts_enum.mustache",'r') as f:
    ts_enum_text = f.read()

ts_name = 'Code'

i = 0
ts_enums = []
cpp_enums = []
ts_key_names = []
cpp_key_string_mappings = []
for key in keys:
    if key.startswith("#") or key == '':
        continue
    if not key.startswith("private: "):
        ts_key = key
        ts_enums.append(' ' * 4 + f"{ts_key} = {i}")
        ts_key_names.append(' ' * 8 + f"case {ts_name}.{ts_key}:\n{' ' * 12}return '{ts_key}';")
    if key.startswith("private: "):
        key = key.lstrip("private: ")
    cpp_enums.append(' ' * 8 + f"{key} = {i}")
    cpp_key_string_mappings.append(' ' * 12 + f"case Code::{key}:\n{' ' * 16}return \"{key}\";")
    i += 1

with open(cpp_outfile, 'w') as f:
    f.write(pystache.render(cpp_enum_text, {
        'NAMESPACE': 'estate',
        'NAME': 'Code',
        'KEYS': ",\n".join(cpp_enums),
        'KEY_STRING_MAPPINGS': "\n".join(cpp_key_string_mappings)
    }))

with open(ts_outfile, 'w') as f:
    f.write(pystache.render(ts_enum_text, {
        'NAME': ts_name,
        'KEYS': ",\n".join(ts_enums),
        'KEY_NAME_CASES': "\n".join(ts_key_names),
    }))
#!/usr/bin/env python

import argparse
import io
import os
import re

re_token = re.compile(r'^CT_(\w+),')
re_version = re.compile(r'.*UNCRUSTIFY_VERSION\s*"Uncrustify-([^"]+)"')
re_option = re.compile(r'extern (Bounded)?Option<[^>]+>')
re_enum_decl = re.compile(r'enum class (\w+)( *// *<(\w+)>)?')
re_enum_value = re.compile(r'(\w+)(?= *([,=]|//|$))')
re_aliases = re.compile(r'UNC_OPTVAL_ALIAS\(([^)]+)\)')

version = '0.0'
options = set()
values = set()
tokens = set()

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
script = os.path.relpath(__file__, root)


# -----------------------------------------------------------------------------
def read_enum(f):
    global values

    for line in iter(f.readline, ''):
        line = line.strip()

        if line.startswith('{'):
            for line in iter(f.readline, ''):
                line = line.strip()

                if line.startswith('};'):
                    return

                if 'UNC_INTERNAL' in line:
                    return

                if 'UNC_CONVERT_INTERNAL' in line:
                    return

                mv = re_enum_value.match(line)
                if mv is not None:
                    values.add(mv.group(1).lower())


# -----------------------------------------------------------------------------
def write_items(out, items):
    for i in sorted(items):
        out.write(u'      <item>{}</item>\n'.format(i))


# -----------------------------------------------------------------------------
def write_options(out, args):
    write_items(out, options)


# -----------------------------------------------------------------------------
def write_values(out, args):
    write_items(out, values)


# -----------------------------------------------------------------------------
def write_tokens(out, args):
    write_items(out, tokens)


# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description='Generate uncrustify.xml')
    parser.add_argument('output', type=str,
                        help='location of uncrustify.xml to write')
    parser.add_argument('template', type=str,
                        help='location of uncrustify.xml.in ' +
                             'to use as template')
    parser.add_argument('version', type=str,
                        help='location of uncrustify_version.h to read')
    parser.add_argument('options', type=str,
                        help='location of options.h to read')
    parser.add_argument('optvals', type=str,
                        help='location of option.h to read')
    parser.add_argument('tokens', type=str,
                        help='location of token_enum.h to read')
    args = parser.parse_args()

    # Read version
    with io.open(args.version, 'rt', encoding='utf-8') as f:
        global version
        for line in iter(f.readline, ''):
            line = line.strip()

            mv = re_version.match(line)
            if mv:
                version = mv.group(1)

    # Read options
    with io.open(args.options, 'rt', encoding='utf-8') as f:
        global options
        for line in iter(f.readline, ''):
            line = line.strip()

            if re_option.match(line):
                n, d = f.readline().split(';')
                options.add(n)

    # Read option values
    with io.open(args.optvals, 'rt', encoding='utf-8') as f:
        global values
        for line in iter(f.readline, ''):
            line = line.strip()

            if re_enum_decl.match(line):
                read_enum(f)
                continue

            ma = re_aliases.match(line)
            if ma:
                for v in ma.group(1).split(',')[2:]:
                    v = v.strip()[1:-1]
                    values.add(v)

    # Read tokens
    with io.open(args.tokens, 'rt', encoding='utf-8') as f:
        global tokens
        for line in iter(f.readline, ''):
            line = line.strip()

            m = re_token.match(line)
            if m and not m.group(1).endswith(u'_'):
                tokens.add(m.group(1).lower())

    # Declare replacements
    replacements = {
        u'##OPTION_KEYWORDS##': write_options,
        u'##VALUE_KEYWORDS##': write_values,
        u'##TOKEN_TYPE_KEYWORDS##': write_tokens,
    }

    # Write output file
    with io.open(args.output, 'wt', encoding='utf-8') as out:
        with io.open(args.template, 'rt', encoding='utf-8') as t:
            for line in t:
                directive = line.strip()
                if directive in replacements:
                    replacements[directive](out, args)
                else:
                    if '##VERSION##' in line:
                        line = line.replace('##VERSION##', version)
                    out.write(line)

# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


if __name__ == '__main__':
    main()

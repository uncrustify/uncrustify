#!/usr/bin/python
import argparse

from nltk.parse.generate import generate
from nltk import CFG
from os.path import exists
from sys import exit as sys_exit

DEMO_GRAMMAR = """
    S -> 'import ' ImportList ';' | 'static import ' ImportList ';'
    ImportList -> Import | ImportBindings | Import ', ' ImportList
    Import -> ModuleFullyQualifiedName | ModuleAliasIdentifier ' = ' ModuleFullyQualifiedName
    ImportBindings -> Import ' : ' ImportBindList
    ImportBindList -> ImportBind | ImportBind ', ' ImportBindList
    ImportBind -> Identifier | Identifier ' = ' Identifier
    
    ModuleAliasIdentifier -> Identifier
    
    Packages -> PackageName | Packages '.' PackageName
    ModuleFullyQualifiedName -> ModuleName | Packages '.' ModuleName
    PackageName -> Identifier
    ModuleName -> Identifier
    
    Identifier -> 'x'
"""


def valid_file(arg_parser, *args):
    """
    checks if on of the provided paths is a file


    Parameters
    ----------------------------------------------------------------------------
    :param arg_parser:
        argument parser object that is called if no file is found

    :param args: list< str >
        a list of file path that is going to be checked


    :return: str
    ----------------------------------------------------------------------------
        path to an existing file
    """
    arg = None
    found_flag = False
    for arg in args:
        if exists(arg):
            found_flag = True
            break
    if not found_flag:
        arg_parser.error("file(s) do not exist: %s" % args)

    return arg


def main(args):
    grammar_string = DEMO_GRAMMAR

    if args.input_file_path:
        with open(args.input_file_path, 'r') as f:
            grammar_string = f.read()

    grammar = CFG.fromstring(grammar_string)

    for sentence in generate(grammar, depth=args.depth):
        print(''.join(sentence))

    return 0


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()

    arg_parser.add_argument(
        '-i', '--input_file_path',
        metavar='<path>',
        type=lambda x: valid_file(arg_parser, x),
        help="Path to the grammar file",
        required=False
    )
    arg_parser.add_argument(
        '-d', '--depth',
        metavar='<nr>',
        type=int,
        default=9,
        help='Max depth of grammar tree.'
    )

    FLAGS, unparsed = arg_parser.parse_known_args()

    sys_exit(main(FLAGS))

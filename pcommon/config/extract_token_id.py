#! /usr/bin/env python
# -*- coding: utf-8 -*-
# FILE         :  extract_token_id.py
# DESCRIPTION  :  Extract token IDs from Bison-generated C++ parser header
# PROGRAMMED BY:  Yakov Markovitch
# CREATION DATE:  23 Jul 2010

import re

_begin_tokens = re.compile(r'^\s*enum\s+yytokentype\s*{\s*$').match
_end_tokens = re.compile(r'^\s*[}]\s*;\s*$').match

def token_ids(lines, token_pfx):
    match_id = re.compile(r'^\s*(%s\w+)\s*=\s*([0-9xaAbBcCdDeEfF]+),?\s*$' % token_pfx).match
    outside = True

    for s in lines:
        if outside:
            outside = not _begin_tokens(s)
            continue

        idmatch = match_id(s)
        if idmatch:
            yield idmatch.groups()

        if _end_tokens(s): break

if __name__ == "__main__":
    import sys, os

    usage = "Usage:  %s [TOKEN_PREFIX]" % os.path.basename(sys.argv[0])
    if len(sys.argv) > 2:
        sys.exit("Too many arguments.\n" + usage)
    if len(sys.argv) == 2 and not re.match(r'^[A-Za-z_]\w*$', sys.argv[1]):
        sys.exit("Invalid token prefix '%s'. " \
                     "Token prefix must be a valid C identifier.\n" % sys.argv[1] + usage)


    import itertools, operator
    map(sys.stdout.write,
        itertools.imap(operator.mod,
                       itertools.repeat("%s\t%s\n"),
                       token_ids(sys.stdin, len(sys.argv) != 2 and '[A-Za-z_]' or sys.argv[1])))
    sys.exit(0)

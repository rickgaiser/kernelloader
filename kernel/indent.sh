#!/bin/bash
indent --blank-lines-after-declarations --blank-lines-after-procedures \
--blank-lines-before-block-comments --braces-after-struct-decl-line \
--braces-on-if-line --braces-on-struct-decl-line --indent-level4 \
--no-blank-lines-after-commas --dont-break-function-decl-args \
--no-space-after-function-call-names --no-space-after-parentheses \
--dont-break-procedure-type --dont-space-special-semicolon --ignore-profile \
--use-tabs --tab-size 4 --line-length 80 --comment-line-length 80 \
--parameter-indentation 4 --dont-line-up-parentheses $1

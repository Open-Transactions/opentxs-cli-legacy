#!/bin/bash

# bash-autocompletion script working with othint
# source the file to use autocompletion with ot application

_ot()
{
  local cur="${COMP_WORDS[COMP_CWORD]}" # current word
  local line="${COMP_LINE}" # entire line
  local all=$(./othint --complete-one "$line") # get all possibilities

  COMPREPLY=($(compgen -W "${all}" -- ${cur}))
}
complete -F _ot ot

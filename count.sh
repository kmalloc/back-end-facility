#! /usr/bin/env bash

function extract_souce_file
{
    git ls-files | sed '/\(\.cpp\|\.cc\|\.h\|\.c\|\.sh\|\.py\|\.pl\|Makefile\)$/!d'
}

echo "total files:"
extract_souce_file | wc -l

echo "total lines of code:"
extract_souce_file | xargs cat | sed -e '/^$/d' -e '/^\s\+$/d' | wc -l


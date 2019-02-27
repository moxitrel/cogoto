#!/bin/sh

cd $(dirname $0)/..

cmd=" clang "
cmd=" $cmd -fsyntax-only "              # preprocess, parse and type checking
cmd=" $cmd -Weverything "               # Enable all diagnostics
cmd=" $cmd -Wno-c++98-compat-pedantic " # Disable warn - incompatible with C++98
cmd=" $cmd -Wno-unused-macros "         # Disable warning: macro is not used
cmd=" $cmd -Wno-unused-function "       # Disable warning: unused function A
cmd=" $cmd -Wno-padded "                # Disable warnings about structure padding

$cmd -std=c99   src/*.h
$cmd -std=c++11 src/*.hpp

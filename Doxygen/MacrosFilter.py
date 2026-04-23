# This code from <ue4_doxygen_source_filter> by normalvector
# "https://github.com/normalvector/ue4_doxygen_source_filter"

#!env python3
import re
import sys


# This is from https://stackoverflow.com/questions/5454322/python-how-to-match-nested-parentheses-with-regex
def paren_matcher (n):
    # poor man's matched paren scanning, gives up
    # after n+1 levels.  Matches any string with balanced
    # parens inside; add the outer parens yourself if needed.
    # Nongreedy.
    return r"[^()]*?(?:\("*n+r"[^()]*?"+r"\)[^()]*?)*?"*n

# Check we have the right number of args
if len(sys.argv) != 2:
    print("Usage: filter_ue4_macros <C++ Filename>", file=sys.stderr)
    sys.exit(1)

# Get the filename from args
filename = sys.argv[1]

# Slurp file into a single string
file = open(filename, 'r', encoding='utf-8-sig')
if file.closed:
    print("Cannot read file", file=sys.stderr)
    sys.exit(1)
content = file.read()

# Do a regular expression to replace all UE4 macros, include balanced params
# TODO: Remove C++ comments!
regex = (
    r'^(\s*)('
    r'(?:UFUNCTION|UCLASS|UPROPERTY|UENUM|UMETA)\s*\(' + paren_matcher(25) + r'\)'
    r'|'
    r'(?:GENERATED_BODY|GENERATED_UCLASS_BODY)\s*\(\s*\)'
    r')'
)

content = re.sub(regex, r'\1/* UE4 Macro: \2 */', content, flags=re.MULTILINE)

# Output the content
print(content, end='')

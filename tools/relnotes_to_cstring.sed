#!/bin/sed -f

# This is a helper script to convert release notes written in Markdown
# to C-strings for langs/*.h
# This is not supposed to be perfect.

# (@user)
s/(@\([^)]*\))/(Thanks, \1!)/
# (commit)
s/ ([ 0-9a-f]\+)//

# escape "
s/"/\\"/g

# ## (Features:) → "  \1\n"
s/^## \(.*\)$/"  \1\\n"/

# * (Fix …)      → "    \1\n"
s/^* \(.*\)$/"    \1\\n"/

# (-…)           → "      \1\n"
s/^[[:space:]]\+\(-.*\)$/"      \1\\n"/

# **(…)**        → "  \1\n" [important notes]
s/^\*\*\(.*\)\*\*$/"  \1\\n"/

# c-string the rest
s/^\([^"].*\)$/"\1\\n"/
s/^$/"\\n"/

# Tool for GitHub CI logs to prepend time diffs between lines.
#
# Processes lines from stdin or from the files passed via argv.
#
from datetime import datetime
import fileinput


prevdate = datetime(1970, 1, 1)
for l in fileinput.input():
    datestr = l.split(maxsplit=1)[0]
    datestr = datestr[:-2] + datestr[-1:] # hack to limit µsec precision
    date = datetime.strptime(datestr, "%Y-%m-%dT%H:%M:%S.%fZ")
    print(f"{date - prevdate} | {l}", end='')
    prevdate = date

import subprocess
import time
import os
import re

branch = "?"
tag = "?"

GITHUB_REF = os.environ.get('GITHUB_REF')

if GITHUB_REF is not None:
  # Running in Github action
  BRANCH_TAG_RE = re.compile(r'^refs\/([^\/]+)\/(.*)?$')
  m = BRANCH_TAG_RE.match(GITHUB_REF)
  if m.group(1) is not None:
    if m.group(1) == "tags":
      # tagged
      tag = m.group(2)
      try:
        branch = subprocess.check_output("git branch -r --contains tags/" + tag, shell=True).decode().strip()
        if branch.startswith("origin/"):
          branch = branch[7:]
      except:
        branch = "unknown"
    elif m.group(1) == "heads":
      # branch
      branch = m.group(2)
      tag = "latest"

else:
  # running locally

  # get tag/base version
  try:
    tag = subprocess.check_output("git describe --tags --abbrev=0", shell=True).decode().strip()
  except:
    tag = "?"
  if tag.startswith('v'):
    tag = tag[1:]

  # get branch name
  try:
    branch = subprocess.check_output("git rev-parse --abbrev-ref HEAD", shell=True).decode().strip()
  except:
    branch = "unknown"

version = tag

# get current revision hash
try:
  commit = subprocess.check_output("git log --pretty=format:%h -n 1", shell=True).decode().strip()
except:
  commit = "?"

# if not main branch append branch name
if branch != "main" and branch != "HEAD":
  version += "-[" + branch + "]"

# check if clean
try:
  clean = subprocess.check_output("git status -uno --porcelain", shell=True).decode().strip()
except:
  clean = ""

ts = time.strftime('%Y%m%d%H%M%S')
# if not clean append timestamp
if clean != "":
  version += "-" + ts

# print build flags
print("'-DAPP_TAG=\"{0}\"'".format(tag))
print("'-DAPP_VERSION=\"{0}\"'".format(version))
print("'-DSRC_REVISION=\"{0}\"'".format(commit))
print("'-DSRC_BRANCH=\"{0}\"'".format(branch))
print("'-DBUILD_TIMESTAMP=\"{0}\"'".format(ts))

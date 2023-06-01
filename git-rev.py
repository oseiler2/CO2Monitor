import subprocess
import time
import os
import re

# get tag/base version
try:
  tag = subprocess.check_output("git describe --tags --abbrev=0", shell=True).decode().strip()
except:
  tag = "?"

GITHUB_TAG = os.environ.get('GITHUB_TAG')

if GITHUB_TAG is not None:
  tag = GITHUB_TAG

# Strip any leading v, and any trailing :name suffix.
TAG_RE = re.compile(r'^v?([^-]+)(-.*)?$')
m = TAG_RE.match(tag)
if m:
  tag = m.group(1)
version = tag

# get current revision hash
try:
  commit = subprocess.check_output("git log --pretty=format:%h -n 1", shell=True).decode().strip()
except:
  commit = "?"

GITHUB_SHA = os.environ.get('GITHUB_SHA')

if GITHUB_SHA is not None:
  commit = GITHUB_SHA

# get branch name
if GITHUB_TAG is not None:
  # When running in Github actions we're in a detached head, so we need to look at the remote branches to find the one that contains the tag
  try:
    branch = subprocess.check_output("git branch -r --contains tags/" + GITHUB_TAG, shell=True).decode().strip()
    if branch.startswith("origin/"):
      branch = branch[7:]
  except:
    branch = "unknown"
else:
  try:
    branch = subprocess.check_output("git rev-parse --abbrev-ref HEAD", shell=True).decode().strip()
  except:
    branch = "unknown"

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

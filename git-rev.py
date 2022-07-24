import subprocess
import time
import os

# get tag/base version
try:
  tag = subprocess.check_output("git describe --tags --abbrev=0", shell=True).decode().strip()
except:
  tag = "?"

GITHUB_TAG = os.environ.get('GITHUB_TAG')

if GITHUB_TAG is not None:
  tag = GITHUB_TAG

if tag.startswith('v'):
  tag = tag[1:]
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
try:
  branch = subprocess.check_output("git rev-parse --abbrev-ref HEAD", shell=True).decode().strip()
except:
  branch = "unknown"

if GITHUB_TAG is not None:
  branch = "main"

# if not main branch append branch name
if branch != "main":
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

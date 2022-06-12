import subprocess
import time

# get tag/base version
version = tag = subprocess.check_output("git describe --tags --abbrev=0", shell=True).decode().strip()

# get current revision hash
commit = subprocess.check_output("git log --pretty=format:%h -n 1", shell=True).decode().strip()

# get branch name
branch = subprocess.check_output("git rev-parse --abbrev-ref HEAD", shell=True).decode().strip()
# if not main branch append branch name
if branch != "main":
  version += "-[" + branch + "]"

# check if clean
clean = subprocess.check_output("git status -uno --porcelain", shell=True).decode().strip()
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

import subprocess
import time
import os
import os.path
import re
from SCons.Script import DefaultEnvironment

try:
    import configparser
except ImportError:
    import ConfigParser as configparser


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
        if branch.find("origin/main") >= 0:
          branch = "main"
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

ts = time.strftime('%Y%m%d%H%M%S')

if GITHUB_REF is None:
  # check if clean
  try:
    subprocess.run("git restore include/version.h")
  except:
    print("ignoring exception")

  try:
    clean = subprocess.check_output("git status -uno --porcelain", shell=True).decode().strip()
  except:
    clean = ""

  # if not clean append timestamp
  if clean != "":
    version += "-" + ts

# print build flags
print("APP_TAG=\"{0}\"".format(tag))
print("APP_VERSION=\"{0}\"".format(version))
print("SRC_REVISION=\"{0}\"".format(commit))
print("SRC_BRANCH=\"{0}\"".format(branch))
print("BUILD_TIMESTAMP=\"{0}\"".format(ts))

# get platformio environment variables
env = DefaultEnvironment()
env.Append(APP_VERSION = version)
env.Append(BUILD_TIMESTAMP = ts)
config = configparser.ConfigParser()
config.read("platformio.ini")

# get platformio source path
srcdir = env.get("PROJECT_SRC_DIR")

print ("PROJECT_SRC_DIR: " + srcdir)

# check if lmic config file is present in source directory
versionconstants = config.get("env", "versionconstants")
versionconstantsfile = os.path.join (srcdir, "../include", versionconstants)

if os.path.exists(versionconstantsfile):
  os.remove(versionconstantsfile)
f = open(versionconstantsfile, "x")
f.write('#define APP_TAG         "{0}"\n'.format(tag))
f.write('#define APP_VERSION     "{0}"\n'.format(version))
f.write('#define SRC_REVISION    "{0}"\n'.format(commit))
f.write('#define SRC_BRANCH      "{0}"\n'.format(branch))
f.write('#define BUILD_TIMESTAMP "{0}"\n'.format(ts))

f.close

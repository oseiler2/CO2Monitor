import subprocess
import time

Import("env")

config = env.GetProjectConfig()
release_version = config.get("common", "release_version")

pioenv= env.get('PIOENV')
ts = time.strftime('%Y%m%d%H%M%S')

def Command(args):
  ret = subprocess.run(args, stdout=subprocess.PIPE, text=True)
  return ret.stdout.strip()

def GetRev():
  rev = Command(["git", "rev-parse", "--short", "HEAD"])
  clean = Command(["git", "status", "-uno", "--porcelain"])
  if clean.strip() != "":
    rev += "++"
    return rev
  return rev

version = filetag = release_version
if pioenv == "debug":
  # For non release builds, increment the patch level and add a pre-release section with the build timestamp
  parts = list(map(int, version.split('.')))
  parts[-1]+=1
  version = '%s-%s' % ('.'.join(map(str, parts)), ts)
  filetag = ts

# Rename firmware files with version for convenience
env.Replace(PROGNAME="co2monitor_%s" % filetag)

# Add defines for use in code
env.Append(BUILD_FLAGS=[
  '-D BUILD_TIMESTAMP=\\\"%s\\\"' % ts,
  '-D GIT_REV=\\\"%s\\\"' % GetRev(),
  '-D VERSION=\\\"%s\\\"' % version
])


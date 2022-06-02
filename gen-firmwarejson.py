#!/usr/bin/python
#
# Scan firmware/images in a directory and generate firmware.json needed by
# esp32FOTA to point to the most recent version for each different model
# available.
#
# Expects firmwares to be named co2monitor_MODEL_VERSION.bin.
import json
import os
import re

fw = {}

FW_PATTERN = re.compile(r'co2monitor_([a-z]+)_([0-9]+\.[0-9]+\.[0-9]+).bin')
for f in os.scandir(os.getcwd()):
  m = FW_PATTERN.match(f.name)
  if not m:
    continue
  model, version = m.groups()
  fw.setdefault(model, [])
  fw[model].append(version)

base_url = ''
with open('urlbase.conf', 'r') as fp:
  base_url = fp.read().strip()

best = []
for model, versions in fw.items():
  newest = sorted(versions)[-1]
  best.append({
    'type': 'co2monitor-%s' % model,
    'version':newest,
    'url': os.path.join(base_url, 'co2monitor_%s_%s.bin' % (model, newest)),
    })

with open('firmware.json', 'w') as fp:
  fp.write(json.dumps(best, indent=2, sort_keys=True))

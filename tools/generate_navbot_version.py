# vim: set ts=8 sts=2 sw=2 tw=99 et:
import re
import os, sys
import subprocess

argv = sys.argv[1:]
if len(argv) < 2:
  sys.stderr.write('Usage: generate_navbot_version.py <source_path> <output_folder>\n')
  sys.exit(1)

SourceFolder = os.path.abspath(os.path.normpath(argv[0]))
OutputFolder = os.path.normpath(argv[1])

class FolderChanger:
  def __init__(self, folder):
    self.old = os.getcwd()
    self.new = folder

  def __enter__(self):
    if self.new:
      os.chdir(self.new)

  def __exit__(self, type, value, traceback):
    os.chdir(self.old)

def run_and_return(argv):
  text = subprocess.check_output(argv)
  if str != bytes:
    text = str(text, 'utf-8')
  return text.strip()

def get_git_version():
  revision_hash = run_and_return(['git', 'log', '--pretty=format:%h:%H', '-n', '1'])
  url = run_and_return(['git', 'config', '--get', 'remote.origin.url'])
  shorthash, longhash = revision_hash.split(':')
  cleanurl = url.removesuffix('.git')

  return shorthash, longhash, cleanurl

def output_version_headers():
  with FolderChanger(SourceFolder):
    shorthash, longhash, url = get_git_version()

  with open(os.path.join(SourceFolder, 'product.version')) as fp:
    contents = fp.read().strip()
  m = re.match(r'(\d+)\.(\d+)\.(\d+)-?(.*)', contents)
  if m == None:
    raise Exception('Could not detremine product version')
  major, minor, patch, tag = m.groups()
  product = "{0}.{1}.{2} - {3}".format(major, minor, patch, shorthash)
  numeric = "{0}.{1}.{2}".format(major, minor, patch)

  with open(os.path.join(OutputFolder, 'navbot_version_auto.h'), 'w') as fp:
    fp.write("""
#ifndef __NAVBOT_AUTO_VERSION_INFORMATION_H_
#define __NAVBOT_AUTO_VERSION_INFORMATION_H_

#define NAVBOT_BUILD_SHORT_HASH     \"{0}\" // short commit hash
#define NAVBOT_BUILD_LONG_HASH      \"{1}\" // full commit hash
#define NAVBOT_BUILD_FULL_VERSION   \"{2}\" // numeric version + short commit hash
#define NAVBOT_BUILD_VERSION        \"{3}\" // numeric version only
#define NAVBOT_BUILD_GIT_REMOTE_URL \"{4}\"

#endif /* __NAVBOT_AUTO_VERSION_INFORMATION_H_ */
    """.format(shorthash, longhash, product, numeric, url))

output_version_headers()
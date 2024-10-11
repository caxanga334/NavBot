# Package script
import pathlib, shutil

RootPath = pathlib.Path(__file__).parents[0]
BasePath = pathlib.Path(__file__).parents[0].joinpath('output/')
BinsDir = pathlib.Path(__file__).parents[0].joinpath('build/bin/')

if not BasePath.exists():
  BasePath.mkdir(0o755, True, True)

CreateDirs = [
  'addons/sourcemod/configs/navbot/',
  'addons/sourcemod/configs/navbot/tf/',
  'addons/sourcemod/gamedata/navbot.games',
  'addons/sourcemod/extensions/x64/',
  'addons/sourcemod/scripting/include',
  'addons/sourcemod/translations/',
]

for folder in CreateDirs:
  NewFolder = BasePath.joinpath(folder)
  NewFolder.mkdir(0o755, True, True)
  print("Creating folder: " + str(NewFolder))

shutil.copytree(RootPath.joinpath('configs'), BasePath.joinpath('addons/sourcemod/configs/navbot/'), dirs_exist_ok=True)
shutil.copytree(RootPath.joinpath('gamedata'), BasePath.joinpath('addons/sourcemod/gamedata/'), dirs_exist_ok=True)
shutil.copytree(RootPath.joinpath('scripting'), BasePath.joinpath('addons/sourcemod/scripting/'), dirs_exist_ok=True)
shutil.copytree(RootPath.joinpath('translations'), BasePath.joinpath('addons/sourcemod/translations/'), dirs_exist_ok=True)
shutil.copy(RootPath.joinpath('extension/navbot.autoload'), BasePath.joinpath('addons/sourcemod/extensions/navbot.autoload'))

ValidFiles = [
  '.dll',
  '.pdb',
  '.so'
]

x86Bins = BinsDir.joinpath('x86', 'release')

if x86Bins.exists():
  allfiles = x86Bins.glob('*')

  for file in allfiles:
    if file.suffix in ValidFiles:
      shutil.copy(file, BasePath.joinpath('addons/sourcemod/extensions/' + file.name))

x64Bins = BinsDir.joinpath('x86_64', 'release')

if x64Bins.exists():
  allfiles = x64Bins.glob('*')

  for file in allfiles:
    if file.suffix in ValidFiles:
      shutil.copy(file, BasePath.joinpath('addons/sourcemod/extensions/x64/' + file.name))

import pathlib, subprocess, textwrap

StoreDir = pathlib.Path(__file__).parents[0].joinpath('versioning/include/')
StoreDir.mkdir(0o755, True, True)
OutFile = StoreDir / 'auto_version.h'

print(OutFile)

def git_output():
    out = str('NONE')

    try:
        out = subprocess.check_output(['git', 'log', '--pretty=format:%H', '-n', '1']).decode('ascii')
    except subprocess.CalledProcessError as e:
        print(e)

    return out

def generate_version_file():
    with open(str(OutFile), 'wt') as f:
        values = {
            'git_hash': git_output(),
            'git_short_hash': git_output()[:8]
        }
        f.write(textwrap.dedent("""
            #ifndef NAVBOT_VERSIONING_H_
            #define NAVBOT_VERSIONING_H_
            
            #define GIT_COMMIT_HASH "{git_hash}" // git commit hash or NONE if generated outside a git clone
            #define GIT_COMMIT_SHORT_HASH "{git_short_hash}" // shorter commit hash
            
            #endif // !NAVBOT_VERSIONING_H_
        """.format(**values))[1:])

print('Generating versining file...')
generate_version_file()
# Utility to patch the version info by modifying *.rc files.

import sys
import subprocess

def PatchRC (filename,  version):
    '''Patch .rc file (native C/C++ projects)'''
    
    tokens = version.split('.') # assume '.'-separated input
    major = tokens[0]
    minor = tokens[1]
    patch = '0'
    if len(tokens) > 2:
        patch = tokens[2]
    build = '0'
    if len(tokens) > 3:
        build = tokens[3]
    
    version_dot   = major+'.'+minor+'.'+patch+'.'+build
    version_comma = major+','+minor+','+patch+','+build
    
    with open(filename, 'r',  encoding="utf-16") as file:
        lines = file.readlines()
    
    with open(filename, 'w', encoding="utf-16") as file:
        for line in lines:
            if '0.0.0.0' in line:
                # replace '0.0.0.0' with current version
                line = line.replace('0.0.0.0', version_dot)
            elif '0,0,0,0' in line:
                # replace '0,0,0,0' with current version
                line = line.replace('0,0,0,0', version_comma)
            
            file.write(line)


# entry point
if __name__ == "__main__":
    # filename as command-line argument
    filename = sys.argv[1]
    try:
        # query git tag for current commit
        result = subprocess.run("git describe --tags --exact-match", check=True, stdout=subprocess.PIPE)
        version = result.stdout.decode("utf-8")
        version = version[1:].strip() # remove "v" prefix and newline suffix
    except subprocess.CalledProcessError:
        print("Skipping version update since no git tag was found")
        exit()
    
    PatchRC(filename, version)


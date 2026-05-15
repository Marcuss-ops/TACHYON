import os, glob

with open('sfx_files.txt', 'w', encoding='utf-8') as f:
    for file in glob.glob('src/audio/SoundEffect/**/*.m4a', recursive=True):
        folder = os.path.basename(os.path.dirname(file))
        name = os.path.basename(file)
        f.write(f'    "{folder}/{name}",\n')

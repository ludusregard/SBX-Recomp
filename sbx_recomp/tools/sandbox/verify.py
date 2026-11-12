"""Checks the PC's re-staged files against a batch's hashes; on success the
batch is applied to the PC mirror (pc)."""
import sys, json, hashlib, os, shutil
name = sys.argv[1]; info = json.load(open(f'/home/claude/tools/{name}.json'))
bad = [r for r, h in info['hashes'].items()
       if hashlib.sha256(open(('/mnt/user-data/uploads/sbx/' if '/' not in r else '/mnt/user-data/uploads/') + r, 'rb').read()).hexdigest() != h]
print(len(info['hashes']), 'files,', len(bad), 'mismatched', bad)
if not bad:
    for r in info['hashes']:
        os.makedirs(os.path.dirname('/home/claude/pc/' + r), exist_ok=True)
        shutil.copy(f'/mnt/user-data/outputs/{name}/{r}', '/home/claude/pc/' + r)
    print('mirror updated; still to delete on the PC:', info['gone'])

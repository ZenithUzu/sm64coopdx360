#!/usr/bin/env python3
"""Audit Git's current index, not private files in the developer's working tree."""
import pathlib,re,subprocess,sys

BLOCKED={'.z64','.n64','.v64','.rom','.xex','.exe','.dll','.lib','.a','.so','.dylib',
 '.bin','.tex','.ctl','.tbl','.m64','.aiff','.aifc','.o','.obj','.pdb','.elf','.png','.jpg',
 '.jpeg','.gif','.ico','.icns','.bmp','.wav','.mp3','.ogg','.zip','.7z','.gz','.tgz','.ttf','.otf','.tga','.col','.lvl','.bhv','.bundle'}
PRIVATE_NAMES={'.env','net_log.txt','dbgprint.txt','sm64config.txt','sm64_save_file.bin','bank_sets_compressed','.DS_Store'}
SECRET_PATTERNS=[re.compile(rb'gh[pousr]_[A-Za-z0-9]{30,}'),re.compile(rb'github_pat_[A-Za-z0-9_]{40,}'),
 re.compile(rb'AKIA[0-9A-Z]{16}'),re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----')]

def main():
    root=pathlib.Path(__file__).resolve().parents[1]
    names=[p for p in subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().split('\0') if p]
    payload=''.join(':'+p+'\n' for p in names).encode()
    result=subprocess.run(['git','cat-file','--batch'],cwd=root,input=payload,stdout=subprocess.PIPE,check=True).stdout
    errors=[];offset=0
    for name in names:
        end=result.index(b'\n',offset);header=result[offset:end].split();offset=end+1
        if len(header)!=3 or header[1]!=b'blob':errors.append((name,'cannot read indexed blob'));continue
        size=int(header[2]);data=result[offset:offset+size];offset+=size+1
        path=pathlib.PurePosixPath(name)
        if path.suffix.lower() in BLOCKED or path.name in PRIVATE_NAMES or path.name.startswith('.env.'):
            errors.append((name,'prebuilt/media/private file'))
        if b'\0' in data[:8192]:errors.append((name,'binary content; inspect provenance'))
        if len(data)>16*1024*1024:errors.append((name,'oversized file'))
        if any(p.search(data) for p in SECRET_PATTERNS):errors.append((name,'possible credential; inspect locally without printing it'))
        if re.search(rb'[A-Za-z]:[\\/]Users[\\/][^\s\\/]+',data):errors.append((name,'personal absolute Windows path'))
    for name,reason in errors:print(f'{name}: {reason}',file=sys.stderr)
    print(f'Audited {len(names)} indexed source files; {len(errors)} finding(s).')
    return 1 if errors else 0
if __name__=='__main__':sys.exit(main())

# Copyright 2020 Randal A. Koene
# License TBD

"""
This header file declares functions used to call Formalizer command line
tools through a subsystem process or tty shell.

Versioning is based on https://semver.org/. See coreversion.hpp for more.
"""

import subprocess
import error

results = {}

def try_subprocess_check_output(thecmdstring, resstore, config: dict):
    if config['verbose']:
        print(f'Calling subprocess: `{thecmdstring}`', flush=True)
    if config['logcmdcalls']:
        with open(config['cmdlog'],'a') as f:
            f.write(thecmdstring+'\n')
    try:
        res = subprocess.check_output(thecmdstring, shell=True)

    except subprocess.CalledProcessError as cpe:
        if config['logcmderrors']:
            with open(config['cmderrlog'],'a') as f:
                f.write(f'Subprocess call ({thecmdstring}) caused exception.\n')
                f.write(f'Error output: {cpe.output.decode()}\n')
                f.write(f'Error code  : {cpe.returncode}\n')
                if (cpe.returncode>0):
                    f.write('Formalizer error: '+error.exit_status_code[cpe.returncode]+'\n')
        if config['verbose']:
            print('Subprocess call caused exception.')
            print('Error output: ',cpe.output.decode())
            print('Error code  : ',cpe.returncode)
            if (cpe.returncode>0):
                print('Formalizer error: ', error.exit_status_code[cpe.returncode])
        return cpe.returncode

    else:
        if resstore:
            results[resstore] = res
        if config['verbose']:
            print('Result of subprocess call:', flush=True)
            print(res.decode(), flush=True)
        return 0

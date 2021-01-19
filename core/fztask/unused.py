def old_make_log_entry():
    print('Launching logentry...')
    #thecmd = 'logentry'
    #if config['verbose']:
    #    thecmd += ' -v'
    #retcode = try_subprocess_check_output(thecmd, 'logentry_res')
    thecmd = ['logentry']
    if config['verbose']:
        thecmd = ['logentry','-v']
    retcode = pty.spawn(thecmd)
    print('Back from logentry.')
    if not os.WIFEXITED(retcode):
        exit_error(os.WEXITSTATUS(retcode), 'Attempt to make Log entry failed.')


#def a_function_that_calls_subprocess(some_arg, resstore):
#    retcode = try_subprocess_check_output(f"someprogram -A '{some_arg}'", resstore)
#    if (retcode != 0):
#        print(f'Attempt to do something failed.')
#        exit(retcode)
#
#
#def a_function_that_spawns_a_call_in_pseudo_TTY(some_arg):
#    retcode = pty.spawn(['someprogram','-A', some_arg])
#    return retcode


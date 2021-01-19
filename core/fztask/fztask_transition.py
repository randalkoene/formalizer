# Import this to use the transition option in fztask, so that Formalizer 1.x
# dil2al operations are requested.

def set_DIL_entry_preset(node):
    topic = get_main_topic(node)
    dilpreset = f'{topic}.html#{node}:!'
    print(f'Specifying the DIL ID preset: {dilpreset}')
    with open(userhome+'/.dil2al-DILidpreset','w') as f:
        f.write(dilpreset)


#def transition_dil2al_request(recent_node, node):
def transition_dil2al_request(node, args):
    # - set 'flagcmd' in dil2al/controller.cc:chunk_controller() such that no alert is called
    # - provide a command string to automatically answer confirmation() 'N' about making a note
    # - provide 'S' or 't' to start a new chunk or simply close the chunk, for auto_interactive()
    # - case 'S': decide_add_TL_chunk(): perhaps set timechunkoverstrategy=TCS_NEW and timechunkunderstrategy,
    #   then like DIL_entry selection in logentry
    # - case 't': stop_TL_chunk(): could modify alautoupdate if I want to prevent AL update and
    #   set completion ratios directly instead of automatically
    # - set 'isdaemon' false to prevent waiting in loop in schedule_controller()
    # - possibly also prevent an at-command from being created
    # *** If this is all too difficult, I can just call `dil2al -C` and let me figure it out manually.
    #     And note that `dil2al -u` sets alautoupdate to no, yes or ask. Note that `dil2al -C` does
    #     not appear to set a timer or at-command (in fact, it seems that the `at` program is not
    #     even installed on aether).
    print('For transition synchronization back to Formalizer 1.x:')
    set_DIL_entry_preset(node)
    print(f'  preset DIL entry selection to {node}')
    # thecmd = "urxvt -e dil2al -C -u no -p 'noaskALDILref' -p 'noshowflag'"
    # print(f'  calling `{thecmd}` for:\n  alautoupdate=no, no timer setting, and use the (preset) default as the DIL entry')
    thecmd = "urxvt -e dil2al -C -u yes -p 'noaskALDILref' -p 'noshowflag'"
    if args.T_emulate:
        thecmd += ' -T ' + args.T_emulate
    print(f'  calling `{thecmd}` for:\n  alautoupdate=yes, no timer setting, and use the (preset) default as the DIL entry')
    retcode = try_subprocess_check_output(thecmd, 'dil2al_chunk')
    exit_error(retcode, 'Call to dil2al -C failed.')

    # *** I can'd do the steps below and need to let dil2al do its thing instead, because of the
    #     complex nature of targetdate updates through links to Superiors.
    # get_completion_required(node)
    # completion = results['completion']
    # required = results['required']
    # print(f'  synchronizing {recent_node} completion={completion} and required={required}')
    # thecmd = f"urxvt -e w3m '/cgi-bin/dil2al?dil2al=MEi&DILID={recent_node}&required={required}&completion={completion}'"
    # retcode = try_subprocess_check_output(thecmd, 'dil2al_compreq')
    # exit_error(retcode, 'GET call to dil2al failed.')



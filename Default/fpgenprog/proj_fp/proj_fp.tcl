open_project -project {C:/Microchip/SoftConsole-v6.5/extras/workspace.testing/hart-software-services/Default/fpgenprog\proj_fp\proj_fp.pro}
enable_device -name {target} -enable 1
set_programming_file -name {target} -file {C:/Microchip/SoftConsole-v6.5/extras/workspace.testing/hart-software-services/Default/fpgenprog\proj_fp\target.ppd}
set_programming_action -action {PROGRAM} -name {target}
run_selected_actions
save_project
close_project

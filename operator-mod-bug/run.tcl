set CFLAGS ""
set hls_prj "evil.prj"
open_project ${hls_prj} -reset
set_top dut

add_files evil.cc -cflags $CFLAGS
add_files -tb evil_test.cc -cflags $CFLAGS

open_solution "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10

csim_design
csynth_design

# We will skip C-RTL cosimulation for now
#cosim_design

quit

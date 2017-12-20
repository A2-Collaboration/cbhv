# CBHV
Scripts and tools for the Crystal Ball High Voltage

## Calibration of CBHV cards

Everytime a PMT get's exchanged or even a splitter card is broken and exchanged, a calibration of the correction values has to be done. There are some crucial steps:

* Stop the CBHV EPICS IOC on slowcontrol
* Set correction loop and correction values to 0 using the script `cbhv_set_values.py` with the option -r or --reset
* Get a Windows PC inside the KPh network (yes, sorry)
* Open PowerShell programme as administrator  
  first command:  
  `Set-ExecutionPolicy RemoteSigned`
  or, if that doesn't work
  `Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass`
  then you can execute the following script (please edit where necessary), which will take around 6-8h to finish: `cbhv_measure_corr_values.ps1`
* analyse data with `root` and `cbhv_calibrate_boxes.C`
* optional: convert all ps to pdf and add them together with `convert_add_ps2pdf.sh`
* finally set the correction values with `cbhv_set_corr_values.py`, providing the input file eg.
  `.\cbhv_set_corr_values.py -i HV_gains_offsets.txt`
  This will also activate the correction loop again

### ToDo

* convert all Powershell and Bash scripts to useful python scripts

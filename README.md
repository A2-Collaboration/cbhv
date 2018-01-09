# CBHV
Scripts and tools for the Crystal Ball High Voltage

## Calibration of CBHV cards

Everytime a PMT get's exchanged or even a splitter card is broken and exchanged, a calibration of the correction values has to be done. There are some crucial steps:

* Stop the CBHV EPICS IOC on slowcontrol
* Set correction loop and correction values to 0 using the script `cbhv_control.py` with the option `-r` or `--reset`
* Measure the new correction values using the same script `cbhv_control.py` with the option `-c` or `--calibrate`;
  the measurement will take around 6-8h to finish
* analyse data with `root` and `cbhv_calibrate_boxes.C`
* optional: convert all ps to pdf and add them together with `convert_add_ps2pdf.sh`
* finally set the correction values with `cbhv_control.py`, providing the input file eg.
  `.\cbhv_control.py -i HV_gains_offsets.txt`
  This will also activate the correction loop again



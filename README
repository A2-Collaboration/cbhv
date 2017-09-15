# CBHV
Scripts and tools for the Crystal Ball High Voltage

## Calibration of CBHV cards

Everytime a PMT get's exchanged or even a splitter card is broken and exchanged, a calibration of the correction values has to be done. There are some crucial steps:

* Stop the CBHV EPICS IOC on slowcontrol
* Set correction loop and correction values to 0 per bash script `delete_correctionvalues.sh`
* Get a Windows PC inside the KPh network (yes, sorry)
* Open PowerShell programme as administrator  
  first command:  
  `Set-ExecutionPolicy RemoteSigned`  
  then you can execute the following script (please edit where necessary), which will take around 6-8h to finish: `hvcb_spannungsverlauf_alle_hvboxen.zip`
* analyse data with `root` and `cbhv_calibrate_boxes.C`
* optional: convert all ps to pdf and add them together with `convert_add_ps2pdf.sh`
* finally set the correction values with `cbhv_set_corr_values.py`
* don't forget to activate the correction loop after that

### ToDo

* convert all Powershell and Bash scripts to useful python scripts

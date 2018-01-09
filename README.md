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

## Additional options

* You can specify only certain boxes by using `-b` or `--boxes` and providing a list of boxes, e.g. `-b 1 2 4 12`
* For the calibration you might want to change the stepping or the voltage range, `-s 20 --range 1300 1500`
* For a full list of options run `cbhv_control.py` with `-h` or `--help`


## Tips

By default, running the ROOT macro creates a PDF with the histograms for the different channels. For every card a PDF is created. If you want to merge them to one file, you may want to use `pdftk`:

    pdftk HV_gains_offsets*.pdf cat output HV_gains_all_cards.pdf

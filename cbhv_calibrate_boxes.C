#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>

#include <TROOT.h>
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TStyle.h>

using namespace std;

void Karte(const size_t box_start = 1, const size_t box_stop = 18, const bool SaveTxt = true,
           const string TxtFileName = "HV_gains_offsets.txt", const bool SavePlots = true)
{
	// the following values can be used to control different routines of this macro
	const double fit_min = 1400;
	const double fit_max = 1650;
	const char* input_file_format = "box%02d_card%d.txt";
	const char* plot_output_format = "HVgains_box%02d_card%d.pdf";

	// do not change anything of the following code unless you know what you're doing
	const size_t N_BOXES = 18;     // number of boxes
	const size_t N_CARDS = 5;      // number of cards per box
	const size_t N_CHANNELS = 8;   // number of channels per card
	FILE *InFile;
	FILE *TxtFile;
	char FileName[255];
	char plotFileName[100];
	char line[512];
	char HistName[100];
	char HistTitle[100];
	TH1F *hist[N_CHANNELS];
	TF1 *f[N_CHANNELS];
	char fname[100];
	double xlow, xup;

	//Setting up the canvas
	TCanvas *hvcanv = new TCanvas("HV", "HV", 0, 0, 1200, 800);
	hvcanv->SetFillStyle(4000);
	hvcanv->Divide(3, 3, 0.001, 0.001);

	if (SaveTxt) {
		TxtFile = fopen(TxtFileName.c_str(), "w");
		if (!TxtFile) {
			cerr << "Error opening output file" << endl;
			exit(1);
		}
		fprintf(TxtFile, "#CardNo,Channel,Slope,Offset\n");
	}

	vector<vector<double>> channel_values;
	for (size_t i = 0; i < N_CHANNELS; i++) {
		vector<double> v;
		channel_values.push_back(v);
	}
	vector<double> setHV;
	double hv, ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7;
	// Loop over all chosen boxes
	for (size_t box = box_start; box <= box_stop; box++) {
	for (size_t card = 0; card < N_CARDS; card++) {

		// Set file name
		sprintf(FileName, input_file_format, box, card);

		// Open file
		InFile = fopen(FileName, "r");
		if (!InFile) {
			// File opening did not work
			cerr << "File '" << FileName << "' not found." << endl;
			continue;
		}
		// File opening worked
		fscanf(InFile, "%s", line);	// Skip the first line, it is one big string
		setHV.clear();
		for (size_t c = 0; c < channel_values.size(); c++)
			channel_values.at(c).clear();
		fscanf(InFile, "%lf,%*s %*[^,],%*d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%*s",
		       &hv, &ch0, &ch1, &ch2, &ch3, &ch4, &ch5, &ch6, &ch7);  // Read in normal line, use comma as delimiter (mostly)
		// Check whether read line was already at the end of file
		while (!feof(InFile)) {
			setHV.push_back(hv);
			channel_values[0].push_back(ch0);
			channel_values[1].push_back(ch1);
			channel_values[2].push_back(ch2);
			channel_values[3].push_back(ch3);
			channel_values[4].push_back(ch4);
			channel_values[5].push_back(ch5);
			channel_values[6].push_back(ch6);
			channel_values[7].push_back(ch7);
			fscanf(InFile, "%lf,%*s %*[^,],%*d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%*s",
		       &hv, &ch0, &ch1, &ch2, &ch3, &ch4, &ch5, &ch6, &ch7);  // Read in normal line, use comma as delimiter (mostly)
		}
		// Finding xlow, xup boundries
		xlow = *min_element(setHV.begin(), setHV.end());
		xup = *max_element(setHV.begin(), setHV.end());

		// Loop over all channels from one board
		for (size_t channel = 0; channel < N_CHANNELS; channel++) {
			// Highlight next pad
			hvcanv->cd(channel + 1);
			sprintf(HistName, "box%d_board%d_channel%d", box, card, channel);
			sprintf(HistTitle, "Box%02d Board%d", box, card);
			hist[channel] = new TH1F(HistName, HistTitle, setHV.size(), xlow - 5, xup + 5);
			hist[channel]->SetMarkerStyle(2);
			hist[channel]->SetMarkerColor(2);
			//Loop for the number of data points for 1 channel
			for (size_t n = 0; n < setHV.size(); n++)
				hist[channel]->SetBinContent(n + 1, channel_values[channel][n] - setHV[n]);

			sprintf(fname, "f%d_%d_%d", box, card, channel);
			//f[channel] = new TF1(fname, "pol1" , xlow, xup);
			f[channel] = new TF1(fname, "pol1", fit_min, fit_max);
			f[channel]->SetLineWidth(1);
			gStyle->SetOptStat("n");
			hist[channel]->Fit(f[channel], "R0Q");
			hist[channel]->Draw("P");
			f[channel]->Draw("same");
			float par0 = f[channel]->GetParameter(0);
			float par1 = f[channel]->GetParameter(1);
			if (SaveTxt)
				fprintf(TxtFile, "%d,%d,%d,%f,%f\r\n", box, card, channel, par1, par0);
		}
		hvcanv->Update();
		if (SavePlots) {
			sprintf(plotFileName, plot_output_format, box, card);
			hvcanv->SaveAs(plotFileName);
			cout << "Saved histograms to " << plotFileName << endl;
		}
		fclose(InFile);
	}}
	if (SaveTxt) {
		fclose(TxtFile);
		cout << "Saved values to " << TxtFileName << endl;
	}
}


int cbhv_calibrate_boxes(size_t start = 1, size_t stop = 18)
{
	cout << "Start calibrating cards from box " << start << " to box " << stop << endl;
	Karte(start, stop);
	return 0;
}

int main(int argc, char **argv)
{
	if (argc > 3) {
		cerr << "Too many arguments!" << endl;
		return argc;
	}

	int start = 1;
	int stop = 18;
	if (argc >= 2) {
		if (*argv[1] == '0')  // !int(0) is true, so using only the other if condition would make it impossible to pass a 0 through without error
			start = 0;
		else if (!(start = atoi(argv[1]))) {
			cerr << "Invalid input! First argument is not an integer." << endl;
			return 1;
		}
	}
	if (argc == 3) {
		if (*argv[2] == '0')
			stop = 0;
		else if (!(stop = atoi(argv[2]))) {
			cerr << "Invalid input! Second argument is not an integer." << endl;
			return 1;
		}
	}

	return cbhv_calibrate_boxes(start, stop);
}

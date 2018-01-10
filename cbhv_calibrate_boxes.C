#include <stdio.h>
#include <fstream.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <TF1.h>

void Karte(UInt_t start, UInt_t stop, Bool_t SaveTxt = true,
		   char *TxtFileName = "HV_gains_offsets.txt", Bool_t SavePlots = true)
{
	// the following values can be used to control different routines of this macro
	const double fit_min = 1400;
	const double fit_max = 1650;
	const char* input_file_format = "karte%03d.txt";
	const char* plot_output_format = "HV_gains_offsets%03d.pdf";

	// do not change anything of the following code unless you know what you're doing
	const size_t max_size = 1000;  // allocate arrays to hold at maximum this amount of measured values
	const size_t n_channels = 8;  // channels per board
	FILE *InFile;
	FILE *TxtFile;
	char FileName[255];
	char plotFileName[100];
	char line[max_size];
	TH1F *hist[n_channels];
	char HistName[max_size];
	char HistTitle[max_size];
	TF1 *f[n_channels];
	Float_t xlow, xup;
	char fname[100];

	//Setting up the canvas
	TCanvas *hvcanv = new TCanvas("HV", "HV", 0, 0, 1200, 800);
	hvcanv->SetFillStyle(4000);
	hvcanv->Divide(3, 3, 0.001, 0.001);

	if (SaveTxt) {
		TxtFile = fopen(TxtFileName, "w");
		if (!TxtFile) {
			printf("Error opening output file\n");
			exit(1);
		}
		fprintf(TxtFile, "#CardNo,Channel,Slope,Offset\n");
	}

	vector<vector<double>> channel_values;
	for (size_t i = 0; i < n_channels; i++) {
		vector<double> v;
		channel_values.push_back(v);
	}
	vector<double> setHV;
	double hv, ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7;
	// Loop over all chosen files
	for (size_t i = start; i <= stop; i++) {

		// Set file name
		sprintf(FileName, input_file_format, i);

		// Open file
		InFile = fopen(FileName, "r");
		if (!InFile) {
			// File opening did not work
			printf("File '%s' not found.\n", FileName);
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
		//Loop over all channels from one board
		for (size_t channel = 0; channel < n_channels; channel++) {
			// Highlight next pad
			hvcanv->cd(channel + 1);
			sprintf(HistName, "Channel%d_Board%d", channel, i);
			sprintf(HistTitle, "Board%d", i);
			hist[channel] = new TH1F(HistName, HistTitle, setHV.size(), xlow - 5, xup + 5);
			hist[channel]->SetMarkerStyle(2);
			hist[channel]->SetMarkerColor(2);
			//Loop for the number of data points for 1 channel
			for (size_t n = 0; n < setHV.size(); n++)
				hist[channel]->SetBinContent(n + 1, channel_values[channel][n] - setHV[n]);

			sprintf(fname, "f%d_%d", i, channel);
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
				fprintf(TxtFile, "%d,%d,%f,%f\r\n", i, channel, par1, par0);
		}
		hvcanv->Update();
		if (SavePlots) {
			sprintf(plotFileName, plot_output_format, i);
			hvcanv->SaveAs(plotFileName);
			printf("Saved histograms to %s\n", plotFileName);
		}
		fclose(InFile);
	}
	if (SaveTxt) {
		fclose(TxtFile);
		printf("Saved values to %s\n", TxtFileName);
	}
}

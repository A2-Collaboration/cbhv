#include <stdio.h>
#include <fstream.h>
#include <TF1.h>

void Karte(UInt_t start, UInt_t stop, Bool_t SaveTxt = true,
		   char *TxtFileName = "HV_gains_offsets.txt", Bool_t SavePs = true)
{
	FILE *InFile;
	char FileName[255];
	char line[1000];
	Float_t SetHV[1000];
	char SSetHV[1000];
	UInt_t level[1000];
	char Slevel[1000];
	Float_t chn[8][1000];
	char Schn0[1000], Schn1[1000], Schn2[1000], Schn3[1000], Schn4[1000],
	     Schn5[1000], Schn6[1000], Schn7[1000];
	TH1F *HistHV[1000][8];
	char HistName[1000];
	char HistTitle[1000];
	Float_t delta[8][1000];
	UInt_t nbins;
	TF1 *f[1000][8];
	Float_t xlow = 10000;
	Float_t xup = 0;
	Float_t binwidth;
	char fname[100];

	//Setting up the canvas
	TCanvas *hvcanv = new TCanvas("HV", "HV", 0, 0, 1200, 800);
	hvcanv->SetFillStyle(4000);
	hvcanv->Divide(3, 3, 0.001, 0.001);

	if (SaveTxt) {
		FILE *TxtFile = fopen(TxtFileName, "w");
		if (!TxtFile) {
			printf("Error opening output file\n");
			exit(1);
		}
		fprintf(TxtFile, "KarteNo,Kanal,Steig,Offset\n");
	}
	// Loop over all chosen files
	for (UInt_t i = start; i <= stop; i++) {

		if (SavePs) {
			char PsFileName[100];
			sprintf(PsFileName, "HV_gains_offsets%d.ps", i);
			TPostScript *ps = new TPostScript(PsFileName, 112);
		}
		// Set file name
		sprintf(FileName, "karte%03d.txt", i);

		// Open file
		InFile = fopen(FileName, "r");
		if (!InFile) {
			// File opening did not work
			printf("File '%s' not found.\n", FileName);
			continue;
		}
		nbins = 0;
		// File opening worked
		fscanf(InFile, "%s", line);	// Skip the first line, it is one big string
		fscanf(InFile, "%[^,],%*s %*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
				       SSetHV, Slevel, Schn0, Schn1, Schn2, Schn3, Schn4, Schn5, Schn6, Schn7);	// Read in normal line, use comma as delimiter (mostly)
		// Check whether read line was already at the end of file
		while (!feof(InFile)) {
			// Convert strings into numbers
			sscanf(SSetHV, "%f", &SetHV[nbins]);
			sscanf(Slevel, "%d", &level[nbins]);
			sscanf(Schn0, "%f", &chn[0][nbins]);
			sscanf(Schn1, "%f", &chn[1][nbins]);
			sscanf(Schn2, "%f", &chn[2][nbins]);
			sscanf(Schn3, "%f", &chn[3][nbins]);
			sscanf(Schn4, "%f", &chn[4][nbins]);
			sscanf(Schn5, "%f", &chn[5][nbins]);
			sscanf(Schn6, "%f", &chn[6][nbins]);
			sscanf(Schn7, "%f", &chn[7][nbins]);
			nbins++;	// Good line, increase counter
			fscanf(InFile, "%[^,],%*s %*[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%s",
					       SSetHV, Slevel, Schn0, Schn1, Schn2, Schn3, Schn4, Schn5, Schn6, Schn7);	// Read in normal line, use comma as delimiter (mostly)
		}
		// Finding xlow, xup boundries
		xlow = SetHV[0];
		xup = SetHV[0];
		for (int n = 1; n < nbins; n++) {
			if (SetHV[n] < xlow) {
				xlow = SetHV[n];
			}
			if (SetHV[n] > xup) {
				xup = SetHV[n];
			}
		}
		binwidth = 10;
		//Loop over all 8 channels from one board
		for (UInt_t j = 0; j < 8; j++) {
			// Highlight next pad
			hvcanv->cd(j + 1);
			sprintf(HistName, "Channel%d_Board%d", j, i);
			sprintf(HistTitle, "Board%d", i);
			HistHV[i][j] = new TH1F(HistName, HistTitle, nbins, xlow - 5, xup + 5);
			HistHV[i][j]->SetMarkerStyle(2);
			HistHV[i][j]->SetMarkerColor(2);
			//Loop for the number of data points for 1 channel
			for (UInt_t k = 0; k < nbins; k++) {
				delta[j][k] = chn[j][k] - SetHV[k];
				HistHV[i][j]->SetBinContent(k + 1, delta[j][k]);
			}

			sprintf(fname, "f%d_%d", i, j);
			//f[i][j] = new TF1(fname, "pol1" , xlow, xup);
			f[i][j] = new TF1(fname, "pol1", 1450, 1700);
			f[i][j]->SetLineWidth(1);
			gStyle->SetOptStat("n");
			HistHV[i][j]->Fit(f[i][j], "R0Q");
			HistHV[i][j]->Draw("P");
			f[i][j]->Draw("same");
			float par0 = f[i][j]->GetParameter(0);
			float par1 = f[i][j]->GetParameter(1);
			if (SaveTxt)
				fprintf(TxtFile, "%d,%d,%f,%f\r\n", i, j, par1, par0);
		}
		hvcanv->Update();
		if (SavePs) {
			ps->Close();
			printf("Saved histograms to %s\n", PsFileName);
		}
		fclose(InFile);
	}
	if (SaveTxt) {
		fclose(TxtFile);
		printf("Saved values to %s\n", TxtFileName);
	}
}

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TF1.h>
#include <TString.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TMath.h>
#include <TLine.h>

using namespace std;

void CBCalibrationGraphs(int ChanCalib=60);
void SetReadVoltages(vector<vector<int>>& ReadVolt);
void SetHoleList(Int_t HoleList[720]);

void CBCalibrationGraphs(int ChanCalib)
{
    // The voltage values set for source-data collection
    vector<int> SetVolt{1600, 1570, 1530, 1500, 1425, 1350};
    vector<vector<int>> ReadVolt;
    for(int i=0; i<(int)SetVolt.size(); i++){
        vector<int> volt(720,SetVolt.at(i));
        ReadVolt.push_back(volt);
    }
    // The change the voltage for channels with very deviating read voltage values (needs to be provided by hand)
    SetReadVoltages(ReadVolt);

    // The path and names of the parameter lists to be read
    vector<string> filenames;
    for(int i=0; i<(int)SetVolt.size(); i++){
        string filename = "Peakpositionenliste_HV";
        filename += to_string(SetVolt.at(i));
        filename += ".txt";
        filenames.push_back(filename);
    }

    // Create the 720 histograms
    TH1F *hists[720];
    for(int i=0; i<720; i++){
        TString histname = Form("CalibCurve%d",i);
        hists[i] = new TH1F(histname,histname,400, 1250, 1650);
        hists[i]->SetMarkerStyle(5); hists[i]->SetMarkerColor(1); hists[i]->SetMarkerSize(3);
        hists[i]->GetXaxis()->SetTitle("Voltage [V]");
        hists[i]->GetYaxis()->SetTitle("Peakposition in ADC-channels");
        hists[i]->GetYaxis()->SetTitleOffset(1);
    }

    // Read in the files and fill the histograms
    //-- Some storage containers used in the fits
    vector<int> peaksperchannel(720,0);
    vector<int> volt_lowbound(720,1750); vector<int> volt_highbound(720,1200);
    vector<double> peak_lowbound(720,0); vector<double> peak_highbound(720,0);
    //-- Loop over each set voltage
    for(int f=0; f<(int)filenames.size(); f++){
        fstream pfile(filenames.at(f).c_str());
        if(pfile.is_open()){
            cout<<"Opening file "<<filenames.at(f)<<endl;
            string line, crap;
            int channel; double gauspeak;
            while(getline(pfile,line,'\n')){
                //-- Extracting channel nr and peak position from each line, assuming there's 6 entries/line
                stringstream sst(line);
                sst >> channel >> gauspeak >> crap >> crap >> crap >> crap;
                //-- Fill the histogram only if this bin has already been filled
                int volt = ReadVolt.at(f).at(channel);
                int voltbin = hists[channel]->GetXaxis()->FindFixBin(volt);
                if(hists[channel]->GetBinContent(voltbin) > 0){
                    cout<<"HV"<<volt<<", ch"<<channel<<" is already set."<<endl;
                    continue;
                }
                hists[channel]->SetBinContent(voltbin,gauspeak);
                //-- Storing values for the fits
                peaksperchannel.at(channel) += 1;
                if(volt<volt_lowbound.at(channel)){volt_lowbound.at(channel)=volt; peak_lowbound.at(channel)=gauspeak;}
                if(volt>volt_highbound.at(channel)){volt_highbound.at(channel)=volt; peak_highbound.at(channel)=gauspeak;}
            }
            pfile.close();
        }
        else {
            cout<<"File "<<filenames.at(f)<<" not found"<<endl;
        }
    }

    // Extract the HV at the chosen ADC value
    vector<double> CalcVolt (720,1500);
    double averageCalcVolt = 0;
    int NrAverageCalcVolt = 0;
    vector<double> intercepts (720,0); vector<double> slopes (720,0);
    for( int ch=0; ch<720; ch++){
        //-- If there's more than two data points, a line is fitted
        if(peaksperchannel.at(ch) > 2){
            TF1 *fitline = new TF1("fitline","pol1",volt_lowbound.at(ch)-10,volt_highbound.at(ch)+10);
            //--Calculate starting values from first and last point
            double startslope = ((double)(peak_lowbound.at(ch)-peak_highbound.at(ch)))/((double)(volt_lowbound.at(ch)-volt_highbound.at(ch)));
            double startintercept = peak_lowbound.at(ch)-startslope*volt_lowbound.at(ch);
            fitline->SetParameters(startintercept,startslope);
            //-- Do the fit and calculate the needed voltage
            hists[ch]->Fit(fitline,"R0Q");
            double intercept = fitline->GetParameter(0);
            double slope = fitline->GetParameter(1);
            intercepts.at(ch) = intercept;
            slopes.at(ch) = slope;
            double calcHV = (ChanCalib - intercept)/slope;
            if(calcHV>1625){ CalcVolt.at(ch) = 1625; cout<<"Channel "<<ch<<" wanted "<< calcHV<<endl;}
            else if(calcHV<1275){ CalcVolt.at(ch) = 1275; cout<<"Channel "<<ch<<" wanted "<< calcHV<<endl;}
            else CalcVolt.at(ch) = calcHV;
            averageCalcVolt += CalcVolt.at(ch);
            NrAverageCalcVolt++;
            delete fitline;
        }
        //-- If there's only two data points, just calculate a line
        if(peaksperchannel.at(ch) == 2){
            double x1=0; double x2=0;
            double y1=0; double y2=0;
            for(int ihbin=1; ihbin<=(hists[ch]->GetNbinsX());ihbin++){
                if(hists[ch]->GetBinContent(ihbin)>0){
                    if(x1==0){
                        x1=hists[ch]->GetXaxis()->GetBinCenter(ihbin);
                        y1=hists[ch]->GetBinContent(ihbin);
                    }
                    else if(x2==0){
                        x2=hists[ch]->GetXaxis()->GetBinCenter(ihbin);
                        y2=hists[ch]->GetBinContent(ihbin);
                    }
                    else
                        cout<<"Channel "<<ch<<" have more than 2 fittable?"<<endl;
                }
            }
            double slope = (y1-y2)/(x1-x2);
            double intercept = ((y1-slope*x1)+(y2-slope*x2))/2.;
            intercepts.at(ch) = intercept;
            slopes.at(ch) = slope;
            double calcHV = (ChanCalib - intercept)/slope;
            if(calcHV>1625){ CalcVolt.at(ch) = 1625; cout<<"Channel "<<ch<<" wanted "<< calcHV<<endl;}
            else if(calcHV<1275){ CalcVolt.at(ch) = 1275; cout<<"Channel "<<ch<<" wanted "<< calcHV<<endl;}
            else CalcVolt.at(ch) = calcHV;
            averageCalcVolt += CalcVolt.at(ch);
            NrAverageCalcVolt++;
        }
    }
    // For every channel that had less than two peaks, use the average HV
    averageCalcVolt = averageCalcVolt/NrAverageCalcVolt;
    cout<<"Average HV from all "<<NrAverageCalcVolt<<" channels with 2 or more peaks = "<<averageCalcVolt<<endl;
    for(int ch=0; ch<720; ch++){
        if(peaksperchannel.at(ch) < 2)
            CalcVolt.at(ch) = averageCalcVolt;
    }

    // Print the calcualted voltage values to file
    Int_t HoleList[720];
    SetHoleList(HoleList);
    FILE *fileout = fopen("calibcurves_X.txt", "w");
    for(int ch=0; ch<720; ch++){
        if(HoleList[ch]==0){
            fprintf(fileout,"%i\t%i\n",ch,(int)CalcVolt.at(ch));
        }
    }
    fclose(fileout);

    // Print to screen the channel numbers who had less than two data points
    cout<<"The following channels had only one data point:"<<endl;
    for(int ch=0; ch<720; ch++){
        if(HoleList[ch]==0 && peaksperchannel.at(ch)==1){
            cout<<ch<<", ";
        }
    }
    cout<<endl;
    cout<<"The following non-hole channels had no data points:"<<endl;
    for(int ch=0; ch<720; ch++){
        if(HoleList[ch]==0 && peaksperchannel.at(ch)==0){
            cout<<ch<<", ";
        }
    }
    cout<<endl;

    // Draw the histograms and the corresponding lines, 16 channels per page
    gErrorIgnoreLevel = 1001;
    TCanvas *canv = new TCanvas("canv", "HV", 0, 0, 1200, 800);
    canv->Divide(4, 4, 0.001, 0.001);
    gStyle->SetCanvasColor(0);
    gStyle->SetOptStat(0);
    canv->SetLeftMargin(.1);
    canv->SetRightMargin(.05);
    canv->SetBottomMargin(.12);
    canv->SetTopMargin(.05);
    for(Int_t chBl=0; chBl<720; chBl+=16){
        for(Int_t chsub=0; chsub<16; chsub++){
            int ch=chBl+chsub;
            canv->cd(chsub+1);
            //-- Only plot for the channels with two or more datapoints
            if(peaksperchannel.at(ch) > 1){
                TF1 *plotline = new TF1("plotline","pol1",1275,1625);
                plotline->SetParameters(intercepts.at(ch),slopes.at(ch));
                double ymax = hists[ch]->GetMaximum();
                if(ymax < 200 ) hists[ch]->GetYaxis()->SetRangeUser(0,200);
                hists[ch]->Draw("P");
                plotline->SetLineWidth(2);
                plotline->DrawCopy("same");
                canv->Update();
                double ymin = gPad->GetUymin();
                TLine* lineHor = new TLine(1300,ChanCalib,CalcVolt.at(ch),ChanCalib);
                lineHor->SetLineColor(kGray+2); lineHor->SetLineWidth(2); lineHor->SetLineStyle(7);
                TLine* lineVer = new TLine(CalcVolt.at(ch),ymin,CalcVolt.at(ch),ChanCalib);
                lineVer->SetLineColor(kGray+2); lineVer->SetLineWidth(2); lineVer->SetLineStyle(7);
                lineHor->Draw("same"); lineVer->Draw("same");
            }
            else
                gPad->Clear();
        }
        if(chBl==0) canv->Print("CBHVCalibLines_X.pdf(","pdf");
        else if(chBl==704) canv->Print("CBHVCalibLines_X.pdf)","pdf");
        else canv->Print("CBHVCalibLines_X.pdf","pdf");
    }
}


void SetReadVoltages(vector<vector<int>>& ReadVolt)
{
    //TEMPLATE: ReadVolt.at(HVindex).at(channel)=Read HV value;
    //Ex: ReadVolt.at(0).at(59)=1591;

}

void SetHoleList(Int_t HoleList[])
{
  for(Int_t i=0; i<720; i++){
    HoleList[i]=0;
  }
  Int_t holes[48] = {26, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 40, 311, 315, 316, 318, 319, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366, 400, 401, 402, 405, 408, 679, 681, 682, 683, 684, 685, 686, 687, 688, 689, 691, 692};
  for(Int_t i=0; i<48; i++){
    HoleList[holes[i]]=1;
  }
}

// Includes ***Not completed yet***
#include "TpcFFT.h"
//

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/Fun4AllHistoManager.h>
#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>    // for PHIODataNode
#include <phool/PHNodeIterator.h>  // for PHNodeIterator
#include <phool/PHObject.h>        // for PHObject
#include <phool/getClass.h>

#include <Event/Event.h>
#include <Event/packet.h>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

#include <cassert>
#include <cstddef>
#include <memory>

#include <qautils/QAHistManagerDef.h>
#include <boost/format.hpp>

#include <iostream>
#include <string>
//

//____________________________________________________________________________..
TpcNoiseQA::TpcFFT(const std::string &name)
  : SubsysReco(name)
{
// reserves memory for max ADC samples
m_adcSamples.resize(1024, 0);
}
  
//____________________________________________________________________________..
int TpcFFT::InitRun(PHCompositeNode * /*unused*/)
{

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TpcFFT::process_event(PHCompositeNode *topNode)
{
  // Defines object from class Event which calls getClass function from
  // findNode class
  Event *_event = findNode::getClass<Event>(topNode, "PRDF");

  // Checks if event exists and returns error if not
  if (_event == nullptr)
    {
      std::cout << "TPCRawDataTree::Process_Event - Event not found" << std::endl;
      return -1;
    }

  // Checks if event is "special" and discards it if so
  if (_event->getEvtType() >= 8)  /// special events
    {
      return Fun4AllReturnCodes::DISCARDEVENT;
    }
//

  std::vector<Packet *> pktvec = _event->getPacketVector();

// Loop over packets in event
for (auto packet : pktvec)
  {

    int32_t packet_id = packet->getIdentifier();

    if (Verbosity())
      {
	std::cout << __PRETTY_FUNCTION__ << " : decoding packet " << packet_id << std::endl;
      }

    if (!packet)
    {
      if (Verbosity())
      {
        std::cout << __PRETTY_FUNCTION__ << " : missing packet " << packet_id << std::endl;
      }
      continue;
    }

    int ep = (packet_id-4000) % 10;
    sector = (packet_id - 4000 - ep)/10;

    // pull number of waveforms
    m_nWaveformInFrame = packet->iValue(0, "NR_WF");

    for (int wf = 0; wf < m_nWaveformInFrame; wf++)
      {
        sprintf(name,"h_WF_%s_%d",sec[sector].c_str(),ent_num);
	h_WF = new TH1F(name,name,360,0,360);
	
	sprintf(name,"h_FFT_%s_%d",sec[sector].c_str(),ent_num);
	h_FFT = new TH1F(name,name,360,0,20);
	
	m_FEE = packet->iValue(wf, "FEE");
	m_Channel = packet->iValue(wf, "CHANNEL");
	m_nSamples = packet->iValue(wf, "SAMPLES");

	// Checks if sample number and number of ADC values agrees
	//assert(m_nSamples < (int) m_adcSamples.size());
	if(m_nSamples > (int) m_adcSamples.size()) continue;
	

	dead = false;
	// Loop over samples in waveform
	for (int s = 0; s < m_nSamples; s++)
	  {
	    // Assign ADC value of sample s in waveform wf to adcSamples[s]
	    m_adcSamples[s] = packet->iValue(wf, s);
	
	    if(m_adcSamples[s] == 0 || TMath::IsNaN(float(m_adcSamples[s]))) {
	      dead=true;
	      break;
	    }
	  }

	if(dead) continue;

	pedestal.push_back(TMath::Median(360,m_adcSamples));

	for(int adc_sam_no=0;adc_sam_no<m_nSamples;adc_sam_no++){
	  if(m_adcSamples[adc_sam_no]<1024){
	    h_WF->Fill(adc_sam_no,m_adcSamples[adc_sam_no]);
	    if(m_adcSamples[adc_sam_no]>pedestal[wf]+50.0 || m_adcSamples[adc_sam_no]<pedestal[wf]-50.0) continue;
	    pedestal_sum += m_adcSamples[adc_sam_no];
	    pedestal_sigma_sum += pow(m_adcSamples[adc_sam_no],2);
	    pedestal_samples += 1.0;
	  }
	}

	pedestal_sigma.push_back(sqrt(pedestal_sigma_sum/pedestal_samples - pow(pedestal_sum/pedestal_samples,2)));

	h_FFT = h_WF->FFT(h_FFT,"MAG");
	Int_t FFT_entries = h_FFT->GetEntries();
	h_FFT->Scale(1.0/sqrt(FFT_entries));

	TH1 *WF_temp;
      	WF_temp=(TH1*)h_WF->Clone();
      	WF_temp->SetDirectory(0);
      	WF_clone[s].push_back(WF_temp);
      	TH1 *FFT_temp;
      	FFT_temp=(TH1*)h_FFT->Clone();
      	FFT_temp->SetDirectory(0);
      	FFT_clone[s].push_back(FFT_temp);
      	evt_num[s].push_back(ent_num);

	ent_num++;
      }
  }

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TpcFFT::End(PHCompositeNode * /*unused*/)
{

  sprintf(name, "/sphenix/user/llegnosky/TPCAnalysis/RootFiles/TpcFFT_%d.root", run_num);
  string outfile = name;
  TFile *TpcFFT = new TFile(outfile.c_str(),"RECREATE");

  for(int s=0;s<24;s++){
    sprintf(name,"Event_Numbers_sec%s",sec[s].c_str());
    TpcFFT->WriteObject(&evt_num[s],name);
    for(int evt=0;evt<FFT_clone[s].size();evt++){
      sprintf(name,"WF_%d_sec%s_evt%d",run_num,sec[s].c_str(),evt_num[s][evt]);
      WF_clone[s][evt]->Write(name);
      sprintf(name,"FFT_%d_sec%s_evt%d",run_num,sec[s].c_str(),evt_num[s][evt]);
      FFT_clone[s][evt]->Write(name);
      sprintf(name,"Ped_%d_sec%s_evt%d",run_num,sec[s].c_str(),evt_num[s][evt]);
      TpcFFT->WriteObject(&pedestal[s],name);
      sprintf(name,"Ped_RMS_%d_sec%s_evt%d",run_num,sec[s].c_str(),evt_num[s][evt]);
      TpcFFT->WriteObject(&pedestal_sigma[s],name);
    }
  }

  TpcFFT->Close();

  return Fun4AllReturnCodes::EVENT_OK;
}

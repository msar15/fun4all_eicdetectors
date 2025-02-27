#include "PHG4ForwardHcalSubsystem.h"

#include "PHG4ForwardHcalDetector.h"
#include "PHG4ForwardHcalDisplayAction.h"
#include "PHG4ForwardHcalSteppingAction.h"

#include <phparameter/PHParameters.h>

#include <g4main/PHG4DisplayAction.h>  // for PHG4DisplayAction
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4SteppingAction.h>  // for PHG4SteppingAction
#include <g4main/PHG4Subsystem.h>       // for PHG4Subsystem
#include <g4main/PHG4Utils.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>    // for PHIODataNode
#include <phool/PHNode.h>          // for PHNode
#include <phool/PHNodeIterator.h>  // for PHNodeIterator
#include <phool/PHObject.h>        // for PHObject
#include <phool/getClass.h>

#include <TSystem.h>

#include <set>  // for set
#include <sstream>

class PHG4Detector;

//_______________________________________________________________________
PHG4ForwardHcalSubsystem::PHG4ForwardHcalSubsystem(const std::string& name, const int lyr)
  : PHG4DetectorSubsystem(name, lyr)
{
  InitializeParameters();
}

//_______________________________________________________________________
PHG4ForwardHcalSubsystem::~PHG4ForwardHcalSubsystem()
{
  delete m_DisplayAction;
}

//_______________________________________________________________________
int PHG4ForwardHcalSubsystem::InitRunSubsystem(PHCompositeNode* topNode)
{
  PHNodeIterator iter(topNode);
  PHCompositeNode* dstNode = dynamic_cast<PHCompositeNode*>(iter.findFirst("PHCompositeNode", "DST"));

  // create display settings before detector
  m_DisplayAction = new PHG4ForwardHcalDisplayAction(Name());
  // create detector
  m_Detector = new PHG4ForwardHcalDetector(this, topNode, GetParams(), Name());
  m_Detector->SuperDetector(SuperDetector());
  m_Detector->OverlapCheck(CheckOverlap());
  m_Detector->Verbosity(Verbosity());

  std::set<std::string> nodes;
  if (GetParams()->get_int_param("active"))
  {
    PHNodeIterator dstIter(dstNode);
    PHCompositeNode* DetNode = dstNode;
    if (SuperDetector() != "NONE")
    {
      DetNode = dynamic_cast<PHCompositeNode*>(dstIter.findFirst("PHCompositeNode", SuperDetector()));
      if (!DetNode)
      {
        DetNode = new PHCompositeNode(SuperDetector());
        dstNode->addNode(DetNode);
      }
    }
    // create hit output nodes
    std::string detector_suffix = SuperDetector();
    if (detector_suffix == "NONE")
    {
      detector_suffix = Name();
    }
    m_HitNodeName = "G4HIT_" + detector_suffix;
    nodes.insert(m_HitNodeName);
    m_AbsorberNodeName = "G4HIT_ABSORBER_" + detector_suffix;
    if (GetParams()->get_int_param("absorberactive"))
    {
      nodes.insert(m_AbsorberNodeName);
    }
    m_SupportNodeName = "G4HIT_SUPPORT_" + detector_suffix;
    if (GetParams()->get_int_param("supportactive"))
    {
      nodes.insert(m_SupportNodeName);
    }
    for (auto thisnode : nodes)
    {
      PHG4HitContainer* g4_hits = findNode::getClass<PHG4HitContainer>(topNode, thisnode);
      if (!g4_hits)
      {
        g4_hits = new PHG4HitContainer(thisnode);
        DetNode->addNode(new PHIODataNode<PHObject>(g4_hits, thisnode, "PHObject"));
      }
    }
    // create stepping action
    PHG4ForwardHcalSteppingAction* tmp = new PHG4ForwardHcalSteppingAction(m_Detector, GetParams());
    tmp->SetHitNodeName(m_HitNodeName);
    tmp->SetAbsorberNodeName(m_AbsorberNodeName);
    tmp->SetSupportNodeName(m_SupportNodeName);
    m_SteppingAction = tmp;
  }

  return 0;
}

//_______________________________________________________________________
int PHG4ForwardHcalSubsystem::process_event(PHCompositeNode* topNode)
{
  // pass top node to stepping action so that it gets
  // relevant nodes needed internally
  if (m_SteppingAction)
  {
    m_SteppingAction->SetInterfacePointers(topNode);
  }
  return 0;
}

//_______________________________________________________________________
PHG4Detector* PHG4ForwardHcalSubsystem::GetDetector() const
{
  return m_Detector;
}

void PHG4ForwardHcalSubsystem::SetDefaultParameters()
{
  set_default_double_param("place_x", 0.);
  set_default_double_param("place_y", 0.);
  set_default_double_param("place_z", 400.);
  set_default_double_param("tower_dx", 10.);
  set_default_double_param("tower_dy", 10.);
  set_default_double_param("tower_dz", 100.);
  set_default_double_param("dz", 100.);
  set_default_double_param("rMin1", 5.);
  set_default_double_param("rMax1", 262.);
  set_default_double_param("rMin2", 5.);
  set_default_double_param("rMax2", 336.9);
  set_default_double_param("wls_dw", 0.3);
  set_default_double_param("support_dw", 0.2);
  set_default_double_param("rot_x", 0.);
  set_default_double_param("rot_y", 0.);
  set_default_double_param("rot_z", 0.);
  set_default_double_param("thickness_absorber", 2.);
  set_default_double_param("thickness_scintillator", 0.231);

  std::ostringstream mappingfilename;
  const char* calibroot = getenv("CALIBRATIONROOT");
  if (calibroot)
  {
    mappingfilename << calibroot;
  }
  else
  {
    std::cout << "no CALIBRATIONROOT environment variable" << std::endl;
    gSystem->Exit(1);
  }

  mappingfilename << "/ForwardHcal/mapping/towerMap_FHCAL_v005.txt";
  set_default_string_param("mapping_file", mappingfilename.str());
  set_default_string_param("mapping_file_md5", PHG4Utils::md5sum(mappingfilename.str()));
  set_default_string_param("scintillator", "G4_POLYSTYRENE");
  set_default_string_param("absorber", "G4_Fe");
  set_default_string_param("support", "G4_Fe");

  return;
}

void PHG4ForwardHcalSubsystem::SetTowerMappingFile(const std::string& filename)
{
  set_string_param("mapping_file", filename);
  set_string_param("mapping_file_md5", PHG4Utils::md5sum(get_string_param("mapping_file")));
}

void PHG4ForwardHcalSubsystem::Print(const std::string& what) const
{
  std::cout << Name() << " Parameters: " << std::endl;
  if (!BeginRunExecuted())
  {
    std::cout << "Need to execute BeginRun() before parameter printout is meaningful" << std::endl;
    std::cout << "To do so either run one or more events or on the command line execute: " << std::endl;
    std::cout << "Fun4AllServer *se = Fun4AllServer::instance();" << std::endl;
    std::cout << "PHG4Reco *g4 = (PHG4Reco *) se->getSubsysReco(\"PHG4RECO\");" << std::endl;
    std::cout << "g4->InitRun(se->topNode());" << std::endl;
    std::cout << "PHG4ForwardHcalSubsystem *fhcal = (PHG4ForwardHcalSubsystem *) g4->getSubsystem(\"" << Name() << "\");" << std::endl;
    std::cout << "fhcal->Print()" << std::endl;
    return;
  }
  GetParams()->Print();
  if (m_SteppingAction)
  {
    m_SteppingAction->Print(what);
  }
  return;
}

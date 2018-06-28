//#include "src/surf/surf_interface.hpp"
#include "simgrid-fmi.hpp"
#include "FMUCoSimulation_v1.h"
#include "FMUCoSimulation_v2.h"
#include "ModelManager.h"
#include <vector>
#include <unordered_map>
#include <fmiModelTypes.h>
#include <simgrid/simix.hpp>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_fmi, surf, "Logging specific to the SURF FMI plugin");


namespace simgrid{
namespace fmi{


std::unordered_map<std::string,FMUxxCS*> FMIPlugin::fmus;

/**
 * FMIPlugin
 *
 */
FMIPlugin::FMIPlugin(){
}

FMIPlugin::~FMIPlugin(){
}

void FMIPlugin::addFMUCS(std::string fmu_uri, std::string fmu_name, const double stepSize){

	const double startTime = SIMIX_get_clock();

	FMUxxCS *fmu = new FMUxxCS(fmu_uri, fmu_name, startTime, stepSize);
	fmus[fmu_name] = fmu;
	all_existing_models->push_back(fmu);
}


FMUxxCS* FMIPlugin::getFMU(std::string name){
	return fmus[name];
}

/**
 * FMUxxCS
 */

FMUxxCS::FMUxxCS(std::string fmu_uri, std::string fmu_name, const double startTime, const double stepSize)
: Model(simgrid::kernel::resource::Model::UpdateAlgo::LAZY){

	commStep = stepSize;
	nextEvent = -1;
	current_time = 0;

	FMUType fmuType = invalid;
	ModelManager::LoadFMUStatus loadStatus = ModelManager::loadFMU( fmu_name, fmu_uri, false, fmuType );

	if (( ModelManager::success != loadStatus ) && ( ModelManager::duplicate != loadStatus) ) {
		model = 0;
		// TODO : manage loading failure
		return;
	}

	if(fmi_1_0_cs == fmuType){
		XBT_DEBUG("instantiation of the FMU-CS v 1.0");
		model = new fmi_1_0::FMUCoSimulation( fmu_name, true, 1e-4  );
	}else if( (fmi_2_0_cs == fmuType) || (fmi_2_0_me_and_cs == fmuType) ){
		XBT_DEBUG("instantiation of the FMU-CS v 2.0");
		model = new fmi_2_0::FMUCoSimulation( fmu_name , true, 1e-4  );
	}

	model->instantiate(fmu_name, 0, fmiFalse, fmiFalse );

	XBT_DEBUG("FMU-CS instantiated");

	// TODO: manage initialization failure
	model->initialize( startTime, false, -1 );

	XBT_DEBUG("FMU-CS initialized");

}


FMUxxCS::~FMUxxCS() {
	delete model;
}

double FMUxxCS::getRealOutput(std::string name){
	return model->getRealValue(name);
}

bool FMUxxCS::getBooleanOutput(std::string name){
	return model->getBooleanValue(name);
}

int FMUxxCS::getIntegerOutput(std::string name){
	return model->getIntegerValue(name);
}

std::string FMUxxCS::getStringOutput(std::string name){
	return model->getStringValue(name);
}

void FMUxxCS::setRealInput(std::string name, double value, bool iterateAfter){
	simgrid::simix::simcall([this,name,value,iterateAfter]() {
		model->setValue(name,value);
		if(iterateAfter){
			model->doStep(SIMIX_get_clock(), 0., fmiTrue );
		}
		manageEventNotification();
	});
}

void FMUxxCS::setBooleanInput(std::string name, bool value,  bool iterateAfter){
	simgrid::simix::simcall([this,name,value,iterateAfter]() {
		model->setValue(name,value);
		if(iterateAfter){
			model->doStep(SIMIX_get_clock(), 0., fmiTrue );
		}
		manageEventNotification();
	});
}

void FMUxxCS::setIntegerInput(std::string name, int value,  bool iterateAfter){
	simgrid::simix::simcall([this,name,value,iterateAfter]() {
		model->setValue(name,value);
		if(iterateAfter){
			model->doStep(SIMIX_get_clock(), 0., fmiTrue );
		}
		manageEventNotification();
	});
}

void FMUxxCS::setStringInput(std::string name, std::string value,  bool iterateAfter){
	simgrid::simix::simcall([this,name,value,iterateAfter]() {
		model->setValue(name,value);
		if(iterateAfter){
			model->doStep(SIMIX_get_clock(), 0., fmiTrue );
		}
		manageEventNotification();
	});
}

void FMUxxCS::update_actions_state(double now, double delta){
	XBT_DEBUG("updating FMU at time = %f, delta = %f",now,delta);
	if(delta > 0){
		model->doStep(current_time, delta, fmiTrue );
		current_time = now;
	}
	manageEventNotification();

}

double FMUxxCS::next_occuring_event(double now){

	if(event_handlers.size()==0){
		return -1;
	}else{
		return commStep;
	}
}


void FMUxxCS::registerEvent(bool (*condition)(std::vector<std::string>),
		void (*handleEvent)(std::vector<std::string>),
		std::vector<std::string> handlerParam){

	simgrid::simix::simcall([this,condition,handleEvent,handlerParam]() {
		if(condition(handlerParam)){
			handleEvent(handlerParam);
		}else{
			event_handlers.push_back(handleEvent);
			event_conditions.push_back(condition);
			event_params.push_back(handlerParam);
		}
	});
}

void FMUxxCS::manageEventNotification(){
	int size = event_handlers.size();
	for(int i = 0;i<event_handlers.size();i++){

		bool isEvent = (*event_conditions[i])(event_params[i]);
		if(isEvent){

			(*event_handlers[i])(event_params[i]);

			event_handlers.erase(event_handlers.begin()+i);
			event_conditions.erase(event_conditions.begin()+i);
			event_params.erase(event_params.begin()+i);
			i--;
		}
	}
}

void FMUxxCS::deleteEvents(){
	event_handlers.clear();
	event_conditions.clear();
	event_params.clear();
}

}
}

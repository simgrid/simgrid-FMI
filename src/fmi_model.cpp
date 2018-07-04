//#include "src/surf/surf_interface.hpp"
#include "simgrid-fmi.hpp"
#include "FMUCoSimulation_v1.h"
#include "FMUCoSimulation_v2.h"
#include "ModelManager.h"
#include <vector>
#include <unordered_map>
#include <fmiModelTypes.h>
#include <simgrid/simix.hpp>
#include <FMIVariableType.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_fmi, surf, "Logging specific to the SURF FMI plugin");


namespace simgrid{
namespace fmi{


MasterFMI* FMIPlugin::master;


/**
 * FMIPlugin
 *
 */
FMIPlugin::FMIPlugin(){
}

FMIPlugin::~FMIPlugin(){
}

void FMIPlugin::addFMUCS(std::string fmu_uri, std::string fmu_name, bool iterateAfterInput){
	master->addFMUCS(fmu_uri, fmu_name, iterateAfterInput);
}

void FMIPlugin::connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port){
	master->connectFMU(out_fmu_name,output_port,in_fmu_name,input_port);
}

void FMIPlugin::connectRealFMUToSimgrid(double (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	master->connectRealFMUToSimgrid(generateInput,params,fmu_name,input_name);
}

void FMIPlugin::connectIntegerFMUToSimgrid(int (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	master->connectIntegerFMUToSimgrid(generateInput,params,fmu_name,input_name);
}

void FMIPlugin::connectBooleanFMUToSimgrid(bool (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	master->connectBooleanFMUToSimgrid(generateInput,params,fmu_name,input_name);
}

void FMIPlugin::connectStringFMUToSimgrid(std::string (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	master->connectStringFMUToSimgrid(generateInput,params,fmu_name,input_name);
}


void FMIPlugin::initFMIPlugin(double communication_step){
	if(master == 0){
		master = new MasterFMI(communication_step);
	}
}

void FMIPlugin::readyForSimulation(){
	master->initCouplings();
	all_existing_models->push_back(master);
}

double FMIPlugin::getRealOutput(std::string fmi_name, std::string output_name){
	return master->getRealOutput(fmi_name, output_name);
}

bool FMIPlugin::getBooleanOutput(std::string fmi_name, std::string output_name){
	return master->getBooleanOutput(fmi_name, output_name);
}

int FMIPlugin::getIntegerOutput(std::string fmi_name, std::string output_name){
	return master->getIntegerOutput(fmi_name, output_name);
}

std::string FMIPlugin::getStringOutput(std::string fmi_name, std::string output_name){
	return master->getStringOutput(fmi_name, output_name);
}

void FMIPlugin::setRealInput(std::string fmi_name, std::string input_name, double value){
	simgrid::simix::simcall([fmi_name, input_name,value]() {
		master->setRealInput(fmi_name, input_name, value, true);
	});
}

void FMIPlugin::setBooleanInput(std::string fmi_name, std::string input_name, bool value){
	simgrid::simix::simcall([fmi_name, input_name,value]() {
		master->setBooleanInput(fmi_name, input_name, value, true);
	});
}

void FMIPlugin::setIntegerInput(std::string fmi_name, std::string input_name, int value){
	simgrid::simix::simcall([fmi_name, input_name,value]() {
		master->setIntegerInput(fmi_name, input_name, value, true);
	});
}

void FMIPlugin::setStringInput(std::string fmi_name, std::string input_name, std::string value){
	simgrid::simix::simcall([fmi_name, input_name,value]() {
		master->setStringInput(fmi_name, input_name, value, true);
	});

}

void FMIPlugin::registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params){
	simgrid::simix::simcall([condition,handleEvent,params]() {
		master->registerEvent(condition,handleEvent,params);
	});
}

void FMIPlugin::deleteEvents(){
	master->deleteEvents();
}



/**
 * MasterFMI
 */

MasterFMI::MasterFMI(const double stepSize)
: Model(simgrid::kernel::resource::Model::UpdateAlgo::LAZY){

	commStep = stepSize;
	nextEvent = -1;
	current_time = 0;
	firstEvent = true;
}


MasterFMI::~MasterFMI() {
	//delete fmus;
}


void MasterFMI::addFMUCS(std::string fmu_uri, std::string fmu_name, bool iterateAfterInput){
	FMUType fmuType = invalid;
	ModelManager::LoadFMUStatus loadStatus = ModelManager::loadFMU( fmu_name, fmu_uri, false, fmuType );

	FMUCoSimulationBase *model;
	if (( ModelManager::success != loadStatus ) && ( ModelManager::duplicate != loadStatus) ) {
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

	const double startTime = SIMIX_get_clock();

	model->instantiate(fmu_name, 0, fmiFalse, fmiFalse );

	XBT_DEBUG("FMU-CS instantiated");

	// TODO: manage initialization failure
	model->initialize( startTime, false, -1 );

	XBT_DEBUG("FMU-CS initialized");

	fmus[fmu_name] = model;
	iterate_input[fmu_name] = iterateAfterInput;

}


void MasterFMI::connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port){

	//TODO: check if coupling is valid (i.e. FMU exists and variable type are the same
	fmu_connection connection;
	connection.out_fmu_name = out_fmu_name;
	connection.output_port = output_port;
	connection.in_fmu_name = in_fmu_name;
	connection.input_port = input_port;
	couplings.push_back(connection);
}

void MasterFMI::connectRealFMUToSimgrid(double (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	real_simgrid_fmu_connection connection;

	connection.in_fmu_name = fmu_name;
	connection.input_port = input_name;
	connection.generateInput = generateInput;
	connection.params = params;

	real_ext_couplings.push_back(connection);
}

void MasterFMI::connectIntegerFMUToSimgrid(int (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	integer_simgrid_fmu_connection connection;

	connection.in_fmu_name = fmu_name;
	connection.input_port = input_name;
	connection.generateInput = generateInput;
	connection.params = params;

	integer_ext_couplings.push_back(connection);
}

void MasterFMI::connectBooleanFMUToSimgrid(bool (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	boolean_simgrid_fmu_connection connection;

	connection.in_fmu_name = fmu_name;
	connection.input_port = input_name;
	connection.generateInput = generateInput;
	connection.params = params;

	boolean_ext_couplings.push_back(connection);
}

void MasterFMI::connectStringFMUToSimgrid(std::string (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){
	string_simgrid_fmu_connection connection;

	connection.in_fmu_name = fmu_name;
	connection.input_port = input_name;
	connection.generateInput = generateInput;
	connection.params = params;

	string_ext_couplings.push_back(connection);
}



double MasterFMI::getRealOutput(std::string fmi_name, std::string output_name){
	return fmus[fmi_name]->getRealValue(output_name);
}

bool MasterFMI::getBooleanOutput(std::string fmi_name, std::string output_name){
	return fmus[fmi_name]->getBooleanValue(output_name);
}

int MasterFMI::getIntegerOutput(std::string fmi_name, std::string output_name){
	return fmus[fmi_name]->getIntegerValue(output_name);
}

std::string MasterFMI::getStringOutput(std::string fmi_name, std::string output_name){
	return fmus[fmi_name]->getStringValue(output_name);
}

void MasterFMI::setRealInput(std::string fmi_name, std::string input_name, double value, bool simgrid_input){
	fmus[fmi_name]->setValue(input_name,value);
	if(iterate_input[fmi_name]){
		fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
	}

	if(simgrid_input){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setBooleanInput(std::string fmi_name, std::string input_name, bool value, bool simgrid_input){
	fmus[fmi_name]->setValue(input_name,value);
	if(iterate_input[fmi_name]){
		fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
	}

	if(simgrid_input){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setIntegerInput(std::string fmi_name, std::string input_name, int value, bool simgrid_input){
	fmus[fmi_name]->setValue(input_name,value);
	if(iterate_input[fmi_name]){
		fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
	}

	if(simgrid_input){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setStringInput(std::string fmi_name, std::string input_name, std::string value, bool simgrid_input){
	fmus[fmi_name]->setValue(input_name,value);
	if(iterate_input[fmi_name]){
		fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
	}

	if(simgrid_input){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::solveCouplings(bool firstIteration){

	bool change = true;
	while(change){
		change = false;
		for(fmu_connection coupling : couplings){
			change = (change || solveCoupling(coupling,!firstIteration));
		}
		if(firstIteration)
			firstIteration = false;
	}
}

bool MasterFMI::solveCoupling(fmu_connection coupling, bool checkChange){

	bool change = false;

	FMIVariableType type = fmus[coupling.out_fmu_name]->getType(coupling.output_port);
	std::string var_name = coupling.out_fmu_name+"_"+coupling.output_port;
	switch(type){
		case FMIVariableType::fmiTypeReal:
		{
			double r_out = getRealOutput(coupling.out_fmu_name, coupling.output_port);
			if( !checkChange || r_out !=  last_real_outputs[var_name]){
				setRealInput(coupling.in_fmu_name, coupling.input_port, r_out,false);
				last_real_outputs[var_name] = r_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeInteger:
		{
			int i_out = getIntegerOutput(coupling.out_fmu_name, coupling.output_port);
			if( !checkChange || i_out != last_int_outputs[var_name]){
				setIntegerInput(coupling.in_fmu_name, coupling.input_port, i_out,false);
				last_int_outputs[var_name] = i_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeBoolean:
		{
			bool b_out = getBooleanOutput(coupling.out_fmu_name, coupling.output_port);
			if( !checkChange || b_out != last_bool_outputs[var_name]){
				setBooleanInput(coupling.in_fmu_name, coupling.input_port, b_out,false);
				last_bool_outputs[var_name] = b_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeString:
		{
			std::string s_out = getStringOutput(coupling.out_fmu_name, coupling.output_port);
			if( !checkChange || s_out != last_string_outputs[var_name]){
				setStringInput(coupling.in_fmu_name, coupling.input_port, s_out,false);
				last_string_outputs[var_name] = s_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeUnknown:
			// TODO: manage this error !!!
			break;
	}

	return change;
}

void MasterFMI::solveExternalCoupling(){

	for(real_simgrid_fmu_connection coupling : real_ext_couplings){
		double input = coupling.generateInput(coupling.params);
		setRealInput(coupling.in_fmu_name, coupling.input_port, input,false);
	}

	for(integer_simgrid_fmu_connection coupling : integer_ext_couplings){
		int input = coupling.generateInput(coupling.params);
		setIntegerInput(coupling.in_fmu_name, coupling.input_port, input,false);
	}

	for(boolean_simgrid_fmu_connection coupling : boolean_ext_couplings){
		bool input = coupling.generateInput(coupling.params);
		setBooleanInput(coupling.in_fmu_name, coupling.input_port, input,false);
	}

	for(string_simgrid_fmu_connection coupling : string_ext_couplings){
		std::string input = coupling.generateInput(coupling.params);
		setStringInput(coupling.in_fmu_name, coupling.input_port, input,false);
	}
}


void MasterFMI::update_actions_state(double now, double delta){
	XBT_DEBUG("updating FMU at time = %f, delta = %f",now,delta);
	bool newTime = false;
	if(delta > 0){
		for(auto it : fmus){
			it.second->doStep(current_time, delta, fmiTrue );
		}
		current_time = now;
		newTime = true;
	}

	solveExternalCoupling();
	solveCouplings(newTime);
	manageEventNotification();
}

void MasterFMI::initCouplings(){
	solveExternalCoupling();
	solveCouplings(true);
	manageEventNotification();
}

double MasterFMI::next_occuring_event(double now){

	if(firstEvent){
		firstEvent = false;
		return 0;
	}else if(event_handlers.size()==0){
		return -1;
	}else{
		return commStep;
	}
}


void MasterFMI::registerEvent(bool (*condition)(std::vector<std::string>),
	void (*handleEvent)(std::vector<std::string>),
	std::vector<std::string> handlerParam){

	if(condition(handlerParam)){
		handleEvent(handlerParam);
	}else{
		event_handlers.push_back(handleEvent);
		event_conditions.push_back(condition);
		event_params.push_back(handlerParam);
	}
}

void MasterFMI::manageEventNotification(){
	int size = event_handlers.size();
	for(int i = 0;i<event_handlers.size();i++){

		bool isEvent = (*event_conditions[i])(event_params[i]);
		if(isEvent){

			event_conditions.erase(event_conditions.begin()+i);
			void (*handleEvent)(std::vector<std::string>) = event_handlers[i];
			event_handlers.erase(event_handlers.begin()+i);
			std::vector<std::string> handlerParam = event_params[i];
			event_params.erase(event_params.begin()+i);
			i--;

			(*handleEvent)(handlerParam);

		}
	}
}

void MasterFMI::deleteEvents(){
	event_handlers.clear();
	event_conditions.clear();
	event_params.clear();
}

}
}

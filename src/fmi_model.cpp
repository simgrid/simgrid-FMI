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
	return master->getRealOutput(fmi_name, output_name, true);
}

bool FMIPlugin::getBooleanOutput(std::string fmi_name, std::string output_name){
	return master->getBooleanOutput(fmi_name, output_name, true);
}

int FMIPlugin::getIntegerOutput(std::string fmi_name, std::string output_name){
	return master->getIntegerOutput(fmi_name, output_name, true);
}

std::string FMIPlugin::getStringOutput(std::string fmi_name, std::string output_name){
	return master->getStringOutput(fmi_name, output_name, true);
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

void FMIPlugin::configureOutputLog(std::string output_file_path, std::vector<port> ports_to_monitor){
	master->configureOutputLog(output_file_path,ports_to_monitor);
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
	ready_for_simulation = false;
}


MasterFMI::~MasterFMI() {
	output.close();
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

	checkNotReadyForSimulation();
	checkPortValidity(out_fmu_name,output_port,FMIVariableType::fmiTypeUnknown,false);
	checkPortValidity(in_fmu_name,input_port,fmus[out_fmu_name]->getType(output_port),true);

	port out;
	port in;
	out.fmu = out_fmu_name;
	out.name = output_port;
	in.fmu = in_fmu_name;
	in.name = input_port;
	in_coupled_input.push_back(in);
	couplings[in]=out;
}

void MasterFMI::connectRealFMUToSimgrid(double (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){

	checkNotReadyForSimulation();
	checkPortValidity(fmu_name,input_name,FMIVariableType::fmiTypeReal,true);

	real_simgrid_fmu_connection connection;
	port in;
	in.fmu = fmu_name;
	in.name = input_name;
	connection.in = in;
	connection.generateInput = generateInput;
	connection.params = params;

	real_ext_couplings.push_back(connection);
	ext_coupled_input.push_back(in);
}

void MasterFMI::connectIntegerFMUToSimgrid(int (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){

	checkNotReadyForSimulation();
	checkPortValidity(fmu_name,input_name,FMIVariableType::fmiTypeInteger,true);

	integer_simgrid_fmu_connection connection;
	port in;
	in.fmu = fmu_name;
	in.name = input_name;
	connection.in = in;
	connection.generateInput = generateInput;
	connection.params = params;

	integer_ext_couplings.push_back(connection);
	ext_coupled_input.push_back(in);
}

void MasterFMI::connectBooleanFMUToSimgrid(bool (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){

	checkNotReadyForSimulation();
	checkPortValidity(fmu_name,input_name,FMIVariableType::fmiTypeBoolean,true);

	boolean_simgrid_fmu_connection connection;
	port in;
	in.fmu = fmu_name;
	in.name = input_name;
	connection.in = in;
	connection.generateInput = generateInput;
	connection.params = params;

	boolean_ext_couplings.push_back(connection);
	ext_coupled_input.push_back(in);
}

void MasterFMI::connectStringFMUToSimgrid(std::string (*generateInput)(std::vector<std::string>), std::vector<std::string> params, std::string fmu_name, std::string input_name){

	checkNotReadyForSimulation();
	checkPortValidity(fmu_name,input_name,FMIVariableType::fmiTypeString,true);

	string_simgrid_fmu_connection connection;
	port in;
	in.fmu = fmu_name;
	in.name = input_name;
	connection.in = in;
	connection.generateInput = generateInput;
	connection.params = params;

	string_ext_couplings.push_back(connection);
	ext_coupled_input.push_back(in);
}



double MasterFMI::getRealOutput(std::string fmi_name, std::string output_name, bool checkPort){

	if(checkPort)
		checkPortValidity(fmi_name,output_name,FMIVariableType::fmiTypeReal,false);

	double out;
	fmiStatus status = fmus[fmi_name]->getValue(output_name,out);
	if(status != fmiOK)
		xbt_die("FMI %s failed to return the value of variable %s",fmi_name.c_str(),output_name.c_str());

	return out;
}

bool MasterFMI::getBooleanOutput(std::string fmi_name, std::string output_name, bool checkPort){

	if(checkPort)
		checkPortValidity(fmi_name,output_name,FMIVariableType::fmiTypeBoolean,false);

	fmi2Boolean out;
	fmiStatus status = fmus[fmi_name]->getValue(output_name,out);
	if(status != fmiOK)
		xbt_die("FMI %s failed to return the value of variable %s",fmi_name.c_str(),output_name.c_str());

	return out;
}

int MasterFMI::getIntegerOutput(std::string fmi_name, std::string output_name, bool checkPort){

	if(checkPort)
		checkPortValidity(fmi_name,output_name,FMIVariableType::fmiTypeInteger,false);

	int out;
	fmiStatus status = fmus[fmi_name]->getValue(output_name,out);
	if(status != fmiOK)
		xbt_die("FMI %s failed to return the value of variable %s",fmi_name.c_str(),output_name.c_str());

	return out;
}

std::string MasterFMI::getStringOutput(std::string fmi_name, std::string output_name, bool checkPort){

	if(checkPort)
		checkPortValidity(fmi_name,output_name,FMIVariableType::fmiTypeString,false);

	std::string out;
	fmiStatus status = fmus[fmi_name]->getValue(output_name,out);
	if(status != fmiOK)
		xbt_die("FMI %s failed to return the value of variable %s",fmi_name.c_str(),output_name.c_str());

	return out;
}

void MasterFMI::setRealInput(std::string fmi_name, std::string input_name, double value, bool simgrid_input){

	if(simgrid_input){
		checkPortValidity(fmi_name,input_name,FMIVariableType::fmiTypeReal,simgrid_input);
	}

	fmiStatus status = fmus[fmi_name]->setValue(input_name,value);
	if(status != fmiOK)
		xbt_die("FMU %s failed to set its port %s to value %f",fmi_name.c_str(),input_name.c_str(),value);

	if(iterate_input[fmi_name]){
		status = fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
		if(status != fmiOK)
			xbt_die("FMU %s failed to perform a doStep(dt=0) after setting an input (you should may be set iterateAfterInput=false when adding the FMU CS).",fmi_name.c_str());
	}

	if(simgrid_input && ready_for_simulation){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setBooleanInput(std::string fmi_name, std::string input_name, bool value, bool simgrid_input){

	if(simgrid_input){
		checkPortValidity(fmi_name,input_name,FMIVariableType::fmiTypeBoolean,simgrid_input);
	}

	fmiStatus status = fmus[fmi_name]->setValue(input_name,value);
	if(status != fmiOK)
		xbt_die("FMU %s failed to set its port %s to value %i",fmi_name.c_str(),input_name.c_str(),value);

	if(iterate_input[fmi_name]){
		status = fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
		if(status != fmiOK)
			xbt_die("FMU %s failed to perform a doStep(dt=0) after setting an input (you should may be set iterateAfterInput=false when adding the FMU CS).",fmi_name.c_str());
	}

	if(simgrid_input && ready_for_simulation){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setIntegerInput(std::string fmi_name, std::string input_name, int value, bool simgrid_input){

	if(simgrid_input){
		checkPortValidity(fmi_name,input_name,FMIVariableType::fmiTypeInteger,simgrid_input);
	}

	fmiStatus status = fmus[fmi_name]->setValue(input_name,value);
	if(status != fmiOK)
		xbt_die("FMU %s failed to set its port %s to value %i",fmi_name.c_str(),input_name.c_str(),value);

	if(iterate_input[fmi_name]){
		status = fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
		if(status != fmiOK)
			xbt_die("FMU %s failed to perform a doStep(dt=0) after setting an input (you should may be set iterateAfterInput=false when adding the FMU CS).",fmi_name.c_str());
	}

	if(simgrid_input && ready_for_simulation){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::setStringInput(std::string fmi_name, std::string input_name, std::string value, bool simgrid_input){

	if(simgrid_input){
		checkPortValidity(fmi_name,input_name, FMIVariableType::fmiTypeString, simgrid_input);
	}

	fmiStatus status = fmus[fmi_name]->setValue(input_name,value);
	if(status != fmiOK)
		xbt_die("FMU %s failed to set its port %s to value %s",fmi_name.c_str(),input_name.c_str(),value.c_str());

	if(iterate_input[fmi_name]){
		status = fmus[fmi_name]->doStep(SIMIX_get_clock(), 0., fmiTrue );
		if(status != fmiOK)
			xbt_die("FMU %s failed to perform a doStep(dt=0) after setting an input (you should may be set iterateAfterInput=false when adding the FMU CS).",fmi_name.c_str());
	}

	if(simgrid_input && ready_for_simulation){
		solveCouplings(false);
		manageEventNotification();
	}
}

void MasterFMI::solveCouplings(bool firstIteration){

	bool change = true;
	int i = 0;
	while(change){
		change = false;
		for(port in : in_coupled_input){
			change = (solveCoupling(in, couplings[in],!firstIteration) || change);
		}
		if(firstIteration)
			firstIteration = false;
		i++;
	}

	logOutput();
}

bool MasterFMI::solveCoupling(port in, port out, bool checkChange){

	bool change = false;

	FMIVariableType type = fmus[out.fmu]->getType(out.name);
	switch(type){
		case FMIVariableType::fmiTypeReal:
		{
			double r_out = getRealOutput(out.fmu, out.name);
			if( !checkChange || r_out !=  last_real_outputs[out]){
				setRealInput(in.fmu, in.name, r_out,false);
				last_real_outputs[out] = r_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeInteger:
		{
			int i_out = getIntegerOutput(out.fmu, out.name);
			if( !checkChange || i_out != last_int_outputs[out]){
				setIntegerInput(in.fmu, in.name, i_out,false);
				last_int_outputs[out] = i_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeBoolean:
		{
			bool b_out = getBooleanOutput(out.fmu, out.name);
			if( !checkChange || b_out != last_bool_outputs[out]){
				setBooleanInput(in.fmu, in.name, b_out,false);
				last_bool_outputs[out] = b_out;
				change = true;
			}
			break;
		}
		case FMIVariableType::fmiTypeString:
		{
			std::string s_out = getStringOutput(out.fmu, out.name);
			if( !checkChange || s_out != last_string_outputs[out]){
				setStringInput(in.fmu, in.name, s_out,false);
				last_string_outputs[out] = s_out;
				change = true;
			}
			break;
		}
	}

	return change;
}

void MasterFMI::solveExternalCoupling(){

	for(real_simgrid_fmu_connection coupling : real_ext_couplings){
		double input = coupling.generateInput(coupling.params);
		setRealInput(coupling.in.fmu, coupling.in.name, input,false);
	}

	for(integer_simgrid_fmu_connection coupling : integer_ext_couplings){
		int input = coupling.generateInput(coupling.params);
		setIntegerInput(coupling.in.fmu, coupling.in.name, input,false);
	}

	for(boolean_simgrid_fmu_connection coupling : boolean_ext_couplings){
		bool input = coupling.generateInput(coupling.params);
		setBooleanInput(coupling.in.fmu, coupling.in.name, input,false);
	}

	for(string_simgrid_fmu_connection coupling : string_ext_couplings){
		std::string input = coupling.generateInput(coupling.params);
		setStringInput(coupling.in.fmu, coupling.in.name, input,false);
	}
}


void MasterFMI::update_actions_state(double now, double delta){

	XBT_DEBUG("updating the FMUs at time = %f, delta = %f",now,delta);

	while(current_time < now){
		double dt = std::min(commStep, now - current_time);
		XBT_DEBUG("current_time = %f perform doStep of %f ",current_time, dt);
		for(auto it : fmus){
			fmiStatus status = it.second->doStep(current_time, dt, fmiTrue );
			if(status != fmiOK)
				xbt_die("FMU %s failed to go from time %f to time %f during the co-simulation",it.first.c_str(),current_time,(current_time+dt));
		}
		current_time += dt;
		if(current_time != now){
			solveCouplings(true);
		}
	}

	solveExternalCoupling();
	solveCouplings(true);
	manageEventNotification();

}

void MasterFMI::initCouplings(){
	ready_for_simulation = true;
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

bool MasterFMI::isInputCoupled(std::string fmu, std::string input_name){
	port input;
	input.fmu = fmu;
	input.name = input_name;
	return std::find(in_coupled_input.begin(), in_coupled_input.end(), input) != in_coupled_input.end()
			|| std::find(ext_coupled_input.begin(), ext_coupled_input.end(), input) != ext_coupled_input.end();
}

void MasterFMI::checkPortValidity(std::string fmu_name, std::string port_name, FMIVariableType type, bool check_already_coupled){

	if(fmus.find(fmu_name)==fmus.end())
		xbt_die("unknown FMU %s",fmu_name.c_str());

	FMIVariableType var_type = fmus[fmu_name]->getType(port_name);

	if(var_type == FMIVariableType::fmiTypeUnknown)
		xbt_die("unknown variable %s of FMU %s",port_name.c_str(),fmu_name.c_str());

	if(type != FMIVariableType::fmiTypeUnknown && var_type != type)
		xbt_die("wrong type compatibility for port %s of FMU %s.",port_name.c_str(),fmu_name.c_str());

	if(check_already_coupled && isInputCoupled(fmu_name,port_name))
		xbt_die("port %s of FMU %s is already coupled to a model",port_name.c_str(),fmu_name.c_str());
}

void MasterFMI::checkNotReadyForSimulation(){
	if(ready_for_simulation)
		xbt_die("you can not modify the FMI model after calling simgrid::fmi::FMIPlugin::readyForSimulation().");
}


void MasterFMI::configureOutputLog(std::string output_file_path, std::vector<port> ports_to_monitor){
	output.open(output_file_path, std::ios::out);
	monitored_ports = ports_to_monitor;
	for(port p : monitored_ports){
		checkPortValidity(p.fmu,p.name, FMIVariableType::fmiTypeUnknown,false);
	}
}

void MasterFMI::logOutput(){
	output << current_time;

	for(port p : monitored_ports){
		switch(fmus[p.fmu]->getType(p.name)){
			case FMIVariableType::fmiTypeReal:
				output << ";" << getRealOutput(p.fmu, p.name);
				break;
			case FMIVariableType::fmiTypeInteger:
				output << ";" << getIntegerOutput(p.fmu, p.name);
				break;
			case FMIVariableType::fmiTypeBoolean:
				output << ";" << getBooleanOutput(p.fmu, p.name);
				break;
			case FMIVariableType::fmiTypeString:
				output << ";" << getStringOutput(p.fmu, p.name);
				break;
		}
	}

	output << "\n";
}


}
}

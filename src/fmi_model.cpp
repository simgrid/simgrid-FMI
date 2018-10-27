//#include "src/surf/surf_interface.hpp"
#include "simgrid-fmi.hpp"
#include <VariableStepSizeFMU.h>
#include <IncrementalFMU.h>
#include <IntegratorType.h>
#include <vector>
#include <unordered_map>
#include <fmiModelTypes.h>
#include <simgrid/simix.hpp>


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_fmi, surf, "Logging specific to the SURF FMI plugin");


namespace simgrid{
namespace fmi{


std::unordered_map<std::string,FMUModel*> FMIPlugin::fmus;

/**
 * FMIPlugin
 *
 */
FMIPlugin::FMIPlugin(){
}

FMIPlugin::~FMIPlugin(){
}

void FMIPlugin::addFMUCS(std::string fmu_uri, std::string fmu_name, const double stepSize,
		std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
		std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
		std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
		std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
		std::vector<std::string> *realOutputNames,
		std::vector<std::string> *intOutputNames,
		std::vector<std::string> *boolOutputNames,
		std::vector<std::string> *stringOutputNames){

	const double startTime = SIMIX_get_clock();

	FMUxxCS *fmu = new FMUxxCS(fmu_uri, fmu_name, fmu_name, startTime, stepSize,
			initRealInputNames, initRealInputVals, initIntInputNames, initIntInputVals,
			initBoolInputNames, initBoolInputVals, initStringInputNames, initStringInputVals,
			realOutputNames, intOutputNames, boolOutputNames, stringOutputNames);

	fmus[fmu_name] = fmu;
	all_existing_models.push_back(fmu);
}


void FMIPlugin::addFMUME(std::string fmu_uri, std::string fmu_name,
		const double integratorStepSize, const double eventCheckStepSize, const double lookahead, IntegratorType integrator,
		std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
		std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
		std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
		std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
		std::vector<std::string> *realOutputNames,
		std::vector<std::string> *intOutputNames,
		std::vector<std::string> *boolOutputNames,
		std::vector<std::string> *stringOutputNames){

	const double startTime = SIMIX_get_clock();

	FMUxxME *fmu = new FMUxxME(fmu_uri, fmu_name, fmu_name, startTime,
				integratorStepSize, eventCheckStepSize, lookahead, integrator,
				initRealInputNames, initRealInputVals, initIntInputNames, initIntInputVals,
				initBoolInputNames, initBoolInputVals, initStringInputNames, initStringInputVals,
				realOutputNames, intOutputNames, boolOutputNames, stringOutputNames);

	fmus[fmu_name] = fmu;
	all_existing_models.push_back(fmu);
}


FMUModel* FMIPlugin::getFMU(std::string name){
	return fmus[name];
}


/**
 * FMUModel
 */

FMUModel::FMUModel(std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
		std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
		std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
		std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
		std::vector<std::string> *realOutputNames,
		std::vector<std::string> *intOutputNames,
		std::vector<std::string> *boolOutputNames,
		std::vector<std::string> *stringOutputNames) : Model(simgrid::kernel::resource::Model::UpdateAlgo::LAZY)
{

	if(realOutputNames){
		for(int i = 0; i<realOutputNames->size();i++){
			real_output_names[(*realOutputNames)[i]] = i;
		}
	}

	if(initRealInputNames && initRealInputVals){
		for(int i = 0; i<initRealInputNames->size();i++){
			real_input_names[(*initRealInputNames)[i]] = i;
			real_input_values.push_back((*initRealInputVals)[i]);
		}
	}

	if(stringOutputNames){
		for(int i = 0; i<stringOutputNames->size();i++){
			string_output_names[(*stringOutputNames)[i]] = i;
		}
	}

	if(initStringInputNames && initStringInputVals){
		for(int i = 0; i<initStringInputNames->size();i++){
			string_input_names[(*initStringInputNames)[i]] = i;
			string_input_values.push_back((*initStringInputVals)[i]);
		}
	}

	if(intOutputNames){
		for(int i = 0; i<intOutputNames->size();i++){
			int_output_names[(*intOutputNames)[i]] = i;
		}
	}

	if(initIntInputNames && initIntInputNames){
		for(int i = 0; i<initIntInputNames->size();i++){
			int_input_names[(*initIntInputNames)[i]] = i;
			int_input_values.push_back((*initIntInputVals)[i]);
		}
	}

	if(boolOutputNames){
		for(int i = 0; i<boolOutputNames->size();i++){
			bool_output_names[(*boolOutputNames)[i]] = i;
		}
	}

	if(initBoolInputNames && initBoolInputVals){
		for(int i = 0; i<initBoolInputNames->size();i++){
			bool_input_names[(*initBoolInputNames)[i]] = i;
			if((*initBoolInputVals)[i]){
				bool_input_values.push_back(fmiTrue);
			}else{
				bool_input_values.push_back(fmiFalse);
			}
		}
	}
}

FMUModel::~FMUModel() {

}

double FMUModel::next_occuring_event(double now){
	return -1;
}

void FMUModel::update_actions_state(double now, double delta){
}


void FMUModel::setRealInput(std::string name, double value, bool iterateAfter, bool sendNow){
	real_input_values[real_input_names[name]] = value;
}

void FMUModel::setBoolInput(std::string name, bool value, bool iterateAfter, bool sendNow){

	if(value){
		bool_input_values[bool_input_names[name]] = fmiTrue;
	}else{
		bool_input_values[bool_input_names[name]] = fmiFalse;
	}
}

void FMUModel::setIntegerInput(std::string name, int value, bool iterateAfter, bool sendNow){
	int_input_values[int_input_names[name]] = value;
}

void FMUModel::setStringInput(std::string name, std::string value, bool iterateAfter, bool sendNow){
	string_input_values[string_input_names[name]] = value;
}

/**
 * FMUxxCS
 */

FMUxxCS::FMUxxCS(std::string fmu_uri, std::string fmu_name, std::string fmu_instance_name, const double startTime, 	const double stepSize,
				std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
				std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
				std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
				std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
				std::vector<std::string> *realOutputNames,
				std::vector<std::string> *intOutputNames,
				std::vector<std::string> *boolOutputNames,
				std::vector<std::string> *stringOutputNames) : FMUModel(initRealInputNames, initRealInputVals, initIntInputNames, initIntInputVals,
			initBoolInputNames, initBoolInputVals, initStringInputNames, initStringInputVals,
			realOutputNames, intOutputNames, boolOutputNames, stringOutputNames) {

	commStep = stepSize;
	nextEvent = -1;
	current_time = 0;

	model = new VariableStepSizeFMU( fmu_uri , fmu_name );

	XBT_DEBUG("FMU-CS instantiated");

	std::string *real_in_names_ptr = nullptr;
	std::string *int_in_names_ptr = nullptr;
	std::string *bool_in_names_ptr = nullptr;
	std::string *string_in_names_ptr = nullptr;

	double *real_in_val_ptr = nullptr;
	int *int_in_val_ptr = nullptr;
	char *bool_in_val_ptr = nullptr;
	std::string *string_in_val_ptr = nullptr;

	int real_in_size = 0;
	int int_in_size = 0;
	int bool_in_size = 0;
	int string_in_size = 0;


	if(realOutputNames)
		model->defineRealOutputs( realOutputNames->data(),  realOutputNames->size() );

	if(initRealInputNames){
		model->defineRealInputs( initRealInputNames->data(), initRealInputNames->size() );
		real_in_names_ptr = initRealInputNames->data();
		real_in_val_ptr = initRealInputVals->data();
		real_in_size = initRealInputNames->size();
	}

	XBT_DEBUG("real variables initialized");

	if(stringOutputNames)
		model->defineStringOutputs( stringOutputNames->data(),  stringOutputNames->size() );

	if(initStringInputNames){
		model->defineStringInputs( initStringInputNames->data(), initStringInputNames->size() );
		string_in_names_ptr = initStringInputNames->data();
		string_in_val_ptr = initStringInputVals->data();
		string_in_size = initStringInputNames->size();
	}

	XBT_DEBUG("string variables initialized");

	if(intOutputNames)
		model->defineIntegerOutputs( intOutputNames->data(), intOutputNames->size());

	if(initIntInputNames){
		model->defineIntegerInputs( initIntInputNames->data(), initIntInputNames->size());
		int_in_names_ptr = initIntInputNames->data();
		int_in_val_ptr = initIntInputVals->data();
		int_in_size = initIntInputNames->size();
	}

	XBT_DEBUG("int variables initialized");

	if(boolOutputNames)
		model->defineBooleanOutputs( boolOutputNames->data(), boolOutputNames->size() );

	if(initBoolInputNames){
		model->defineBooleanInputs( initBoolInputNames->data(), initBoolInputNames->size() );
		bool_in_names_ptr = initBoolInputNames->data();
		bool_in_val_ptr = bool_input_values.data();
		bool_in_size = initBoolInputNames->size();
	}

	XBT_DEBUG("bool variables initialized");

	int status = model->init( fmu_instance_name ,
						real_in_names_ptr, real_in_val_ptr, real_in_size,
						int_in_names_ptr, int_in_val_ptr, int_in_size,
						bool_in_names_ptr, bool_in_val_ptr, bool_in_size,
						string_in_names_ptr, string_in_val_ptr, string_in_size,
						startTime, stepSize );

}


FMUxxCS::~FMUxxCS() {
	delete model;
}

double FMUxxCS::getRealOutputs(std::string name){
	return model->getRealOutputs()[real_output_names[name]];
}

bool FMUxxCS::getBoolOutputs(std::string name){

	return model->getBooleanOutputs()[string_output_names[name]];
}

int FMUxxCS::getIntegerOutputs(std::string name){
	return model->getIntegerOutputs()[int_output_names[name]];
}

std::string FMUxxCS::getStringOutputs(std::string name){

	return model->getStringOutputs()[string_output_names[name]];
}

void FMUxxCS::setRealInput(std::string name, double value, bool iterateAfter = true, bool sendNow = true){
	simgrid::simix::simcall([this,name,value,iterateAfter,sendNow]() {
		FMUModel::setRealInput(name,value,iterateAfter,sendNow);
		if(sendNow){
			model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data(), iterateAfter);
			manageEventNotification();
		}
	});
}

void FMUxxCS::setBoolInput(std::string name, bool value,  bool iterateAfter = true, bool sendNow = true){
	simgrid::simix::simcall([this,name,value,iterateAfter,sendNow]() {
		FMUModel::setBoolInput(name,value,iterateAfter,sendNow);
		if(sendNow){
			model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data(), iterateAfter);
			manageEventNotification();
		}
	});
}

void FMUxxCS::setIntegerInput(std::string name, int value,  bool iterateAfter = true, bool sendNow = true){
	simgrid::simix::simcall([this,name,value,iterateAfter,sendNow]() {
		FMUModel::setIntegerInput(name,value,iterateAfter,sendNow);
		if(sendNow){
			model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data(), iterateAfter);
			manageEventNotification();
		}
	});
}

void FMUxxCS::setStringInput(std::string name, std::string value,  bool iterateAfter = true, bool sendNow = true){
	simgrid::simix::simcall([this,name,value,iterateAfter,sendNow]() {
		FMUModel::setStringInput(name,value,iterateAfter,sendNow);
		if(sendNow){
			model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data(), iterateAfter);
			manageEventNotification();
		}
	});
}

void FMUxxCS::update_actions_state(double now, double delta){
	XBT_DEBUG("updating FMU at time = %f, delta = %f",now,delta);
	if(delta > 0){
		model->sync( current_time , now );
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


/**
 * FMU-ME
 */
FMUxxME::FMUxxME(std::string fmu_uri, std::string fmu_name, std::string fmu_instance_name, const double startTime,
				const double integratorStepSize, const double eventCheckStepSize, const double lookahead, IntegratorType integrator,
				std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
				std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
				std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
				std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
				std::vector<std::string> *realOutputNames,
				std::vector<std::string> *intOutputNames,
				std::vector<std::string> *boolOutputNames,
				std::vector<std::string> *stringOutputNames) : FMUModel(initRealInputNames, initRealInputVals, initIntInputNames, initIntInputVals,
			initBoolInputNames, initBoolInputVals, initStringInputNames, initStringInputVals,
			realOutputNames, intOutputNames, boolOutputNames, stringOutputNames) {

	commStep = lookahead;
	nextEvent = -1;
	current_time = startTime;

	model = new IncrementalFMU( fmu_uri , fmu_name, true, 1e-4, integrator );

	XBT_DEBUG("FMU-ME instantiated");

	std::string *real_in_names_ptr = nullptr;
	std::string *int_in_names_ptr = nullptr;
	std::string *bool_in_names_ptr = nullptr;
	std::string *string_in_names_ptr = nullptr;

	double *real_in_val_ptr = nullptr;
	int *int_in_val_ptr = nullptr;
	char *bool_in_val_ptr = nullptr;
	std::string *string_in_val_ptr = nullptr;

	int real_in_size = 0;
	int int_in_size = 0;
	int bool_in_size = 0;
	int string_in_size = 0;

	if(realOutputNames)
		model->defineRealOutputs( realOutputNames->data(),  realOutputNames->size() );

	XBT_DEBUG("real output variables initialized");

	if(initRealInputNames){
		model->defineRealInputs( initRealInputNames->data(), initRealInputNames->size() );
		real_in_names_ptr = initRealInputNames->data();
		real_in_val_ptr = initRealInputVals->data();
		real_in_size = initRealInputNames->size();
	}

	XBT_DEBUG("real variables initialized");

	if(stringOutputNames)
		model->defineStringOutputs( stringOutputNames->data(),  stringOutputNames->size() );

	if(initStringInputNames){
		model->defineStringInputs( initStringInputNames->data(), initStringInputNames->size() );
		string_in_names_ptr = initStringInputNames->data();
		string_in_val_ptr = initStringInputVals->data();
		string_in_size = initStringInputNames->size();
	}

	XBT_DEBUG("string variables initialized");

	if(intOutputNames)
		model->defineIntegerOutputs( intOutputNames->data(), intOutputNames->size());

	if(initIntInputNames){
		model->defineIntegerInputs( initIntInputNames->data(), initIntInputNames->size());
		int_in_names_ptr = initIntInputNames->data();
		int_in_val_ptr = initIntInputVals->data();
		int_in_size = initIntInputNames->size();
	}

	XBT_DEBUG("int variables initialized");

	if(boolOutputNames)
		model->defineBooleanOutputs( boolOutputNames->data(), boolOutputNames->size() );

	if(initBoolInputNames){
		model->defineBooleanInputs( initBoolInputNames->data(), initBoolInputNames->size() );
		bool_in_names_ptr = initBoolInputNames->data();
		bool_in_val_ptr = bool_input_values.data();
		bool_in_size = initBoolInputNames->size();
	}

	XBT_DEBUG("bool variables initialized");

	int status = model->init( fmu_instance_name ,
						real_in_names_ptr, real_in_val_ptr, real_in_size,
						int_in_names_ptr, int_in_val_ptr, int_in_size,
						bool_in_names_ptr, bool_in_val_ptr, bool_in_size,
						string_in_names_ptr, string_in_val_ptr, string_in_size,
						startTime, lookahead, eventCheckStepSize, integratorStepSize );

	nextEvent = model->sync(-42.0,startTime);
	if(nextEvent==0.0){
		XBT_DEBUG("update state from the right at time 0.0");
		current_time = model->updateStateFromTheRight(0.0);
		XBT_DEBUG("predict state at time %f",current_time);
		nextEvent = model->predictState(current_time);
	}

}


FMUxxME::~FMUxxME() {
	delete model;
}

double FMUxxME::getRealOutputs(std::string name){
	return model->getRealOutputs()[real_output_names[name]];
}

bool FMUxxME::getBoolOutputs(std::string name){

	return model->getBooleanOutputs()[string_output_names[name]];
}

int FMUxxME::getIntegerOutputs(std::string name){
	return model->getIntegerOutputs()[int_output_names[name]];
}

std::string FMUxxME::getStringOutputs(std::string name){

	return model->getStringOutputs()[string_output_names[name]];
}

void FMUxxME::setRealInput(std::string name, double value, bool iterateAfter = true, bool sendNow = true){

	XBT_DEBUG("start setting real input !");
	FMUModel::setRealInput(name,value,iterateAfter,sendNow);
	if(sendNow){
		nextEvent = model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data());
		simgrid::simix::simcall([this]() { manageEventNotification(); });
	}
	XBT_DEBUG("real input set ! next event (i.e. discrete or continuous lookahead) for FMU at %f",nextEvent);
}

void FMUxxME::setBoolInput(std::string name, bool value,  bool iterateAfter = true, bool sendNow = true){

	FMUModel::setBoolInput(name,value,iterateAfter,sendNow);
	if(sendNow){
		nextEvent = model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data());
		simgrid::simix::simcall([this]() { manageEventNotification(); });
	}
}

void FMUxxME::setIntegerInput(std::string name, int value,  bool iterateAfter = true, bool sendNow = true){

	FMUModel::setIntegerInput(name,value,iterateAfter,sendNow);
	if(sendNow){
		nextEvent = model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data());
		simgrid::simix::simcall([this]() { manageEventNotification(); });
	}
}

void FMUxxME::setStringInput(std::string name, std::string value,  bool iterateAfter = true, bool sendNow = true){

	FMUModel::setStringInput(name,value,iterateAfter,sendNow);
	if(sendNow){
		nextEvent = model -> sync(SIMIX_get_clock(), SIMIX_get_clock(), real_input_values.data(), int_input_values.data(), bool_input_values.data(), string_input_values.data());
		simgrid::simix::simcall([this]() { manageEventNotification(); });
	}

}

void FMUxxME::update_actions_state(double now, double delta){
	XBT_INFO("updating FMU at time = %f, delta = %f",now,delta);

		nextEvent = model->sync( current_time , now );
		current_time = now;
		if(nextEvent==now){
			XBT_INFO("update state from the right at time %f",now);
			current_time = model->updateStateFromTheRight(now);
			XBT_INFO("predict state at time %f",current_time);
			nextEvent = model->predictState(current_time);
		}

	manageEventNotification();

}

double FMUxxME::next_occuring_event(double now){

	double diff = nextEvent-now;
	XBT_INFO("next event (i.e. discrete or continuous lookahead) for FMU in %f, at %f",diff,nextEvent);
	return nextEvent - now;
}


void FMUxxME::registerEvent(bool (*condition)(std::vector<std::string>),
		void (*handleEvent)(std::vector<std::string>),
		std::vector<std::string> handlerParam){

	event_handlers.push_back(handleEvent);
	event_conditions.push_back(condition);
	event_params.push_back(handlerParam);
}

void FMUxxME::manageEventNotification(){
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

void FMUxxME::deleteEvents(){
	event_handlers.clear();
	event_conditions.clear();
	event_params.clear();
}


}
}



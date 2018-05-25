#ifndef INCLUDE_SIMGRID_PLUGINS_FMI_HPP_
#define INCLUDE_SIMGRID_PLUGINS_FMI_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <simgrid/kernel/resource/Model.hpp>
#include <VariableStepSizeFMU.h>
#include <IntegratorType.h>
#include <IncrementalFMU.h>

namespace simgrid{
namespace fmi{

class FMUModel : public simgrid::kernel::resource::Model{

public:
	FMUModel(std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
			std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
			std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
			std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
			std::vector<std::string> *realOutputNames,
			std::vector<std::string> *intOutputNames,
			std::vector<std::string> *boolOutputNames,
			std::vector<std::string> *stringOutputNames);
	virtual ~FMUModel();
	double next_occuring_event(double now) override;
	void update_actions_state(double now, double delta) override;
	virtual double getRealOutputs(std::string name)=0;
	virtual bool getBoolOutputs(std::string name)=0;
	virtual int getIntegerOutputs(std::string name)=0;
	virtual std::string getStringOutputs(std::string name)=0;
	virtual void setRealInput(std::string name, double value, bool iterateAfter=true, bool sendNow=true);
	virtual void setBoolInput(std::string name, bool value, bool iterateAfter=true, bool sendNow=true);
	virtual void setIntegerInput(std::string name, int value, bool iterateAfter=true, bool sendNow=true);
	virtual void setStringInput(std::string name, std::string value, bool iterateAfter=true, bool sendNow=true);
	virtual void registerEvent(bool (*condition)(std::vector<std::string>),
			void (*handleEvent)(std::vector<std::string>),
			std::vector<std::string> handlerParam) = 0;
	virtual void deleteEvents() = 0;

protected:
	std::unordered_map<std::string, int> real_input_names;
	std::unordered_map<std::string, int> bool_input_names;
	std::unordered_map<std::string, int> int_input_names;
	std::unordered_map<std::string, int> string_input_names;

	std::vector<double> real_input_values;
	std::vector<char> bool_input_values;
	std::vector<int> int_input_values;
	std::vector<std::string> string_input_values;

	std::unordered_map<std::string, int> real_output_names;
	std::unordered_map<std::string, int> int_output_names;
	std::unordered_map<std::string, int> bool_output_names;
	std::unordered_map<std::string, int> string_output_names;

	std::vector<void (*)(std::vector<std::string>)> event_handlers;
	std::vector<bool (*)(std::vector<std::string>)> event_conditions;
	std::vector<std::vector<std::string>> event_params;
};



class FMUxxCS : public FMUModel{

private:
	VariableStepSizeFMU *model;
	double nextEvent;
	double commStep;
	void manageEventNotification();
	double current_time;

public:
	FMUxxCS(std::string fmu_uri, std::string fmu_name, std::string fmu_instance_name, const double startTime, 	const double stepSize,
					std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
					std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
					std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
					std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
					std::vector<std::string> *realOutputNames,
					std::vector<std::string> *intOutputNames,
					std::vector<std::string> *boolOutputNames,
					std::vector<std::string> *stringOutputNames);
	~FMUxxCS();
	void update_actions_state(double now, double delta) override;
	double getRealOutputs(std::string name) override;
	bool getBoolOutputs(std::string name) override;
	int getIntegerOutputs(std::string name) override;
	std::string getStringOutputs(std::string name) override;
	void setRealInput(std::string name, double value, bool iterateAfter, bool sendNow) override;
	void setBoolInput(std::string name, bool value, bool iterateAfter, bool sendNow) override;
	void setIntegerInput(std::string name, int value, bool iterateAfter, bool sendNow) override;
	void setStringInput(std::string name, std::string value, bool iterateAfter, bool sendNow) override;
	double next_occuring_event(double now) override;
	void registerEvent(bool (*condition)(std::vector<std::string>),
			void (*handleEvent)(std::vector<std::string>),
			std::vector<std::string> params) override;
	void deleteEvents() override;

};

class FMUxxME : public FMUModel{

private:
	IncrementalFMU *model;
	double nextEvent;
	double commStep;
	void manageEventNotification();
	double current_time;

public:
	FMUxxME(std::string fmu_uri, std::string fmu_name, std::string fmu_instance_name, const double startTime,
					const double integratorStepSize, const double eventCheckStepSize, const double lookahead, IntegratorType integrator,
					std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
					std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
					std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
					std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
					std::vector<std::string> *realOutputNames,
					std::vector<std::string> *intOutputNames,
					std::vector<std::string> *boolOutputNames,
					std::vector<std::string> *stringOutputNames);
	~FMUxxME();
	void update_actions_state(double now, double delta) override;
	double getRealOutputs(std::string name) override;
	bool getBoolOutputs(std::string name) override;
	int getIntegerOutputs(std::string name) override;
	std::string getStringOutputs(std::string name) override;
	void setRealInput(std::string name, double value, bool iterateAfter, bool sendNow) override;
	void setBoolInput(std::string name, bool value, bool iterateAfter, bool sendNow) override;
	void setIntegerInput(std::string name, int value, bool iterateAfter, bool sendNow) override;
	void setStringInput(std::string name, std::string value, bool iterateAfter, bool sendNow) override;
	double next_occuring_event(double now) override;
	void registerEvent(bool (*condition)(std::vector<std::string>),
			void (*handleEvent)(std::vector<std::string>),
			std::vector<std::string> params) override;
	void deleteEvents() override;

};

class FMIPlugin{

public:
	FMIPlugin();
	~FMIPlugin();
	static FMUModel* getFMU(std::string name);
	static void addFMUCS(std::string fmu_uri, std::string fmu_name, const double stepSize,
			std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
			std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
			std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
			std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
			std::vector<std::string> *realOutputNames,
			std::vector<std::string> *intOutputNames,
			std::vector<std::string> *boolOutputNames,
			std::vector<std::string> *stringOutputNames);

	static void addFMUME(std::string fmu_uri, std::string fmu_name,
			const double integratorStepSize, const double eventCheckStepSize, const double lookahead, IntegratorType integrator,
			std::vector<std::string> *initRealInputNames, std::vector<double> *initRealInputVals,
			std::vector<std::string> *initIntInputNames, std::vector<int> *initIntInputVals,
			std::vector<std::string> *initBoolInputNames, std::vector<bool> *initBoolInputVals,
			std::vector<std::string> *initStringInputNames, std::vector<std::string> *initStringInputVals,
			std::vector<std::string> *realOutputNames,
			std::vector<std::string> *intOutputNames,
			std::vector<std::string> *boolOutputNames,
			std::vector<std::string> *stringOutputNames);

private:
	static std::unordered_map<std::string,FMUModel*> fmus;

};

}
}

#endif /* INCLUDE_SIMGRID_PLUGINS_FMI_HPP_ */

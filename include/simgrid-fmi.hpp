#ifndef INCLUDE_SIMGRID_PLUGINS_FMI_HPP_
#define INCLUDE_SIMGRID_PLUGINS_FMI_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <simgrid/kernel/resource/Model.hpp>
#include "FMUCoSimulation_v1.h"
#include "FMUCoSimulation_v2.h"
#include "ModelManager.h"

namespace simgrid{
namespace fmi{

class FMUxxCS : public simgrid::kernel::resource::Model{

private:
	FMUCoSimulationBase *model;
	double nextEvent;
	double commStep;
	void manageEventNotification();
	double current_time;

protected:
	std::vector<void (*)(std::vector<std::string>)> event_handlers;
	std::vector<bool (*)(std::vector<std::string>)> event_conditions;
	std::vector<std::vector<std::string>> event_params;

public:
	FMUxxCS(std::string fmu_uri, std::string fmu_name, const double startTime, const double stepSize);
	~FMUxxCS();
	void update_actions_state(double now, double delta) override;
	double getRealOutput(std::string name);
	bool getBooleanOutput(std::string name);
	int getIntegerOutput(std::string name);
	std::string getStringOutput(std::string name);
	void setRealInput(std::string name, double value, bool iterateAfter=true);
	void setBooleanInput(std::string name, bool value, bool iterateAfter=true);
	void setIntegerInput(std::string name, int value, bool iterateAfter=true);
	void setStringInput(std::string name, std::string value, bool iterateAfter=true);
	double next_occuring_event(double now) override;
	void registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params);
	void deleteEvents();

};

class FMIPlugin{

public:
	FMIPlugin();
	~FMIPlugin();
	static FMUxxCS* getFMU(std::string name);
	static void addFMUCS(std::string fmu_uri, std::string fmu_name, const double stepSize);
private:
	static std::unordered_map<std::string,FMUxxCS*> fmus;

};

}
}

#endif /* INCLUDE_SIMGRID_PLUGINS_FMI_HPP_ */

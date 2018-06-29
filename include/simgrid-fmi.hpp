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

struct fmu_connection{
	std::string out_fmu_name;
	std::string output_port;
	std::string in_fmu_name;
	std::string input_port;
};

class MasterFMI : public simgrid::kernel::resource::Model{

private:
	std::unordered_map<std::string,FMUCoSimulationBase*> fmus;
	std::vector<fmu_connection> external_dependencies;
	double nextEvent;
	double commStep;
	void manageEventNotification();
	double current_time;
	std::vector<void (*)(std::vector<std::string>)> event_handlers;
	std::vector<bool (*)(std::vector<std::string>)> event_conditions;
	std::vector<std::vector<std::string>> event_params;

public:
	MasterFMI(const double stepSize);
	~MasterFMI();
	void addFMUCS(std::string fmu_uri, std::string fmu_name);
	void update_actions_state(double now, double delta) override;
	double getRealOutput(std::string fmi_name, std::string output_name);
	bool getBooleanOutput(std::string fmi_name, std::string output_name);
	int getIntegerOutput(std::string fmi_name, std::string output_name);
	std::string getStringOutput(std::string fmi_name, std::string output_name);
	void setRealInput(std::string fmi_name, std::string input_name, double value, bool iterateAfter);
	void setBooleanInput(std::string fmi_name, std::string input_name, bool value, bool iterateAfter);
	void setIntegerInput(std::string fmi_name, std::string input_name, int value, bool iterateAfter);
	void setStringInput(std::string fmi_name, std::string input_name, std::string value, bool iterateAfter);
	double next_occuring_event(double now) override;
	void registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params);
	void deleteEvents();
	void connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port);
};


class FMIPlugin {

public:
	static void addFMUCS(std::string fmu_uri, std::string fmu_name);
	static void connectFMU(std::string out_fmu_name,std::string output_port,std::string in_fmu_name,std::string input_port);
	static void initFMIPlugin(double communication_step);
	static double getRealOutput(std::string fmi_name, std::string output_name);
	static bool getBooleanOutput(std::string fmi_name, std::string output_name);
	static int getIntegerOutput(std::string fmi_name, std::string output_name);
	static std::string getStringOutput(std::string fmi_name, std::string output_name);
	static void setRealInput(std::string fmi_name, std::string input_name, double value, bool iterateAfter=true);
	static void setBooleanInput(std::string fmi_name, std::string input_name, bool value, bool iterateAfter=true);
	static void setIntegerInput(std::string fmi_name, std::string input_name, int value, bool iterateAfter=true);
	static void setStringInput(std::string fmi_name, std::string input_name, std::string value, bool iterateAfter=true);
	static void registerEvent(bool (*condition)(std::vector<std::string>), void (*handleEvent)(std::vector<std::string>), std::vector<std::string> params);
	static void deleteEvents();
	static MasterFMI *master;
protected:
	FMIPlugin();
	~FMIPlugin();
};

}
}

#endif /* INCLUDE_SIMGRID_PLUGINS_FMI_HPP_ */
